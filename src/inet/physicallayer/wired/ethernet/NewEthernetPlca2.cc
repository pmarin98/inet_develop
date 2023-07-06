//
// Copyright (C) 2023 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "inet/physicallayer/wired/ethernet/NewEthernetPlca2.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/Simsignals.h"
#include "inet/physicallayer/wired/ethernet/EthernetPhyHeader_m.h"

namespace inet {

namespace physicallayer {

simsignal_t NewEthernetPlca2::curIDSignal = cComponent::registerSignal("curID");

Define_Module(NewEthernetPlca2);

NewEthernetPlca2::~NewEthernetPlca2()
{
    cancelAndDelete(to_timer);
}

void NewEthernetPlca2::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        plca_node_count = par("plca_node_count");
        local_nodeID = par("local_nodeID");
        to_interval = par("to_interval");
        phy = getConnectedModule<INewEthernetCsmaPhy>(gate("lowerLayerOut"));
        mac = getConnectedModule<INewEthernetCsmaMac>(gate("upperLayerOut"));
        to_timer = new cMessage("to_timer");
    }
    else if (stage == INITSTAGE_PHYSICAL_LAYER) {
        if (local_nodeID == 0) {
            sendingBeacon = true;
            phy->startSignalTransmission(BEACON);
        }
    }
}

void NewEthernetPlca2::refreshDisplay() const
{
    auto& displayString = getDisplayString();
    std::stringstream stream;
    stream << curID << "/" << plca_node_count << " (" << local_nodeID << ")\n";
    displayString.setTagArg("t", 0, stream.str().c_str());
}

void NewEthernetPlca2::handleMessage(cMessage *message)
{
    if (message->isSelfMessage())
        handleSelfMessage(message);
    else if (message->getArrivalGate() == gate("upperLayerIn")) {
        cancelEvent(to_timer);
        send(message, "lowerLayerOut");
    }
    // TODO remove, not need because state machine handles all events
    else if (message->getArrivalGate() == gate("lowerLayerIn"))
        ; // send(message, "upperLayerOut");
    else
        throw cRuntimeError("Unknown message");
}

void NewEthernetPlca2::handleSelfMessage(cMessage *message)
{
    EV_DEBUG << "Handling self message" << EV_FIELD(message) << EV_ENDL;
    if (message == to_timer) {
        if (curID == plca_node_count - 1) {
            curID = -1;
            emit(curIDSignal, curID);
            if (local_nodeID == 0) {
                sendingBeacon = true;
                phy->startSignalTransmission(BEACON);
                // TODO phy->endSignalTransmission();
            }
        }
        else {
            curID++;
            emit(curIDSignal, curID);
            scheduleAfter(to_interval, to_timer);
        }
        handleCRSChanged();
    }
    else
        throw cRuntimeError("Unknown message");
}

void NewEthernetPlca2::handleCarrierSenseStart()
{
    Enter_Method("handleCarrierSenseStart");
    ASSERT(!phyCRS);
    phyCRS = true;
    EV_DEBUG << "Handling carrier sense start" << EV_FIELD(phyCRS) << EV_ENDL;
    cancelEvent(to_timer);
    handleCRSChanged();
}

void NewEthernetPlca2::handleCarrierSenseEnd()
{
    Enter_Method("handleCarrierSenseEnd");
    ASSERT(phyCRS);
    phyCRS = false;
    EV_DEBUG << "Handling carrier sense end" << EV_FIELD(phyCRS) << EV_ENDL;
    scheduleAfter(to_interval, to_timer);
    handleCRSChanged();
}

void NewEthernetPlca2::handleCollisionStart()
{
    Enter_Method("handleCollisionStart");
    ASSERT(!COL);
    COL = true;
    EV_DEBUG << "Handling collision start" << EV_FIELD(phyCRS) << EV_ENDL;
    throw cRuntimeError("TODO");
}

void NewEthernetPlca2::handleCollisionEnd()
{
    Enter_Method("handleCollisionEnd");
    ASSERT(COL);
    COL = false;
    EV_DEBUG << "Handling collision end" << EV_FIELD(phyCRS) << EV_ENDL;
    throw cRuntimeError("TODO");
}

//void NewEthernetPlca2::handleTransmissionEnd(SignalType signalType, Packet *packet)
//{
//    Enter_Method("handleTransmissionEnd");
//    EV_DEBUG << "Handling transmission end" << EV_ENDL;
//    if (sendingBeacon) {
//        sendingBeacon = false;
//        curID = 0;
//        emit(curIDSignal, curID);
//        handleCRSChanged();
//    }
//    else
//        mac->handleTransmissionEnd(signalType, packet);
//    scheduleAfter(to_interval, to_timer);
//}

void NewEthernetPlca2::handleReceptionStart(SignalType signalType, Packet *packet)
{
    Enter_Method("handleReceptionStart");
}

void NewEthernetPlca2::handleReceptionEnd(SignalType signalType, Packet *packet)
{
    Enter_Method("handleReceptionEnd");
    EV_DEBUG << "Handling received packet" << EV_FIELD(packet) << EV_ENDL;
    if (!strcmp(packet->getName(), "PLCA_BEACON")) {
        curID = 0;
        emit(curIDSignal, curID);
        handleCRSChanged();
    }
    else
        mac->handleReceptionEnd(signalType, packet);
}

void NewEthernetPlca2::startFrameTransmission(Packet *packet)
{
    phy->startFrameTransmission(packet);
}

void NewEthernetPlca2::endFrameTransmission()
{
    phy->endFrameTransmission();
}

void NewEthernetPlca2::handleCRSChanged()
{
    bool virtualCRS = local_nodeID != curID;
    bool macCRS = phyCRS | virtualCRS;
    if (macCRS != oldMacCRS) {
        if (macCRS)
            mac->handleCarrierSenseStart();
        else
            mac->handleCarrierSenseEnd();
        oldMacCRS = macCRS;
    }
}

} // namespace physicallayer

} // namespace inet

