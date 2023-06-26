//
// Copyright (C) 2013 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_PHYPROTOCOLBASE_H
#define __INET_PHYPROTOCOLBASE_H

#include "inet/common/LayeredProtocolBase.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/NetworkInterface.h"
#include "inet/queueing/contract/IPacketQueue.h"

namespace inet {

namespace physicallayer {

class INET_API PhyProtocolBase : public LayeredProtocolBase, public cListener
{
  protected:
    /** @brief Gate ids */
    //@{
    int upperLayerInGateId = -1;
    int upperLayerOutGateId = -1;
    int lowerLayerInGateId = -1;
    int lowerLayerOutGateId = -1;
    //@}

    opp_component_ptr<NetworkInterface> networkInterface;

    opp_component_ptr<cModule> hostModule;

    /** Currently transmitted frame if any */
    Packet *currentTxFrame = nullptr;

  protected:
    PhyProtocolBase();
    virtual ~PhyProtocolBase();

    virtual void initialize(int stage) override;

    virtual void registerInterface();
    virtual void configureNetworkInterface() = 0;

    virtual MacAddress parseMacAddressParameter(const char *addrstr);

    virtual void deleteCurrentTxFrame();
    virtual void dropCurrentTxFrame(PacketDropDetails& details);

    virtual void handleMessageWhenDown(cMessage *msg) override;

    virtual void sendUp(cMessage *message);
    virtual void sendDown(cMessage *message);

    virtual bool isUpperMessage(cMessage *message) const override;
    virtual bool isLowerMessage(cMessage *message) const override;

    virtual bool isInitializeStage(int stage) const override { return stage == INITSTAGE_LINK_LAYER; }
    virtual bool isModuleStartStage(int stage) const override { return stage == ModuleStartOperation::STAGE_LINK_LAYER; }
    virtual bool isModuleStopStage(int stage) const override { return stage == ModuleStopOperation::STAGE_LINK_LAYER; }

    using cListener::receiveSignal;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;
    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;
};

} // namespace physicallayer

} // namespace inet

#endif

