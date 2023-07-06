//
// Copyright (C) 2023 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "inet/physicallayer/wired/ethernet/NewEthernetCsmaPhy.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/Simsignals.h"
#include "inet/physicallayer/wired/ethernet/EthernetPhyHeader_m.h"

namespace inet {

namespace physicallayer {

Define_Module(NewEthernetCsmaPhy);

NewEthernetCsmaPhy::~NewEthernetCsmaPhy()
{
//    cancelAndDelete(txTimer);
    cancelAndDelete(rxChannelIdleTimer);
}

void NewEthernetCsmaPhy::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        physInGate = gate("phys$i");
        physInGate->setDeliverImmediately(true);
        physOutGate = gate("phys$o");
        upperLayerInGate = gate("upperLayerIn");
        upperLayerOutGate = gate("upperLayerOut");
        mac = getConnectedModule<INewEthernetCsmaMac>(gate("upperLayerOut"));
//        txTimer = new cMessage("TxTimer", TX_END);
        rxChannelIdleTimer = new cMessage("RxChannelIdleTimer", RX_CHANNEL_IDLE);
        fsm.setState(IDLE);
        setTxUpdateSupport(true);
    }
}

void NewEthernetCsmaPhy::handleMessage(cMessage *message)
{
    if (message->isSelfMessage())
        handleSelfMessage(message);
    else if (message->getArrivalGate() == upperLayerInGate)
        handleUpperPacket(check_and_cast<Packet *>(message));
    else if (message->getArrivalGate() == physInGate)
        handleEthernetSignal(check_and_cast<EthernetSignalBase *>(message));
    else
        throw cRuntimeError("Received unknown message");
}

void NewEthernetCsmaPhy::handleSelfMessage(cMessage *message)
{
    EV_DEBUG << "Handling self message" << EV_FIELD(message) << EV_ENDL;
    handleWithFsm(message->getKind(), message);
}

void NewEthernetCsmaPhy::handleUpperPacket(Packet *packet)
{
    EV_DEBUG << "Handling upper packet" << EV_FIELD(packet) << EV_ENDL;
    startFrameTransmission(packet);
}

void NewEthernetCsmaPhy::handleEthernetSignal(EthernetSignalBase *signal)
{
    EV_DEBUG << "Handling Ethernet signal" << EV_FIELD(signal) << EV_ENDL;
    handleWithFsm(signal->isUpdate() ? RX_UPDATE : RX_START, signal);
}

void NewEthernetCsmaPhy::handleWithFsm(int event, cMessage *message)
{
    FSMA_Switch(fsm) {
        FSMA_State(IDLE) {
            FSMA_Event_Transition(TX_START,
                                  event == TX_START,
                                  TRANSMITTING,
                startTransmit(check_and_cast<EthernetSignalBase *>(message));
            );
            FSMA_Event_Transition(RX_START,
                                  event == RX_START,
                                  RECEIVING,
                startReceive(check_and_cast<EthernetSignalBase *>(message));
            );
            FSMA_Fail_On_Unhandled_Event();
        }
        FSMA_State(TRANSMITTING) {
            FSMA_Event_Transition(TX_END,
                                  event == TX_END,
                                  IDLE,
                endTransmit();
            );
            FSMA_Event_Transition(RX_START,
                                  event == RX_START,
                                  COLLISION,
                startReceive(check_and_cast<EthernetSignalBase *>(message));
                mac->handleCollisionStart();
            );
            FSMA_Fail_On_Unhandled_Event();
        }
        FSMA_State(RECEIVING) {
            FSMA_Event_Transition(RX_START,
                                  event == RX_START,
                                  RECEIVING,
                updateRxSignals(check_and_cast<EthernetSignalBase *>(message));
            );
            FSMA_Event_Transition(RX_UPDATE,
                                  event == RX_UPDATE,
                                  RECEIVING,
                updateRxSignals(check_and_cast<EthernetSignalBase *>(message));
            );
            FSMA_Event_Transition(RX_CHANNEL_IDLE,
                                  event == RX_CHANNEL_IDLE,
                                  IDLE,
                endReceive();
            );
            FSMA_Event_Transition(TX_START,
                                  event == TX_START,
                                  COLLISION,
                // KLUDGE TODO this is needed for fingerprint compatibility
                startTransmit(check_and_cast<EthernetSignalBase *>(message));
                mac->handleCollisionStart();
            );
            FSMA_Fail_On_Unhandled_Event();
        }
        FSMA_State(COLLISION) {
            FSMA_Event_Transition(TX_END_NOT_RECEIVING,
                                  event == TX_END && rxSignals.empty(),
                                  IDLE,
                endTransmit();
                mac->handleCollisionEnd();
            );
            FSMA_Event_Transition(TX_END_RECEIVING,
                                  event == TX_END && !rxSignals.empty(),
                                  COLLISION,
                endTransmit();
            );
            FSMA_Event_Transition(RX_START,
                                  event == RX_START,
                                  COLLISION,
                updateRxSignals(check_and_cast<EthernetSignalBase *>(message));
            );
            FSMA_Event_Transition(RX_UPDATE,
                                  event == RX_UPDATE,
                                  COLLISION,
                updateRxSignals(check_and_cast<EthernetSignalBase *>(message));
            );
            FSMA_Event_Transition(RX_END_TRANSMITTING,
                                  event == RX_CHANNEL_IDLE && currentTxSignal != nullptr,
                                  COLLISION,
                endReceive();
            );
            FSMA_Event_Transition(RX_END_NOT_TRANSMITTING,
                                  event == RX_CHANNEL_IDLE && currentTxSignal == nullptr,
                                  IDLE,
                endReceive();
                mac->handleCollisionEnd();
            );
            FSMA_Fail_On_Unhandled_Event();
        }
    }
}

