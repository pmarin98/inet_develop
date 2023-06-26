//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


package inet.physicallayer.wired.ethernet;

moduleinterface IEthernetPhy
{
    parameters:
        @display("i=block/rxtx");
    gates:
        input upperLayerIn;
        output upperLayerOut;
        inout phys;
}
