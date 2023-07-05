//
// Copyright (C) 2023 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_NEWETHERNETCSMAPHY_H
#define __INET_NEWETHERNETCSMAPHY_H

#include "inet/common/FSMA.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/ethernet/basic/INewEthernetCsmaMac.h"
#include "inet/physicallayer/wired/ethernet/EthernetSignal_m.h"
#include "inet/physicallayer/wired/ethernet/INewEthernetCsmaPhy.h"

namespace inet {

namespace physicallayer {

class INET_API NewEthernetCsmaPhy : public cSimpleModule, public virtual INewEthernetCsmaPhy
{
  protected:
    enum State {
        IDLE,         // neither transmitting nor receiving any signal
        TRANSMITTING,
        RECEIVING,    // receiving one or more signals, not necessary successfully
        COLLISION,    // both transmitting and receiving simultaneously up to the point where both end
    };

    enum Event {
        TX_START,
        TX_END,
        RX_START,     // start of an Ethernet signal
        RX_UPDATE,    // update for a received Ethernet signal
        RX_CHANNEL_IDLE, // end of the last overlapping Ethernet signal
    };

    class INET_API RxSignal {
      public:
        EthernetSignalBase *signal = nullptr;
        simtime_t endRxTime;
        RxSignal(EthernetSignalBase *signal, simtime_t_cref endRxTime) : signal(signal), endRxTime(endRxTime) {}
    };

  protected:
    // parameters

    // environment
    cGate *physInGate = nullptr;
    cGate *physOutGate = nullptr;
    cGate *upperLayerInGate = nullptr;
    cGate *upperLayerOutGate = nullptr;
    INewEthernetCsmaMac *mac = nullptr;

    // state
    cFSM fsm;
    std::vector<RxSignal> rxSignals;
    EthernetSignalBase *currentTxSignal = nullptr;

    // timers
    cMessage *txTimer = nullptr;
    cMessage *rxChannelIdleTimer = nullptr;

    // statistics

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

    virtual void handleMessage(cMessage *message) override;
    virtual void handleSelfMessage(cMessage *message);
    virtual void handleEthernetSignal(EthernetSignalBase *message);
    virtual void handleUpperPacket(Packet *packet);

    virtual void handleWithFsm(int event, cMessage *message);

    virtual void startTransmit(EthernetSignalBase *signal);
    virtual void endTransmit();

    virtual void startReceive(EthernetSignalBase *signal);
    virtual void endReceive();

    virtual void encapsulate(Packet *packet);
    virtual void decapsulate(Packet *packet);

    virtual void cutEthernetSignalEnd(EthernetSignalBase *signal, simtime_t duration);
    virtual void updateRxSignals(EthernetSignalBase *signal);
    virtual void scheduleTxTimer(EthernetSignalBase *signal);

    virtual void startJamSignalTransmission();
    virtual void startBeaconSignalTransmission();
    virtual void startCommitSignalTransmission();

    virtual EthernetSignalBase *getReceivedSignal();

  public:
    virtual ~NewEthernetCsmaPhy();

    virtual void startSignalTransmission(SignalType signalType) override;
    virtual void endSignalTransmission() override;

    virtual void startFrameTransmission(Packet *packet) override;
    virtual void endFrameTransmission() override;
};

} // namespace physicallayer

} // namespace inet

#endif

