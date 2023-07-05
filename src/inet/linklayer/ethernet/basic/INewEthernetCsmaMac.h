//
// Copyright (C) 2023 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#ifndef __INET_INEWETHERNETCSMAMAC_H
#define __INET_INEWETHERNETCSMAMAC_H

#include "inet/common/packet/Packet.h"

namespace inet {

class INET_API INewEthernetCsmaMac
{
  public:
    virtual void handleCarrierSenseStart() = 0; // MII CRS signal
    virtual void handleCarrierSenseEnd() = 0; // MII CRS signal

    virtual void handleCollisionStart() = 0; // MII COL signal
    virtual void handleCollisionEnd() = 0; // MII COL signal

    virtual void handleTransmissionStart(int signalType, Packet *packet) = 0;
    virtual void handleTransmissionEnd(int signalType, Packet *packet) = 0;

    virtual void handleReceptionStart(int signalType, Packet *packet) = 0; // MII RX_DV and RXD signals (rx_cmd)
    virtual void handleReceptionEnd(int signalType, Packet *packet) = 0; // MII RX_DV and RXD signals (rx_cmd)
};

} // namespace inet

#endif

