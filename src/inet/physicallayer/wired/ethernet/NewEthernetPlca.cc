//
// Copyright (C) 2023 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "inet/physicallayer/wired/ethernet/NewEthernetPlca.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/Simsignals.h"
#include "inet/physicallayer/wired/ethernet/EthernetPhyHeader_m.h"

namespace inet {

namespace physicallayer {

Define_Module(NewEthernetPlca);

NewEthernetPlca::~NewEthernetPlca()
{
    cancelAndDelete(beacon_timer);
    cancelAndDelete(beacon_det_timer);
    cancelAndDelete(invalid_beacon_timer);
    cancelAndDelete(burst_timer);
    cancelAndDelete(to_timer);
    cancelAndDelete(pending_timer);
    cancelAndDelete(commit_timer);
    cancelAndDelete(plca_status_timer);
}

void NewEthernetPlca::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        plca_en = par("plca_en");
        plca_node_count = par("plca_node_count");
        local_nodeID = par("local_nodeID");
        max_bc = par("max_bc");
        to_interval = par("to_interval");
        burst_interval = par("burst_interval");
        phy = getConnectedModule<INewEthernetCsmaPhy>(gate("lowerLayerOut"));
        mac = getConnectedModule<INewEthernetCsmaMac>(gate("upperLayerOut"));
        beacon_timer = new cMessage("beacon_timer", BEACON_TIMER_DONE);
        beacon_det_timer = new cMessage("beacon_det_timer", BEACON_DET_TIMER_DONE);
        invalid_beacon_timer = new cMessage("invalid_beacon_timer", INVALID_BEACON_TIMER_DONE);
        burst_timer = new cMessage("burst_timer", BURST_TIMER_DONE);
        to_timer = new cMessage("to_timer", TO_TIMER_DONE);
        pending_timer = new cMessage("pending_timer", PENDING_TIMER_DONE);
        commit_timer = new cMessage("commit_timer", COMMIT_TIMER_DONE);
        plca_status_timer = new cMessage("plca_status_timer", PLCA_STATUS_TIMER_DONE);
    }
    else if (stage == INITSTAGE_PHYSICAL_LAYER) {
        controlFsm.setState(CS_DISABLE, "CS_DISABLE");
        dataFsm.setState(DS_NORMAL, "DS_NORMAL");
        statusFsm.setState(SS_INACTIVE, "SS_INACTIVE");
        handleWithFSM();
    }
}

void NewEthernetPlca::refreshDisplay() const
{
    auto& displayString = getDisplayString();
    std::stringstream stream;
    stream << curID << "/" << plca_node_count << " (" << local_nodeID << ")\n";
    stream << controlFsm.getStateName() << " - " << dataFsm.getStateName() << " - " << statusFsm.getStateName();
    displayString.setTagArg("t", 0, stream.str().c_str());
}

void NewEthernetPlca::handleMessage(cMessage *message)
{
    if (message->isSelfMessage())
        handleSelfMessage(message);
    // TODO remove, not need because state machine handles all events
    else if (message->getArrivalGate() == gate("upperLayerIn"))
        ; // send(message, "lowerLayerOut");
    // TODO remove, not need because state machine handles all events
    else if (message->getArrivalGate() == gate("lowerLayerIn"))
        ; // send(message, "upperLayerOut");
    else
        throw cRuntimeError("Unknown message");
}

void NewEthernetPlca::handleSelfMessage(cMessage *message)
{
    EV_DEBUG << "Handling self message" << EV_FIELD(message) << EV_ENDL;
    handleWithFSM();
}

void NewEthernetPlca::handleCarrierSenseStart()
{
    Enter_Method("handleCarrierSenseStart");
    ASSERT(!CRS);
    CRS = true;
    EV_DEBUG << "Handling carrier sense start" << EV_FIELD(CRS) << EV_ENDL;
    handleWithFSM();
}

void NewEthernetPlca::handleCarrierSenseEnd()
{
    Enter_Method("handleCarrierSenseEnd");
    ASSERT(CRS);
    CRS = false;
    EV_DEBUG << "Handling carrier sense end" << EV_FIELD(CRS) << EV_ENDL;
    handleWithFSM();
}

