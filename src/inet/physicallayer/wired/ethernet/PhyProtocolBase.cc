//
// Copyright (C) 2013 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "inet/physicallayer/wired/ethernet/PhyProtocolBase.h"

#include "inet/common/IInterfaceRegistrationListener.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"

namespace inet {

namespace physicallayer {

PhyProtocolBase::PhyProtocolBase()
{
}

PhyProtocolBase::~PhyProtocolBase()
{
    delete currentTxFrame;
}

MacAddress PhyProtocolBase::parseMacAddressParameter(const char *addrstr)
{
    MacAddress address;

    if (!strcmp(addrstr, "auto"))
        // assign automatic address
        address = MacAddress::generateAutoAddress();
    else
        address.setAddress(addrstr);

    return address;
}

void PhyProtocolBase::initialize(int stage)
{
    LayeredProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        upperLayerInGateId = findGate("upperLayerIn");
        upperLayerOutGateId = findGate("upperLayerOut");
        lowerLayerInGateId = findGate("lowerLayerIn");
        lowerLayerOutGateId = findGate("lowerLayerOut");
        currentTxFrame = nullptr;
        hostModule = findContainingNode(this);
    }
    else if (stage == INITSTAGE_NETWORK_INTERFACE_CONFIGURATION)
        registerInterface();
}

void PhyProtocolBase::registerInterface()
{
    ASSERT(networkInterface == nullptr);
    networkInterface = getContainingNicModule(this);
    configureNetworkInterface();
}

void PhyProtocolBase::sendUp(cMessage *message)
{
    if (message->isPacket())
        emit(packetSentToUpperSignal, message);
    send(message, upperLayerOutGateId);
}

void PhyProtocolBase::sendDown(cMessage *message)
{
    if (message->isPacket())
        emit(packetSentToLowerSignal, message);
    send(message, lowerLayerOutGateId);
}

bool PhyProtocolBase::isUpperMessage(cMessage *message) const
{
    return message->getArrivalGateId() == upperLayerInGateId;
}

bool PhyProtocolBase::isLowerMessage(cMessage *message) const
{
    return message->getArrivalGateId() == lowerLayerInGateId;
}

void PhyProtocolBase::handleMessageWhenDown(cMessage *msg)
{
    if (!msg->isSelfMessage() && msg->getArrivalGateId() == lowerLayerInGateId) {
        EV << "Interface is turned off, dropping packet\n";
        delete msg;
    }
    else
        LayeredProtocolBase::handleMessageWhenDown(msg);
}

void PhyProtocolBase::deleteCurrentTxFrame()
{
    delete currentTxFrame;
    currentTxFrame = nullptr;
}

void PhyProtocolBase::dropCurrentTxFrame(PacketDropDetails& details)
{
    emit(packetDroppedSignal, currentTxFrame, &details);
    delete currentTxFrame;
    currentTxFrame = nullptr;
}

void PhyProtocolBase::flushQueue(PacketDropDetails& details)
{
    // code would look slightly nicer with a pop() function that returns nullptr if empty
    if (txQueue)
        while (canDequeuePacket()) {
            auto packet = dequeuePacket();
            emit(packetDroppedSignal, packet, &details); // FIXME this signal lumps together packets from the network and packets from higher layers! separate them
            delete packet;
        }
}

void PhyProtocolBase::clearQueue()
{
    if (txQueue)
        while (canDequeuePacket())
            delete dequeuePacket();
}

void PhyProtocolBase::handleStartOperation(LifecycleOperation *operation)
{
    networkInterface->setState(NetworkInterface::State::UP);
    networkInterface->setCarrier(true);
}

void PhyProtocolBase::handleStopOperation(LifecycleOperation *operation)
{
    PacketDropDetails details;
    details.setReason(INTERFACE_DOWN);
    if (currentTxFrame)
        dropCurrentTxFrame(details);
    flushQueue(details);

    networkInterface->setCarrier(false);
    networkInterface->setState(NetworkInterface::State::DOWN);
}

void PhyProtocolBase::handleCrashOperation(LifecycleOperation *operation)
{
    deleteCurrentTxFrame();
    clearQueue();

    networkInterface->setCarrier(false);
    networkInterface->setState(NetworkInterface::State::DOWN);
}

void PhyProtocolBase::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    Enter_Method("%s", cComponent::getSignalName(signalID));
}


queueing::IPacketQueue *PhyProtocolBase::getQueue(cGate *gate) const
{
    // TODO use findConnectedModule() when the function is updated
    for (auto g = gate->getPreviousGate(); g != nullptr; g = g->getPreviousGate()) {
        if (g->getType() == cGate::OUTPUT) {
            auto m = dynamic_cast<queueing::IPacketQueue *>(g->getOwnerModule());
            if (m)
                return m;
        }
    }
    throw cRuntimeError("Gate %s is not connected to a module of type queueing::IPacketQueue (did you use OmittedPacketQueue as queue type?)", gate->getFullPath().c_str());
}

bool PhyProtocolBase::canDequeuePacket() const
{
    return txQueue && txQueue->canPullSomePacket(gate(upperLayerInGateId)->getPathStartGate());
}

Packet *PhyProtocolBase::dequeuePacket()
{
    Packet *packet = txQueue->dequeuePacket();
    take(packet);
    packet->setArrival(getId(), upperLayerInGateId, simTime());
    emit(packetReceivedFromUpperSignal, packet);
    return packet;
}

} // namespace physicallayer

} // namespace inet

