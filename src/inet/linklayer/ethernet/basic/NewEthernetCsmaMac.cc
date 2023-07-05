//
// Copyright (C) 2023 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "inet/linklayer/ethernet/basic/NewEthernetCsmaMac.h"

#include "inet/common/checksum/EthernetCRC.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/linklayer/ethernet/common/Ethernet.h"

namespace inet {

Define_Module(NewEthernetCsmaMac);

NewEthernetCsmaMac::~NewEthernetCsmaMac()
{
    cancelAndDelete(ifgTimer);
    cancelAndDelete(backoffTimer);
}

void NewEthernetCsmaMac::initialize(int stage)
{
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        fcsMode = parseFcsMode(par("fcsMode"));
        txQueue = getQueue(gate(upperLayerInGateId));
        sendRawBytes = par("sendRawBytes");
        promiscuous = par("promiscuous");
        phy = getConnectedModule<INewEthernetCsmaPhy>(gate("lowerLayerOut"));
        ifgTimer = new cMessage("IfgTimer", IFG_END);
        backoffTimer = new cMessage("BackoffTimer", BACKOFF_END);
        fsm.setState(IDLE);
    }
}

void NewEthernetCsmaMac::configureNetworkInterface()
{
    // MTU: typical values are 576 (Internet de facto), 1500 (Ethernet-friendly),
    // 4000 (on some point-to-point links), 4470 (Cisco routers default, FDDI compatible)
    networkInterface->setMtu(par("mtu"));

    // capabilities
    networkInterface->setMulticast(true);
    networkInterface->setBroadcast(true);
}

void NewEthernetCsmaMac::handleSelfMessage(cMessage *message)
{
    handleWithFsm(message->getKind(), message);
}

void NewEthernetCsmaMac::handleUpperPacket(Packet *packet)
{
    handleWithFsm(UPPER_PACKET, packet);
}

void NewEthernetCsmaMac::handleLowerPacket(Packet *packet)
{
    handleWithFsm(LOWER_PACKET, packet);
}

void NewEthernetCsmaMac::handleCarrierSenseStart()
{
    Enter_Method("handleCarrierSenseStart");
    ASSERT(!carrierSense);
    carrierSense = true;
    handleWithFsm(CARRIER_SENSE_START, nullptr);
}

void NewEthernetCsmaMac::handleCarrierSenseEnd()
{
    Enter_Method("handleCarrierSenseEnd");
    ASSERT(carrierSense);
    carrierSense = false;
    handleWithFsm(CARRIER_SENSE_END, nullptr);
}

void NewEthernetCsmaMac::handleCollisionStart()
{
    Enter_Method("handleCollisionStart");
    handleWithFsm(COLLISION_START, nullptr);
}

void NewEthernetCsmaMac::handleCollisionEnd()
{
    Enter_Method("handleCollisionEnd");
    // NOTE: this event is not needed in the FSM
}

void NewEthernetCsmaMac::handleTransmissionStart(int signalType, Packet *packet)
{
    Enter_Method("handleTransmissionStart");
    // NOTE: this event is not needed in the FSM
}

void NewEthernetCsmaMac::handleTransmissionEnd(int signalType, Packet *packet)
{
    Enter_Method("handleTransmissionEnd");
    handleWithFsm(TX_END, packet);
}

void NewEthernetCsmaMac::handleReceptionStart(int signalType, Packet *packet)
{
    Enter_Method("handleReceptionStart");
    // NOTE: this event is not needed in the FSM
}

void NewEthernetCsmaMac::handleReceptionEnd(int signalType, Packet *packet)
{
    Enter_Method("handleReceptionEnd");
    take(packet);
    handleWithFsm(LOWER_PACKET, packet);
}

