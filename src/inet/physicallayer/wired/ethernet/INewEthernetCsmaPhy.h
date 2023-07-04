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
    virtual void startJamSignalTransmission() = 0;
    virtual void startBeaconSignalTransmission() = 0;
    virtual void startCommitSignalTransmission() = 0;
    virtual void endSignalTransmission() = 0;

    virtual void startFrameTransmission(Packet *packet) = 0;
    virtual void endFrameTransmission() = 0;

    virtual EthernetSignalBase *getReceivedSignal() = 0;
};

} // namespace physicallayer

} // namespace inet

#endif