void NewEthernetPlca::handleCollisionStart()
{
    throw cRuntimeError("Invalid operation");
}

void NewEthernetPlca::handleCollisionEnd()
{
    throw cRuntimeError("Invalid operation");
}

void NewEthernetPlca::handleTransmissionEnd()
{
    Enter_Method("handleTransmissionEnd");
    EV_DEBUG << "Handling transmission end" << EV_ENDL;
    mac->handleTransmissionEnd();
}

void NewEthernetPlca::handleReceivedPacket(Packet *packet)
{
    Enter_Method("handleReceivedPacket");
    EV_DEBUG << "Handling received packet" << EV_FIELD(packet) << EV_ENDL;
    mac->handleReceivedPacket(packet);
}

void NewEthernetPlca::startJamSignalTransmission()
{
    throw cRuntimeError("Invalid operation");
}

EthernetSignalBase *NewEthernetPlca::getReceivedSignal()
{
    throw cRuntimeError("Invalid operation");
}

void NewEthernetPlca::handleWithControlFSM()
{
    // 148.4.4.6 State diagram of IEEE Std 802.3cg-2019
    FSMA_Switch(controlFsm) {
        FSMA_State(CS_DISABLE) {
            FSMA_Enter(
                phy->endSignalTransmission();
                committed = false;
                curID = 0;
                plca_active = false;
            );
            FSMA_Transition(TRANSITION1,
                            plca_en && local_nodeID != 0 && local_nodeID != 255,
                            CS_RESYNC,
            );
            FSMA_Transition(TRANSITION2,
                            plca_en && local_nodeID == 0,
                            CS_RECOVER,
            );
            FSMA_Transition(TRANSITION3, // open arrow
                            plca_reset || !plca_en || local_nodeID == 255,
                            CS_DISABLE,
            );
//            FSMA_Transition(TRANSITION4, // open arrow
//                            !invalid_beacon_timer->isScheduled(),
//                            CS_RESYNC,
//            );
        }
        FSMA_State(CS_RESYNC) {
            FSMA_Enter(
                plca_active = false;
            );
            FSMA_Transition(TRANSITION1,
                            local_nodeID != 0 && CRS,
                            CS_EARLY_RECEIVE,
            );
            FSMA_Transition(TRANSITION2,
                            PMCD && !CRS && local_nodeID == 0,
                            CS_SEND_BEACON,
            );
            FSMA_Transition(TRANSITION3, // open arrow
                            plca_reset || !plca_en || local_nodeID == 255,
                            CS_DISABLE,
            );
//            FSMA_Transition(TRANSITION4, // open arrow
//                            !invalid_beacon_timer->isScheduled(),
//                            CS_RESYNC,
//            );
        }
        FSMA_State(CS_RECOVER) {
            FSMA_Enter(
                plca_active = false;
            );
            FSMA_Transition(TRANSITION1,
                            true,
                            CS_WAIT_TO,
            );
            FSMA_Transition(TRANSITION2, // open arrow
                            plca_reset || !plca_en || local_nodeID == 255,
                            CS_DISABLE,
            );
//            FSMA_Transition(TRANSITION3, // open arrow
//                            !invalid_beacon_timer->isScheduled(),
//                            CS_RESYNC,
//            );
        }
        FSMA_State(CS_SEND_BEACON) {
            FSMA_Enter(
                scheduleAfter(20 / 100E+6, beacon_timer);
                phy->startBeaconSignalTransmission();
                plca_active = true;
            );
            FSMA_Transition(TRANSITION1,
                            !beacon_timer->isScheduled(),
                            CS_SYNCING,
            );
            FSMA_Transition(TRANSITION2, // open arrow
                            plca_reset || !plca_en || local_nodeID == 255,
                            CS_DISABLE,
            );
//            FSMA_Transition(TRANSITION3, // open arrow
//                            !invalid_beacon_timer->isScheduled(),
//                            CS_RESYNC,
//            );
        }
        FSMA_State(CS_SYNCING) {
            FSMA_Enter(
                curID = 0;
                phy->endSignalTransmission();
                plca_active = true;
                if (local_nodeID != 0 && rx_cmd != "BEACON")
                    scheduleAfter(4000E-9, invalid_beacon_timer);
            );
            FSMA_Transition(TRANSITION1,
                            !CRS,
                            CS_WAIT_TO,
            );
            FSMA_Transition(TRANSITION2, // open arrow
                            plca_reset || !plca_en || local_nodeID == 255,
                            CS_DISABLE,
            );
//            FSMA_Transition(TRANSITION3, // open arrow
//                            !invalid_beacon_timer->isScheduled(),
//                            CS_RESYNC,
//            );
        }
        FSMA_State(CS_WAIT_TO) {
            FSMA_Enter(
                scheduleAfter(to_interval, to_timer);
            );
            FSMA_Transition(TRANSITION1,
                            CRS,
                            CS_EARLY_RECEIVE,
            );
            FSMA_Transition(TRANSITION2,
                            plca_active && curID == local_nodeID && packetPending && !CRS,
                            CS_COMMIT,
            );
            FSMA_Transition(TRANSITION3,
                            !to_timer->isScheduled() && curID != local_nodeID && !CRS,
                            CS_NEXT_TX_OPPORTUNITY,
            );
            FSMA_Transition(TRANSITION4,
                            curID == local_nodeID && (!packetPending || !plca_active) && !CRS,
                            CS_YIELD,
            );
            FSMA_Transition(TRANSITION5, // open arrow
                            plca_reset || !plca_en || local_nodeID == 255,
                            CS_DISABLE,
            );
//            FSMA_Transition(TRANSITION6, // open arrow
//                            !invalid_beacon_timer->isScheduled(),
//                            CS_RESYNC,
//            );
        }
        FSMA_State(CS_EARLY_RECEIVE) {
            FSMA_Enter(
                cancelEvent(to_timer);
                scheduleAfter(22 / 100E+6, beacon_det_timer);
            );
            FSMA_Transition(TRANSITION1, // D
                            local_nodeID != 0 && !receiving && (rx_cmd == "BEACON" || (!CRS && beacon_det_timer->isScheduled())),
                            CS_SYNCING,
            );
            FSMA_Transition(TRANSITION2, // B
                            !CRS && local_nodeID != 0 && rx_cmd != "BEACON" && !beacon_det_timer->isScheduled(),
                            CS_RESYNC,
            );
            FSMA_Transition(TRANSITION3, // C
                            !CRS && local_nodeID == 0,
                            CS_RECOVER,
            );
            FSMA_Transition(TRANSITION4,
                            receiving && CRS,
                            CS_RECEIVE,
            );
            FSMA_Transition(TRANSITION5, // open arrow
                            plca_reset || !plca_en || local_nodeID == 255,
                            CS_DISABLE,
            );
//            FSMA_Transition(TRANSITION6, // open arrow
//                            !invalid_beacon_timer->isScheduled(),
//                            CS_RESYNC,
//            );
        }
        FSMA_State(CS_COMMIT) {
            FSMA_Enter(
                phy->startCommitSignalTransmission();
                committed = true;
                cancelEvent(to_timer);
                bc = 0;
            );
            FSMA_Transition(TRANSITION1,
                            TX_EN,
                            CS_TRANSMIT,
            );
            FSMA_Transition(TRANSITION2,
                            !TX_EN && !packetPending,
                            CS_ABORT,
            );
            FSMA_Transition(TRANSITION3, // open arrow
                            plca_reset || !plca_en || local_nodeID == 255,
                            CS_DISABLE,
            );
//            FSMA_Transition(TRANSITION4, // open arrow
//                            !invalid_beacon_timer->isScheduled(),
//                            CS_RESYNC,
//            );
        }
        FSMA_State(CS_YIELD) {
            FSMA_Transition(TRANSITION1,
                            CRS && to_timer->isScheduled(),
                            CS_EARLY_RECEIVE,
            );
            FSMA_Transition(TRANSITION2,
                            !to_timer->isScheduled(),
                            CS_NEXT_TX_OPPORTUNITY,
            );
            FSMA_Transition(TRANSITION3, // open arrow
                            plca_reset || !plca_en || local_nodeID == 255,
                            CS_DISABLE,
            );
//            FSMA_Transition(TRANSITION4, // open arrow
//                            !invalid_beacon_timer->isScheduled(),
//                            CS_RESYNC,
//            );
        }
        FSMA_State(CS_RECEIVE) {
            FSMA_Transition(TRANSITION1,
                            !CRS,
                            CS_NEXT_TX_OPPORTUNITY,
            );
            FSMA_Transition(TRANSITION2, // open arrow
                            plca_reset || !plca_en || local_nodeID == 255,
                            CS_DISABLE,
            );
//            FSMA_Transition(TRANSITION3, // open arrow
//                            !invalid_beacon_timer->isScheduled(),
//                            CS_RESYNC,
//            );
        }
        FSMA_State(CS_TRANSMIT) {
            FSMA_Enter(
                phy->endSignalTransmission();
                if (bc >= max_bc)
                    committed = false;
            );
            FSMA_Transition(TRANSITION1,
                            !TX_EN && !CRS && bc >= max_bc,
                            CS_NEXT_TX_OPPORTUNITY,
            );
            FSMA_Transition(TRANSITION2,
                            !TX_EN && bc < max_bc,
                            CS_BURST,
            );
            FSMA_Transition(TRANSITION3, // open arrow
                            plca_reset || !plca_en || local_nodeID == 255,
                            CS_DISABLE,
            );
//            FSMA_Transition(TRANSITION4, // open arrow
//                            !invalid_beacon_timer->isScheduled(),
//                            CS_RESYNC,
//            );
        }
        FSMA_State(CS_BURST) {
            FSMA_Enter(
                bc = bc + 1;
                phy->startCommitSignalTransmission();
                scheduleAfter(burst_interval, burst_timer);
            );
            FSMA_Transition(TRANSITION1,
                            TX_EN,
                            CS_TRANSMIT,
            );
            FSMA_Transition(TRANSITION2,
                            !TX_EN && !burst_timer->isScheduled(),
                            CS_ABORT,
            );
            FSMA_Transition(TRANSITION3, // open arrow
                            plca_reset || !plca_en || local_nodeID == 255,
                            CS_DISABLE,
            );
//            FSMA_Transition(TRANSITION4, // open arrow
//                            !invalid_beacon_timer->isScheduled(),
//                            CS_RESYNC,
//            );
        }
        FSMA_State(CS_ABORT) {
            FSMA_Enter(
                phy->endSignalTransmission();
            );
            FSMA_Transition(TRANSITION1,
                            !CRS,
                            CS_NEXT_TX_OPPORTUNITY,
            );
            FSMA_Transition(TRANSITION2, // open arrow
                            plca_reset || !plca_en || local_nodeID == 255,
                            CS_DISABLE,
            );
//            FSMA_Transition(TRANSITION3, // open arrow
//                            !invalid_beacon_timer->isScheduled(),
//                            CS_RESYNC,
//            );
        }
        FSMA_State(CS_NEXT_TX_OPPORTUNITY) {
            FSMA_Enter(
                curID = curID + 1;
                committed = false;
            );
            FSMA_Transition(TRANSITION1, // B
                            (local_nodeID == 0 && curID >= plca_node_count) || curID == 255,
                            CS_RESYNC,
            );
            FSMA_Transition(TRANSITION2,
                            true,
                            CS_WAIT_TO,
            );
            FSMA_Transition(TRANSITION3, // open arrow
                            plca_reset || !plca_en || local_nodeID == 255,
                            CS_DISABLE,
            );
//            FSMA_Transition(TRANSITION4, // open arrow
//                            !invalid_beacon_timer->isScheduled(),
//                            CS_RESYNC,
//            );
        }
    }
}

