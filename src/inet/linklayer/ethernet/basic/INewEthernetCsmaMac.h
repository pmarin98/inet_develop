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
    virtual void handleCarrierSenseStart() = 0;
    virtual void handleCarrierSenseEnd() = 0;

    virtual void handleCollisionStart() = 0;
    virtual void handleCollisionEnd() = 0;

    virtual void handleTransmissionEnd() = 0;

    virtual void handleReceivedPacket(Packet *packet) = 0;
};

} // namespace inet

#endif