void NewEthernetCsmaPhy::startTransmit(EthernetSignalBase *signal)
{
    ASSERT(currentTxSignal == nullptr);
    EV_DEBUG << "Starting signal transmission" << EV_FIELD(signal) << EV_ENDL;
    auto bitrate = 100E+6; // TODO curEtherDescr->txrate);
    auto duration = signal->getBitLength() / bitrate;
    signal->setSrcMacFullDuplex(false);
    signal->setBitrate(bitrate);
    signal->setDuration(duration);
//    scheduleTxTimer(signal);
    currentTxSignal = signal;
//    auto packet = check_and_cast_nullable<Packet *>(currentTxSignal->getEncapsulatedPacket());
//    mac->handleTransmissionStart(static_cast<SignalType>(currentTxSignal->getKind()), packet);
    send(signal->dup(), SendOptions().transmissionId(signal->getId()).duration(duration), physOutGate);
}

void NewEthernetCsmaPhy::endTransmit()
{
    ASSERT(currentTxSignal != nullptr);
    EV_DEBUG << "Ending signal transmission" << EV_ENDL;
    simtime_t duration = simTime() - currentTxSignal->getCreationTime(); // TODO save and use start tx time
    if (duration != currentTxSignal->getDuration()) {
        cutEthernetSignalEnd(currentTxSignal, duration); // TODO save and use start tx time
        emit(transmissionEndedSignal, currentTxSignal);
        send(currentTxSignal, SendOptions().finishTx(currentTxSignal->getId()), physOutGate);
    }
    else
        delete currentTxSignal;
//    auto bitrate = 100E+6; // TODO curEtherDescr->txrate);
//    auto duration = currentTxSignal->getBitLength() / bitrate;
//    auto packet = check_and_cast_nullable<Packet *>(currentTxSignal->getEncapsulatedPacket());
//    mac->handleTransmissionEnd(static_cast<SignalType>(currentTxSignal->getKind()), packet);
//    if (currentTxSignal->getDuration() != duration)
//        send(currentTxSignal, SendOptions().updateTx(currentTxSignal->getId(), 0).duration(duration), physOutGate);
//    else
//        delete currentTxSignal;
    currentTxSignal = nullptr;
}

void NewEthernetCsmaPhy::startJamSignalTransmission()
{
    Enter_Method("transmitJamSignal");
    EV_DEBUG << "Starting jam signal transmission" << EV_ENDL;
    auto signal = new EthernetSignal("Jam");
    signal->setKind(JAM);
    signal->setByteLength(B(JAM_SIGNAL_BYTES).get());
//    cancelEvent(txTimer);
    handleWithFsm(TX_START, signal);
}

void NewEthernetCsmaPhy::startBeaconSignalTransmission()
{
    Enter_Method("startBeaconSignalTransmission");
    EV_DEBUG << "Starting beacon signal transmission" << EV_ENDL;
    auto signal = new EthernetSignal("Beacon");
    signal->setKind(BEACON);
    signal->setBitLength(20);
    handleWithFsm(TX_START, signal);
}

void NewEthernetCsmaPhy::startCommitSignalTransmission()
{
    Enter_Method("startCommitSignalTransmission");
    EV_DEBUG << "Starting commit signal transmission" << EV_ENDL;
    auto signal = new EthernetSignal("Commit");
    signal->setKind(COMMIT);
    // TODO: make it indefinite long?
    signal->setBitLength(512);
    handleWithFsm(TX_START, signal);
}

void NewEthernetCsmaPhy::startSignalTransmission(SignalType signalType)
{
    switch (signalType) {
        case JAM: startJamSignalTransmission(); break;
        case BEACON: startBeaconSignalTransmission(); break;
        case COMMIT: startCommitSignalTransmission(); break;
        default: throw cRuntimeError("Unknown signal type");
    }
}

