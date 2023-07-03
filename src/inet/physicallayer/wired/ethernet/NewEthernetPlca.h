//
// Copyright (C) 2023 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_NEWETHERNETPLCA_H
#define __INET_NEWETHERNETPLCA_H

#include "inet/common/FSMA.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/ethernet/basic/INewEthernetCsmaMac.h"
#include "inet/physicallayer/wired/ethernet/EthernetSignal_m.h"
#include "inet/physicallayer/wired/ethernet/INewEthernetCsmaPhy.h"

namespace inet {

namespace physicallayer {

class INET_API NewEthernetPlca : public cSimpleModule, public virtual INewEthernetCsmaMac, public virtual INewEthernetCsmaPhy
{
  protected:
    // 148.4.4.6 State diagram of IEEE Std 802.3cg-2019
    enum ControlState {
        CS_DISABLE,
        CS_RESYNC,
        CS_RECOVER,
        CS_SEND_BEACON,
        CS_SYNCING,
        CS_WAIT_TO,
        CS_EARLY_RECEIVE,
        CS_COMMIT,
        CS_YIELD,
        CS_RECEIVE,
        CS_TRANSMIT,
        CS_BURST,
        CS_ABORT,
        CS_NEXT_TX_OPPORTUNITY,
    };

    enum DataState {
        DS_NORMAL,
        DS_WAIT_IDLE,
        DS_IDLE,
        DS_RECEIVE,
        DS_HOLD,
        DS_ABORT,
        DS_COLLIDE,
        DS_DELAY_PENDING,
        DS_PENDING,
        DS_WAIT_MAC,
        DS_TRANSMIT,
        DS_FLUSH,
    };

    // 148.4.6.5 State diagram of IEEE Std 802.3cg-2019
    enum StatusState {
        SS_INACTIVE,
        SS_ACTIVE,
        SS_HYSTERESIS,
    };

    enum Event {
        BEACON_TIMER_DONE,
        BEACON_DET_TIMER_DONE,
        INVALID_BEACON_TIMER_DONE,
        BURST_TIMER_DONE,
        TO_TIMER_DONE,
        PENDING_TIMER_DONE,
        COMMIT_TIMER_DONE,
        PLCA_STATUS_TIMER_DONE,
        CRS_CHANGED,
    };

  protected:
    // parameters
    bool plca_en = false;
    int plca_node_count = -1;
    int local_nodeID = -1;
    int max_bc = -1;
    simtime_t to_interval;
    simtime_t burst_interval;

    // environment
    INewEthernetCsmaPhy *phy = nullptr;
    INewEthernetCsmaMac *mac = nullptr;

    // state
    cFSM controlFsm;
    cFSM dataFsm;
    cFSM statusFsm;

    // 148.4.4.2 PLCA Control variables and 148.4.5.2 Variables and 148.4.6.2 PLCA Status variables
    bool COL = false;
    bool committed = false;
    bool CRS = false;
    bool MCD = true;
    bool packetPending = false;
    bool plca_active = false;
    bool plca_reset = false;
    bool plca_status = true;
    bool plca_txen = false;
    bool plca_txer = false;
    bool PMCD = true;
    bool receiving = false;
    bool TX_EN = false;
    bool TX_ER = false;
    int a = -1;
    int b = -1;
    int bc = 0;
    int curID = -1;
    int delay_line_length = -1;
    std::string CARRIER_STATUS;
    std::string rx_cmd; // possible values "NONE", "BEACON", "COMMIT"
    std::string SIGNAL_STATUS;
    std::string tx_cmd; // possible values "NONE", "BEACON", "COMMIT"

    // control timers
    cMessage *beacon_timer = nullptr;
    cMessage *beacon_det_timer = nullptr;
    cMessage *invalid_beacon_timer = nullptr;
    cMessage *burst_timer = nullptr;
    cMessage *to_timer = nullptr;

    // data timers
    cMessage *pending_timer = nullptr;
    cMessage *commit_timer = nullptr;

    // status timers
    cMessage *plca_status_timer = nullptr;

    // statistics

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void refreshDisplay() const override;

    virtual void handleMessage(cMessage *message) override;
    virtual void handleSelfMessage(cMessage *message);

    virtual void handleWithControlFSM();
    virtual void handleWithDataFSM();
    virtual void handleWithStatusFSM();

    virtual void handleWithFSM();

  public:
    virtual ~NewEthernetPlca();

    virtual void handleCarrierSenseStart() override;
    virtual void handleCarrierSenseEnd() override;

    virtual void handleCollisionStart() override;
    virtual void handleCollisionEnd() override;

    virtual void handleTransmissionEnd() override;

    virtual void handleReceivedPacket(Packet *packet) override;

    virtual void transmitJamSignal() override;
    virtual void transmitPlcaBeacon() override;

    virtual EthernetSignalBase *getReceivedSignal() override;
};

} // namespace physicallayer

} // namespace inet

#endif