void NewEthernetPlca::handleWithDataFSM()
{
    // 148.4.5.7 State diagram of IEEE Std 802.3cg-2019
    FSMA_Switch(dataFsm) {
        FSMA_State(DS_NORMAL) {
            FSMA_Enter(
                packetPending = false;
                if (CRS)
                    CARRIER_STATUS = "CARRIER_ON";
                else
                    CARRIER_STATUS = "CARRIER_OFF";
//                    TXD = plca_txd
                TX_EN = plca_txen;
                TX_ER = plca_txer;
                if (COL)
                    SIGNAL_STATUS = "SIGNAL_ERROR";
                else
                    SIGNAL_STATUS = "NO_SIGNAL_ERROR";
            );
            FSMA_Transition(TRANSITION1,
                            plca_en && !plca_reset && plca_status,
                            DS_IDLE,
            );
//            FSMA_Transition(TRANSITION2,
//                            true,
//                            DS_NORMAL,
//            );
            FSMA_Transition(TRANSITION3, // open arrow
                            plca_reset || !plca_en || !plca_status,
                            DS_IDLE,
            );
        }
        FSMA_State(DS_WAIT_IDLE) {
            FSMA_Enter(
                packetPending = false;
                CARRIER_STATUS = "CARRIER_OFF";
                SIGNAL_STATUS = "NO_SIGNAL_ERROR";
//                    TXD = ENCODE_TXD(tx_cmd_sync)
                TX_EN = false;
//                    TX_ER = ENCODE_TXER(tx_cmd_sync)
                a = 0;
                b = 0;
            );
            FSMA_Transition(TRANSITION1,
                            MCD && !CRS,
                            DS_IDLE,
            );
            FSMA_Transition(TRANSITION2, // B
                            MCD && CRS && plca_txen,
                            DS_TRANSMIT,
            );
//            FSMA_Transition(TRANSITION3,
//                            true,
//                            DS_WAIT_IDLE,
//            );
            FSMA_Transition(TRANSITION4, // open arrow
                            plca_reset || !plca_en || !plca_status,
                            DS_IDLE,
            );
        }
        FSMA_State(DS_IDLE) {
            FSMA_Enter(
                packetPending = false;
                CARRIER_STATUS = "CARRIER_OFF";
                SIGNAL_STATUS = "NO_SIGNAL_ERROR";
//                    TXD = ENCODE_TXD(tx_cmd_sync)
                TX_EN = false;
//                    TX_ER = ENCODE_TXER(tx_cmd_sync)
                a = 0;
                b = 0;
            );
            FSMA_Transition(TRANSITION1,
                            receiving && !plca_txen && tx_cmd == "NONE",
                            DS_RECEIVE,
            );
            FSMA_Transition(TRANSITION2,
                            plca_txen,
                            DS_HOLD,
            );
//            FSMA_Transition(TRANSITION3,
//                            true,
//                            DS_IDLE,
//            );
            FSMA_Transition(TRANSITION4, // open arrow
                            plca_reset || !plca_en || !plca_status,
                            DS_IDLE,
            );
        }
        FSMA_State(DS_RECEIVE) {
            FSMA_Enter(
                if (CRS && rx_cmd == "COMMIT")
                    CARRIER_STATUS = "CARRIER_ON";
                else
                    CARRIER_STATUS = "CARRIER_OFF";
//                    TXD = ENCODE_TXD(tx_cmd_sync)
//                    TX_ER = ENCODE_TXER(tx_cmd_sync)
            );
            FSMA_Transition(TRANSITION1,
                            !receiving && !plca_txen,
                            DS_IDLE,
            );
            FSMA_Transition(TRANSITION2,
                            plca_txen,
                            DS_COLLIDE,
            );
//            FSMA_Transition(TRANSITION3,
//                            true,
//                            DS_RECEIVE,
//            );
            FSMA_Transition(TRANSITION4, // open arrow
                            plca_reset || !plca_en || !plca_status,
                            DS_IDLE,
            );
        }
        FSMA_State(DS_HOLD) {
            FSMA_Enter(
                packetPending = true;
                CARRIER_STATUS = "CARRIER_ON";
                a = a + 1;
//                    TX_ER = ENCODE_TXER(tx_cmd_sync)
//                    TXD = ENCODE_TXD(tx_cmd_sync)
            );
            FSMA_Transition(TRANSITION1,
                            MCD && !committed && !plca_txer && !receiving && a < delay_line_length,
                            DS_HOLD,
            );
            FSMA_Transition(TRANSITION2,
                            !plca_txer && (receiving || a > delay_line_length),
                            DS_COLLIDE,
            );
            FSMA_Transition(TRANSITION3,
                            MCD && committed && !receiving && !plca_txer && a < delay_line_length,
                            DS_TRANSMIT,
            );
            FSMA_Transition(TRANSITION4,
                            MCD && plca_txer,
                            DS_ABORT,
            );
            FSMA_Transition(TRANSITION5, // open arrow
                            plca_reset || !plca_en || !plca_status,
                            DS_IDLE,
            );
        }
        FSMA_State(DS_ABORT) {
            FSMA_Enter(
                    packetPending = false;
//                    TX_ER = ENCODE_TXER(tx_cmd_sync)
//                    TXD = ENCODE_TXD(tx_cmd_sync)
            );
            FSMA_Transition(TRANSITION1,
                            !plca_txen,
                            DS_ABORT,
            );
            FSMA_Transition(TRANSITION2,
                            true,
                            DS_IDLE,
            );
            FSMA_Transition(TRANSITION3, // open arrow
                            plca_reset || !plca_en || !plca_status,
                            DS_IDLE,
            );
        }
        FSMA_State(DS_COLLIDE) {
            FSMA_Enter(
                packetPending = false;
                CARRIER_STATUS = "CARRIER_ON";
                SIGNAL_STATUS = "SIGNAL_ERROR";
                a = 0;
                b = 0;
//                    TXD = ENCODE_TXD(tx_cmd_sync)
//                    TX_ER = ENCODE_TXER(tx_cmd_sync)
                scheduleAfter(512 / 100E+6, pending_timer);
            );
            FSMA_Transition(TRANSITION1,
                            !plca_txen,
                            DS_DELAY_PENDING,
            );
//            FSMA_Transition(TRANSITION2,
//                            true,
//                            DS_COLLIDE,
//            );
            FSMA_Transition(TRANSITION3, // open arrow
                            plca_reset || !plca_en || !plca_status,
                            DS_IDLE,
            );
        }
        FSMA_State(DS_DELAY_PENDING) {
            FSMA_Enter(
                SIGNAL_STATUS = "NO_SIGNAL_ERROR";
//                    TXD = ENCODE_TXD(tx_cmd_sync)
//                    TX_ER = ENCODE_TXER(tx_cmd_sync)
            );
            FSMA_Transition(TRANSITION1,
                            !pending_timer->isScheduled(),
                            DS_PENDING,
            );
//            FSMA_Transition(TRANSITION2,
//                            true,
//                            DS_DELAY_PENDING,
//            );
            FSMA_Transition(TRANSITION3, // open arrow
                            plca_reset || !plca_en || !plca_status,
                            DS_IDLE,
            );
        }
        FSMA_State(DS_PENDING) {
            FSMA_Enter(
                packetPending = true;
                scheduleAfter(288 / 100E+6, commit_timer);
//                    TXD = ENCODE_TXD(tx_cmd_sync)
//                    TX_ER = ENCODE_TXER(tx_cmd_sync)
            );
            FSMA_Transition(TRANSITION1,
                            committed,
                            DS_WAIT_MAC,
            );
//            FSMA_Transition(TRANSITION2,
//                            true,
//                            DS_PENDING,
//            );
            FSMA_Transition(TRANSITION3, // open arrow
                            plca_reset || !plca_en || !plca_status,
                            DS_IDLE,
            );
        }
        FSMA_State(DS_WAIT_MAC) {
            FSMA_Enter(
                CARRIER_STATUS = "CARRIER_OFF";
//                    TXD = ENCODE_TXD(tx_cmd_sync)
//                    TX_ER = ENCODE_TXER(tx_cmd_sync)
            );
            FSMA_Transition(TRANSITION1,
                            MCD && plca_txen,
                            DS_TRANSMIT,
            );
            FSMA_Transition(TRANSITION2, // C
                            !plca_txen && !commit_timer->isScheduled(),
                            DS_WAIT_IDLE,
            );
//            FSMA_Transition(TRANSITION3,
//                            true,
//                            DS_WAIT_MAC,
//            );
            FSMA_Transition(TRANSITION4, // open arrow
                            plca_reset || !plca_en || !plca_status,
                            DS_IDLE,
            );
        }
        FSMA_State(DS_TRANSMIT) {
            FSMA_Enter(
                packetPending = false;
                CARRIER_STATUS = "CARRIER_ON";
//                    TXD =plca_txdn–a
                TX_EN = true;
                TX_ER = plca_txer;
                if (COL) {
                    SIGNAL_STATUS = "SIGNAL_ERROR";
                    a = 0;
                }
                else
                    SIGNAL_STATUS = "NO_SIGNAL_ERROR";
            );
            FSMA_Transition(TRANSITION1,
                            MCD && plca_txen,
                            DS_TRANSMIT,
            );
            FSMA_Transition(TRANSITION2,
                            MCD && !plca_txen && a > 0,
                            DS_FLUSH,
            );
            FSMA_Transition(TRANSITION3, // C
                            MCD && !plca_txen && a == 0,
                            DS_WAIT_IDLE,
            );
            FSMA_Transition(TRANSITION4, // open arrow
                            plca_reset || !plca_en || !plca_status,
                            DS_IDLE,
            );
        }
        FSMA_State(DS_FLUSH) {
            FSMA_Enter(
                CARRIER_STATUS = "CARRIER_ON";
//                    TXD = plca_txdn–a
                TX_EN = true;
                TX_ER = plca_txer;
                b = b + 1;
                if (COL)
                    SIGNAL_STATUS = "SIGNAL_ERROR";
                else
                    SIGNAL_STATUS = "NO_SIGNAL_ERROR";
            );
            FSMA_Transition(TRANSITION1,
                            MCD && a != b,
                            DS_FLUSH,
            );
            FSMA_Transition(TRANSITION2,
                            MCD && a == b,
                            DS_WAIT_IDLE, // C
            );
            FSMA_Transition(TRANSITION3, // open arrow
                            plca_reset || !plca_en || !plca_status,
                            DS_IDLE,
            );
        }
    }
}

