//
// Copyright (C) 2023 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


package inet.physicallayer.wired.ethernet;

import inet.protocolelement.contract.IProtocolLayer;

simple NewEthernetPlca like IProtocolLayer
{
    parameters:
        bool plca_en = default(true);
        int plca_node_count;
        int local_nodeID;
        int max_bc = default(10);
        double to_interval @unit(s);
        double burst_interval @unit(s);
        @display("i=block/rxtx");
    gates:
        input upperLayerIn @labels(EtherFrame);
        output upperLayerOut @labels(EtherFrame);
        input lowerLayerIn @labels(EtherFrame);
        output lowerLayerOut @labels(EtherFrame);
}

