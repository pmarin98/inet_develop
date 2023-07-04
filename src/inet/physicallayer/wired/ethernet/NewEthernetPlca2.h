//
// Copyright (C) 2023 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_NEWETHERNETPLCA2_H
#define __INET_NEWETHERNETPLCA2_H

#include "inet/common/FSMA.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/ethernet/basic/INewEthernetCsmaMac.h"
#include "inet/physicallayer/wired/ethernet/EthernetSignal_m.h"
#include "inet/physicallayer/wired/ethernet/INewEthernetCsmaPhy.h"

namespace inet {

namespace physicallayer {

class INET_API NewEthernetPlca2 : public cSimpleModule, public virtual INewEthernetCsmaMac, public virtual INewEthernetCsmaPhy
{
  protected:
    static simsignal_t curIDSignal;

  protected:
    // parameters
    int plca_node_count = -1;
    simtime_t to_interval;

    // environment
    INewEthernetCsmaPhy *phy = nullptr;
    INewEthernetCsmaMac *mac = nullptr;

    // state
    int local_nodeID = -1;
    int curID = -1;
    bool phyCRS = false;
    bool COL = false;
    bool sendingBeacon = false;

    bool oldMacCRS = false;

    // timers
    cMessage *to_timer = nullptr;

    // statistics

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message) override;
    virtual void handleSelfMessage(cMessage *message);
    virtual void refreshDisplay() const override;

    virtual void handleCRSChanged();

  public:
    virtual ~NewEthernetPlca2();

    virtual void handleCarrierSenseStart() override;
    virtual void handleCarrierSenseEnd() override;

    virtual void handleCollisionStart() override;
    virtual void handleCollisionEnd() override;

    virtual void handleTransmissionEnd() override;

    virtual void handleReceivedPacket(Packet *packet) override;

    virtual void startJamSignalTransmission() override;
    virtual void startBeaconSignalTransmission() override { throw cRuntimeError("TODO"); }
    virtual void startCommitSignalTransmission() override { throw cRuntimeError("TODO"); }
    virtual void endSignalTransmission() override { throw cRuntimeError("TODO"); }

    virtual void startFrameTransmission(Packet *packet) override { throw cRuntimeError("TODO"); }
    virtual void endFrameTransmission() override { throw cRuntimeError("TODO"); }

    virtual EthernetSignalBase *getReceivedSignal() override;
};

} // namespace physicallayer

} // namespace inet

#endif

