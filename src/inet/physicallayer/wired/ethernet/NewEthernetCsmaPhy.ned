//
// Copyright (C) 2023 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


package inet.physicallayer.wired.ethernet;

simple NewEthernetCsmaPhy
{
    parameters:
        @signal[receptionStarted](type=inet::physicallayer::EthernetSignalBase);
        @signal[receptionEnded](type=inet::physicallayer::EthernetSignalBase);
        @signal[transmissionStarted](type=inet::physicallayer::EthernetSignalBase);
        @signal[transmissionEnded](type=inet::physicallayer::EthernetSignalBase);
        @display("i=block/rxtx");
    gates:
        input upperLayerIn @labels(EtherFrame);
        output upperLayerOut @labels(EtherFrame);
        inout phys @labels(EthernetSignal);
}

