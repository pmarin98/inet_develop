//
// Copyright (C) 2023 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_INEWETHERNETCSMAPHY_H
#define __INET_INEWETHERNETCSMAPHY_H

#include "inet/physicallayer/wired/ethernet/EthernetSignal_m.h"

namespace inet {

namespace physicallayer {

class INET_API INewEthernetCsmaPhy
{
  public:
    virtual void startJamSignalTransmission() = 0; // MII TX_EN and TXD signals (tx_cmd JAM)
    virtual void startBeaconSignalTransmission() = 0; // MII TX_EN and TXD signals (tx_cmd BEACON)
    virtual void startCommitSignalTransmission() = 0; // MII TX_EN and TXD signals (tx_cmd COMMIT)
    virtual void endSignalTransmission() = 0; // MII TX_EN and TXD signals (tx_cmd NONE)

    virtual void startFrameTransmission(Packet *packet) = 0; // MII TX_EN and TXD signals
    virtual void endFrameTransmission() = 0; // MII TX_EN and TXD signals
};

} // namespace physicallayer

} // namespace inet

#endif

