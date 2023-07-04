//
// Copyright (C) 2023 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


package inet.physicallayer.wired.ethernet;

import inet.protocolelement.contract.IProtocolLayer;

simple NewEthernetPlca2 like IProtocolLayer
{
    parameters:
        int plca_node_count;
        int local_nodeID;
        double to_interval @unit(s);
        @signal[curID](type=int);
        @statistic[curID](record=vector; interpolationmode=sample-hold);
        @display("i=block/rxtx");
    gates:
        input upperLayerIn @labels(EtherFrame);
        output upperLayerOut @labels(EtherFrame);
        input lowerLayerIn @labels(EtherFrame);
        output lowerLayerOut @labels(EtherFrame);
}

