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
#include "inet/physicallayer/wired/ethernet/EthernetPhyConstants.h"

namespace inet {

Define_Module(NewEthernetCsmaMac);

NewEthernetCsmaMac::~NewEthernetCsmaMac()
{
    cancelAndDelete(txTimer);
    cancelAndDelete(ifgTimer);
    cancelAndDelete(jamTimer);
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
        txTimer = new cMessage("TxTimer", TX_END);
        ifgTimer = new cMessage("IfgTimer", IFG_END);
        jamTimer = new cMessage("JamTimer", JAM_END);
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
    EV_DEBUG << "Handling upper packet" << EV_FIELD(packet) << EV_ENDL;
    handleWithFsm(UPPER_PACKET, packet);
}

void NewEthernetCsmaMac::handleLowerPacket(Packet *packet)
{
    EV_DEBUG << "Handling lower packet" << EV_FIELD(packet) << EV_ENDL;
    handleWithFsm(LOWER_PACKET, packet);
}

void NewEthernetCsmaMac::handleCarrierSenseStart()
{
    Enter_Method("handleCarrierSenseStart");
    ASSERT(!carrierSense);
    EV_DEBUG << "Handling carrier sense start" << EV_ENDL;
    carrierSense = true;
    handleWithFsm(CARRIER_SENSE_START, nullptr);
}

void NewEthernetCsmaMac::handleCarrierSenseEnd()
{
    Enter_Method("handleCarrierSenseEnd");
    ASSERT(carrierSense);
    EV_DEBUG << "Handling carrier sense end" << EV_ENDL;
    carrierSense = false;
    handleWithFsm(CARRIER_SENSE_END, nullptr);
}

void NewEthernetCsmaMac::handleCollisionStart()
{
    Enter_Method("handleCollisionStart");
    EV_DEBUG << "Handling collision start" << EV_ENDL;
    handleWithFsm(COLLISION_START, nullptr);
}

void NewEthernetCsmaMac::handleCollisionEnd()
{
    Enter_Method("handleCollisionEnd");
    EV_DEBUG << "Handling collision end" << EV_ENDL;
    // NOTE: this event is not needed in the FSM
}

void NewEthernetCsmaMac::handleReceptionStart(SignalType signalType, Packet *packet)
{
    Enter_Method("handleReceptionStart");
    EV_DEBUG << "Handling reception start" << EV_FIELD(signalType) << EV_FIELD(packet) << EV_ENDL;
    // NOTE: this event is not needed in the FSM
}

void NewEthernetCsmaMac::handleReceptionEnd(SignalType signalType, Packet *packet)
{
    Enter_Method("handleReceptionEnd");
    EV_DEBUG << "Handling reception end" << EV_FIELD(signalType) << EV_FIELD(packet) << EV_ENDL;
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
                abortTransmission();
                phy->startSignalTransmission(JAM);
            );
            FSMA_Ignore_Event(event == CARRIER_SENSE_START);
            FSMA_Fail_On_Unhandled_Event();
        }
        FSMA_State(JAMMING) {
            FSMA_Enter(scheduleJamTimer());
            FSMA_Event_Transition(JAM_END,
                                  event == JAM_END,
                                  BACKOFF,
                phy->endSignalTransmission();
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
    EV_DEBUG << "Starting frame transmission" << EV_FIELD(currentTxFrame) << EV_ENDL;
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
    scheduleTxTimer(currentTxFrame);
}

void NewEthernetCsmaMac::endTransmission()
{
    EV_DEBUG << "Ending frame transmission" << EV_FIELD(currentTxFrame) << EV_ENDL;
    phy->endFrameTransmission();
    delete currentTxFrame;
    currentTxFrame = nullptr;
    numRetries = 0;
}

void NewEthernetCsmaMac::abortTransmission()
{
    EV_DEBUG << "Aborting frame transmission" << EV_FIELD(currentTxFrame) << EV_ENDL;
    cancelEvent(txTimer);
    phy->endFrameTransmission();
}

void NewEthernetCsmaMac::retryTransmission()
{
    EV_DEBUG << "Retrying frame transmission" << EV_FIELD(currentTxFrame) << EV_ENDL;
    if (++numRetries > MAX_ATTEMPTS) {
        EV_DEBUG << "Number of retransmit attempts of frame exceeds maximum, cancelling transmission of frame\n";
        PacketDropDetails details;
        details.setReason(RETRY_LIMIT_REACHED);
        details.setLimit(MAX_ATTEMPTS);
        dropCurrentTxFrame(details);
        numRetries = 0;
    }
}

void NewEthernetCsmaMac::processReceivedFrame(Packet *packet)
{
    EV_DEBUG << "Processing received frame" << EV_FIELD(packet) << EV_ENDL;
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

void NewEthernetCsmaMac::scheduleTxTimer(Packet *packet)
{
    EV_DEBUG << "Scheduling TX timer" << EV_FIELD(packet) << EV_ENDL;
    simtime_t duration = b(packet->getDataLength() + ETHERNET_PHY_HEADER_LEN).get() / 100E+6; // TODO curEtherDescr->txrate;
    scheduleAfter(duration, txTimer);
}

void NewEthernetCsmaMac::scheduleIfgTimer()
{
    EV_DEBUG << "Scheduling IFG timer" << EV_ENDL;
    simtime_t duration = b(INTERFRAME_GAP_BITS).get() / 100E+6; // TODO curEtherDescr->txrate;
    scheduleAfter(duration, ifgTimer);
}

void NewEthernetCsmaMac::scheduleJamTimer()
{
    EV_DEBUG << "Scheduling jam timer" << EV_ENDL;
    simtime_t duration = b(JAM_SIGNAL_BYTES).get() / 100E+6; // TODO curEtherDescr->txrate;
    scheduleAfter(duration, jamTimer);
}

void NewEthernetCsmaMac::scheduleBackoffTimer()
{
    EV_DEBUG << "Scheduling backoff timer" << EV_ENDL;
    int backoffRange = (numRetries >= BACKOFF_RANGE_LIMIT) ? 1024 : (1 << numRetries);
    int slotNumber = intuniform(0, backoffRange - 1);
    EV_DEBUG << "Executing backoff procedure" << EV_FIELD(slotNumber) << ", backoffRange = [0," << backoffRange - 1 << "]" << EV_ENDL;
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

