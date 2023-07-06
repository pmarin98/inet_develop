//
// Copyright (C) 2023 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#ifndef __INET_INEWETHERNETCSMAMAC_H
#define __INET_INEWETHERNETCSMAMAC_H

#include "inet/physicallayer/wired/ethernet/EthernetSignal_m.h"

namespace inet {

using namespace inet::physicallayer;

class INET_API INewEthernetCsmaMac
{
  public:
    // IEEE 802.3-2018 22.2.2.11 CRS (carrier sense)
    virtual void handleCarrierSenseStart() = 0; // MII CRS signal
    virtual void handleCarrierSenseEnd() = 0; // MII CRS signal

    // IEEE 802.3-2018 22.2.2.12 COL (collision detected)
    virtual void handleCollisionStart() = 0; // MII COL signal
    virtual void handleCollisionEnd() = 0; // MII COL signal

//    virtual void handleTransmissionStart(SignalType signalType, Packet *packet) = 0;
//    virtual void handleTransmissionEnd(SignalType signalType, Packet *packet) = 0;

    virtual void handleReceptionStart(SignalType signalType, Packet *packet) = 0; // MII RX_DV and RXD signals (rx_cmd)
    virtual void handleReceptionEnd(SignalType signalType, Packet *packet) = 0; // MII RX_DV and RXD signals (rx_cmd)
};

} // namespace inet

#endif