void NewEthernetPlca::handleWithStatusFSM()
{
    // 148.4.6.5 State diagram of IEEE Std 802.3cg-2019
    FSMA_Switch(statusFsm) {
        FSMA_State(SS_INACTIVE) {
            FSMA_Enter(
                plca_status = false;
            );
            FSMA_Transition(TRANSITION1,
                            plca_active,
                            SS_ACTIVE,
            );
            FSMA_Transition(TRANSITION2, // open arrow
                            plca_reset || !plca_en,
                            SS_INACTIVE,
            );
        }
        FSMA_State(SS_ACTIVE) {
            FSMA_Enter(
                plca_status = true;
            );
            FSMA_Transition(TRANSITION1,
                            !plca_active,
                            SS_HYSTERESIS,
            );
            FSMA_Transition(TRANSITION2, // open arrow
                            plca_reset || !plca_en,
                            SS_INACTIVE,
            );
        }
        FSMA_State(SS_HYSTERESIS) {
            FSMA_Enter(
                scheduleAfter(130090 / 100E+6, plca_status_timer);
            );
            FSMA_Transition(TRANSITION1,
                            plca_active,
                            SS_ACTIVE,
            );
            FSMA_Transition(TRANSITION2,
                            !plca_status_timer->isScheduled() && !plca_active,
                            SS_INACTIVE,
            );
            FSMA_Transition(TRANSITION3, // open arrow
                            plca_reset || !plca_en,
                            SS_INACTIVE,
            );
        }
    }
}

void NewEthernetPlca::handleWithFSM()
{
    int controlState, dataState, statusState;
    do {
        controlState = controlFsm.getState();
        dataState = dataFsm.getState();
        statusState = statusFsm.getState();
        handleWithControlFSM();
        handleWithDataFSM();
        handleWithStatusFSM();
    }
    while (controlState != controlFsm.getState() || dataState != dataFsm.getState() || statusState != statusFsm.getState());
}

} // namespace physicallayer

} // namespace inet