void NewEthernetCsmaMac::handleWithFsm(int event, cMessage *message)
{
    FSMA_Switch(fsm) {
        FSMA_State(IDLE) {
            FSMA_Enter(ASSERT(!carrierSense));
            FSMA_Event_Transition(UPPER_PACKET,
                                  event == UPPER_PACKET,
                                  TRANSMITTING,
                setCurrentTransmission(check_and_cast<Packet *>(message));
            );
            FSMA_Event_Transition(CARRIER_SENSE_START,
                                  event == CARRIER_SENSE_START,
                                  RECEIVING,
            );
            FSMA_Fail_On_Unhandled_Event();
        }
        FSMA_State(WAIT_IFG) {
            FSMA_Enter(scheduleIfgTimer());
            FSMA_Event_Transition(IFG_END_HAS_CURRENT_TX,
                                  event == IFG_END && currentTxFrame != nullptr,
                                  TRANSMITTING,
            );
            FSMA_Event_Transition(IFG_END_QUEUE_NOT_EMPTY,
                                  event == IFG_END && currentTxFrame == nullptr && !txQueue->isEmpty(),
                                  TRANSMITTING,
                setCurrentTransmission(txQueue->dequeuePacket());
            );
            FSMA_Event_Transition(IFG_END_QUEUE_EMPTY,
                                  event == IFG_END && currentTxFrame == nullptr && txQueue->isEmpty(),
                                  IDLE,
            );
            FSMA_Event_Transition(CARRIER_SENSE_START,
                                  event == CARRIER_SENSE_START,
                                  RECEIVING,
                cancelEvent(ifgTimer);
            );
            FSMA_Fail_On_Unhandled_Event();
        }
        FSMA_State(TRANSMITTING) {
            FSMA_Enter(startTransmission());
            FSMA_Event_Transition(TX_END,
                                  event == TX_END,
                                  WAIT_IFG,
                endTransmission();
            );
            FSMA_Event_Transition(COLLISION_START,
                                  event == COLLISION_START,
                                  JAMMING,
                phy->startJamSignalTransmission();
            );
            FSMA_Ignore_Event(event == CARRIER_SENSE_START);
            FSMA_Fail_On_Unhandled_Event();
        }
        FSMA_State(JAMMING) {
            FSMA_Event_Transition(TX_END,
                                  event == TX_END,
                                  BACKOFF,
                retryTransmission();
                scheduleBackoffTimer();
            );
            FSMA_Ignore_Event(event == CARRIER_SENSE_START);
            FSMA_Ignore_Event(event == CARRIER_SENSE_END);
            FSMA_Fail_On_Unhandled_Event();
        }
        FSMA_State(BACKOFF) {
            FSMA_Event_Transition(BACKOFF_END,
                                  event == BACKOFF_END && !carrierSense,
                                  WAIT_IFG,
            );
            FSMA_Event_Transition(BACKOFF_END,
                                  event == BACKOFF_END && carrierSense,
                                  RECEIVING,
            );
            FSMA_Event_Transition(LOWER_PACKET,
                                  event == LOWER_PACKET,
                                  BACKOFF,
                processReceivedFrame(check_and_cast<Packet *>(message));
            );
            FSMA_Ignore_Event(event == CARRIER_SENSE_START);
            FSMA_Ignore_Event(event == CARRIER_SENSE_END);
            FSMA_Fail_On_Unhandled_Event();
        }
        FSMA_State(RECEIVING) {
            FSMA_Event_Transition(LOWER_PACKET,
                                  event == LOWER_PACKET,
                                  RECEIVING,
                processReceivedFrame(check_and_cast<Packet *>(message));
            );
            FSMA_Event_Transition(CARRIER_SENSE_END,
                                  event == CARRIER_SENSE_END,
                                  WAIT_IFG,
            );
            FSMA_Fail_On_Unhandled_Event();
        }
    }
}

void NewEthernetCsmaMac::setCurrentTransmission(Packet *packet)
{
    ASSERT(currentTxFrame == nullptr);
    currentTxFrame = packet;
}

void NewEthernetCsmaMac::startTransmission()
{
    MacAddress address = getMacAddress();
    const auto& macHeader = currentTxFrame->peekAtFront<EthernetMacHeader>();
    if (macHeader->getDest().equals(address)) {
        throw cRuntimeError("Logic error: frame %s from higher layer has local MAC address as dest (%s)",
                currentTxFrame->getFullName(), macHeader->getDest().str().c_str());
    }
    if (currentTxFrame->getDataLength() > MAX_ETHERNET_FRAME_BYTES) {
        throw cRuntimeError("Packet length from higher layer (%s) exceeds maximum Ethernet frame size (%s)",
                currentTxFrame->getDataLength().str().c_str(), MAX_ETHERNET_FRAME_BYTES.str().c_str());
    }
    addPaddingAndSetFcs(currentTxFrame, MIN_ETHERNET_FRAME_BYTES);
    phy->startFrameTransmission(currentTxFrame->dup());
}

void NewEthernetCsmaMac::endTransmission()
{
    delete currentTxFrame;
    currentTxFrame = nullptr;
    numRetries = 0;
}

void NewEthernetCsmaMac::retryTransmission()
{
    if (++numRetries > MAX_ATTEMPTS) {
        EV_DETAIL << "Number of retransmit attempts of frame exceeds maximum, cancelling transmission of frame\n";
        PacketDropDetails details;
        details.setReason(RETRY_LIMIT_REACHED);
        details.setLimit(MAX_ATTEMPTS);
        dropCurrentTxFrame(details);
        numRetries = 0;
    }
}