void NewEthernetCsmaPhy::endSignalTransmission()
{
    Enter_Method("endSignalTransmission");
    EV_DEBUG << "Ending signal transmission" << EV_ENDL;
//    cancelEvent(txTimer);
    handleWithFsm(TX_END, currentTxSignal);
}

void NewEthernetCsmaPhy::startFrameTransmission(Packet *packet)
{
    Enter_Method("startFrameTransmission");
    EV_DEBUG << "Starting frame transmission" << EV_FIELD(packet) << EV_ENDL;
    take(packet);
    encapsulate(packet);
    auto signal = new EthernetSignal(packet->getName());
    signal->setKind(DATA);
    signal->encapsulate(packet);
    handleWithFsm(TX_START, signal);
}

void NewEthernetCsmaPhy::endFrameTransmission()
{
    Enter_Method("endFrameTransmission");
    EV_DEBUG << "Ending frame transmission" << EV_ENDL;
//    cancelEvent(txTimer);
    handleWithFsm(TX_END, currentTxSignal);
}

void NewEthernetCsmaPhy::startReceive(EthernetSignalBase *signal)
{
    auto packet = check_and_cast_nullable<Packet *>(signal->getEncapsulatedPacket());
    mac->handleCarrierSenseStart();
    mac->handleReceptionStart(static_cast<SignalType>(signal->getKind()), packet);
    updateRxSignals(signal);
}

void NewEthernetCsmaPhy::endReceive()
{
    if (rxSignals.size() == 1) {
        EthernetSignalBase *signal = rxSignals[0].signal;
        auto packet = check_and_cast_nullable<Packet *>(signal->decapsulate());
        if (packet != nullptr) {
            if (signal->getKind() == DATA)
                decapsulate(packet);
            mac->handleReceptionEnd(static_cast<SignalType>(signal->getKind()), packet);
        }
        delete signal;
    }
    rxSignals.clear();
    mac->handleCarrierSenseEnd();
}

EthernetSignalBase *NewEthernetCsmaPhy::getReceivedSignal()
{
    if (fsm.getState() == RECEIVING && rxSignals.size() == 1)
        return rxSignals[0].signal;
    else
        return nullptr;
}

void NewEthernetCsmaPhy::encapsulate(Packet *frame)
{
    auto phyHeader = makeShared<EthernetPhyHeader>();
    frame->insertAtFront(phyHeader);
    frame->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ethernetPhy);
}

void NewEthernetCsmaPhy::decapsulate(Packet *packet)
{
    auto phyHeader = packet->popAtFront<EthernetPhyHeader>();
    ASSERT(packet->getDataLength() >= MIN_ETHERNET_FRAME_BYTES);
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ethernetMac);
}

void NewEthernetCsmaPhy::cutEthernetSignalEnd(EthernetSignalBase *signal, simtime_t duration)
{
    ASSERT(duration <= signal->getDuration());
    if (duration == signal->getDuration())
        return;
    int64_t newBitLength = duration.dbl() * signal->getBitrate();
    if (auto packet = check_and_cast_nullable<Packet *>(signal->decapsulate())) {
        // TODO removed length calculation based on the PHY layer (parallel bits, bit order, etc.)
        if (newBitLength < packet->getBitLength()) {
            packet->trimFront();
            packet->setBackOffset(B(newBitLength / 8)); //TODO rounded down to byte align instead of b(newBitLength)
            packet->trimBack();
            packet->setBitError(true);
        }
        signal->encapsulate(packet);
    }
    signal->setBitError(true);
    signal->setBitLength(newBitLength);
    signal->setDuration(duration);
}

void NewEthernetCsmaPhy::updateRxSignals(EthernetSignalBase *signal)
{
    simtime_t endRxTime = simTime() + signal->getRemainingDuration();
    simtime_t maxEndRxTime = endRxTime;
    bool found = false;
    bool isUpdate = signal->isUpdate();
    for (auto& rx : rxSignals) {
        if (isUpdate && rx.signal->getTransmissionId() == signal->getTransmissionId()) {
            ASSERT(!found);
            found = true;
            delete rx.signal;
            rx.signal = signal;
            rx.endRxTime = endRxTime;
        }
        if (rx.endRxTime > maxEndRxTime)
            maxEndRxTime = rx.endRxTime;
    }
    if (!found)
        rxSignals.push_back(RxSignal(signal, endRxTime));
    if (rxChannelIdleTimer->getArrivalTime() != maxEndRxTime || endRxTime == maxEndRxTime)
        rescheduleAt(maxEndRxTime, rxChannelIdleTimer);
}

//void NewEthernetCsmaPhy::scheduleTxTimer(EthernetSignalBase *signal)
//{
//    scheduleAfter(signal->getDuration(), txTimer);
//}

} // namespace physicallayer

} // namespace inet

