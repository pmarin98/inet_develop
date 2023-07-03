//
// Copyright (C) 2023 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


package inet.linklayer.ethernet.basic;

import inet.linklayer.base.MacProtocolBase;

simple NewEthernetCsmaMac extends MacProtocolBase
{
    parameters:
        bool sendRawBytes = default(false); // when true packets are serialized into a sequence of bytes before sending out
        bool promiscuous = default(false);  // if true, all packets are received, otherwise only the
                                            // ones with matching destination MAC address
        bool frameBursting = default(true); // enable/disable frame bursting mode in Gigabit Ethernet
        int mtu @unit(B) = default(1500B);
        string fcsMode @enum("declared","computed");
        @class(NewEthernetCsmaMac);
}

