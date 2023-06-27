//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "inet/physicallayer/wired/ethernet/EthernetCsmaPhy.h"

#include "inet/common/ProtocolTag_m.h"
#include "inet/common/packet/Packet.h"
#include "inet/physicallayer/wired/ethernet/EthernetPhyHeader_m.h"
#include "inet/physicallayer/wired/ethernet/EthernetSignal_m.h"
#include "inet/linklayer/ethernet/basic/EthernetCsmaMac.h"

namespace inet {

namespace physicallayer {

Define_Module(EthernetCsmaPhy);

void EthernetCsmaPhy::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        physInGate = gate("phys$i");
        upperLayerInGate = gate("upperLayerIn");
        setTxUpdateSupport(true);
    }
}

void EthernetCsmaPhy::handleMessage(cMessage *message)
{
    if (message->getArrivalGate() == upperLayerInGate)
        throw cRuntimeError("TODO");
    else if (message->getArrivalGate() == physInGate) {
        auto mac = check_and_cast<EthernetCsmaMac *>(getParentModule()->getSubmodule("mac"));
        mac->handleMessageWhenUp(message);
    }
    else
        throw cRuntimeError("Received unknown message");
}

} // namespace physicallayer

} // namespace inet