void NewEthernetCsmaMac::processReceivedFrame(Packet *packet)
{
    const auto& header = packet->peekAtFront<EthernetMacHeader>();
    auto macAddressInd = packet->addTag<MacAddressInd>();
    macAddressInd->setSrcAddress(header->getSrc());
    macAddressInd->setDestAddress(header->getDest());
    if (isFrameNotForUs(header)) {
        EV_WARN << "Frame " << packet->getName() << " not destined to us, discarding\n";
        PacketDropDetails details;
        details.setReason(NOT_ADDRESSED_TO_US);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
    }
    else {
        packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ethernetMac);
        packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ethernetMac);
        if (networkInterface)
            packet->addTagIfAbsent<InterfaceInd>()->setInterfaceId(networkInterface->getInterfaceId());
        sendUp(packet);
    }
}

bool NewEthernetCsmaMac::isFrameNotForUs(const Ptr<const EthernetMacHeader>& header)
{
    // Current ethernet mac implementation does not support the configuration of multicast
    // ethernet address groups. We rather accept all multicast frames (just like they were
    // broadcasts) and pass it up to the higher layer where they will be dropped
    // if not needed.
    //
    // PAUSE frames must be handled a bit differently because they are processed at
    // this level. Multicast PAUSE frames should not be processed unless they have a
    // destination of MULTICAST_PAUSE_ADDRESS. We drop all PAUSE frames that have a
    // different muticast destination. (Note: Would the multicast ethernet addressing
    // implemented, we could also process the PAUSE frames destined to any of our
    // multicast adresses)
    // All NON-PAUSE frames must be passed to the upper layer if the interface is
    // in promiscuous mode.

    if (header->getDest().equals(getMacAddress()))
        return false;

    if (header->getDest().isBroadcast())
        return false;

    bool isPause = (header->getTypeOrLength() == ETHERTYPE_FLOW_CONTROL);

    if (!isPause && (promiscuous || header->getDest().isMulticast()))
        return false;

    if (isPause && header->getDest().equals(MacAddress::MULTICAST_PAUSE_ADDRESS))
        return false;

    return true;
}

IPassivePacketSource *NewEthernetCsmaMac::getProvider(const cGate *gate)
{
    return gate->getId() == upperLayerInGateId ? txQueue.get() : nullptr;
}

void NewEthernetCsmaMac::handleCanPullPacketChanged(const cGate *gate)
{
    Enter_Method("handleCanPullPacketChanged");
    if (!carrierSense && currentTxFrame == nullptr && fsm.getState() == IDLE && canDequeuePacket()) {
        Packet *packet = dequeuePacket();
        handleUpperPacket(packet);
    }
}

void NewEthernetCsmaMac::scheduleIfgTimer()
{
    simtime_t ifgDuration = b(INTERFRAME_GAP_BITS).get() / 100E+6; // TODO curEtherDescr->txrate;
    scheduleAfter(ifgDuration, ifgTimer);
}

void NewEthernetCsmaMac::scheduleBackoffTimer()
{
    int backoffRange = (numRetries >= BACKOFF_RANGE_LIMIT) ? 1024 : (1 << numRetries);
    int slotNumber = intuniform(0, backoffRange - 1);
    EV_DETAIL << "Executing backoff procedure (slotNumber=" << slotNumber << ", backoffRange=[0," << backoffRange - 1 << "]" << endl;
    simtime_t backoffDuration = slotNumber * 512 / 100E+6; // TODO curEtherDescr->slotTime;
    scheduleAfter(backoffDuration, backoffTimer);
}

void NewEthernetCsmaMac::addPaddingAndSetFcs(Packet *packet, B requiredMinBytes) const
{
    auto ethFcs = packet->removeAtBack<EthernetFcs>(ETHER_FCS_BYTES);
    ethFcs->setFcsMode(fcsMode);

    B paddingLength = requiredMinBytes - ETHER_FCS_BYTES - B(packet->getByteLength());
    if (paddingLength > B(0)) {
        const auto& ethPadding = makeShared<EthernetPadding>();
        ethPadding->setChunkLength(paddingLength);
        packet->insertAtBack(ethPadding);
    }

    switch (ethFcs->getFcsMode()) {
        case FCS_DECLARED_CORRECT:
            ethFcs->setFcs(0xC00DC00DL);
            break;
        case FCS_DECLARED_INCORRECT:
            ethFcs->setFcs(0xBAADBAADL);
            break;
        case FCS_COMPUTED: { // calculate FCS
            auto ethBytes = packet->peekDataAsBytes();
            auto bufferLength = B(ethBytes->getChunkLength()).get();
            auto buffer = new uint8_t[bufferLength];
            // 1. fill in the data
            ethBytes->copyToBuffer(buffer, bufferLength);
            // 2. compute the FCS
            auto computedFcs = ethernetCRC(buffer, bufferLength);
            delete[] buffer;
            ethFcs->setFcs(computedFcs);
            break;
        }
        default:
            throw cRuntimeError("Unknown FCS mode: %d", (int)(ethFcs->getFcsMode()));
    }

    packet->insertAtBack(ethFcs);
}

} // namespace inet

