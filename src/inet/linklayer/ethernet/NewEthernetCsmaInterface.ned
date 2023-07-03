//
// Copyright (C) 2023 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


package inet.linklayer.ethernet;

import inet.linklayer.contract.IEthernetInterface;
import inet.linklayer.ethernet.basic.NewEthernetCsmaMac;
import inet.networklayer.common.NetworkInterface;
import inet.physicallayer.wired.ethernet.NewEthernetCsmaPhy;
import inet.protocolelement.contract.IProtocolLayer;
import inet.queueing.contract.IPacketQueue;
import inet.queueing.contract.ITrafficConditioner;


//
// This module represents an Ethernet network interface.
//
module NewEthernetCsmaInterface extends NetworkInterface like IEthernetInterface
{
    parameters:
        string interfaceTableModule;
        string protocol = default("ethernetmac");
        string address @mutable = default("auto");   // MAC address as hex string (12 hex digits), or
                                                     // "auto". "auto" values will be replaced by
                                                     // a generated MAC address in init stage 0.
        bool duplexMode = default(true);      // selects full-duplex (true) or half-duplex (false) operation
        bool csmacdSupport = default(!duplexMode);  // by default CSMA/CD is turned off, so only point-to-point duplex links are supported.
        double bitrate @unit(bps) = default(0 bps);
        string fcsMode @enum("declared","computed") = default("declared");
        *.interfaceTableModule = default(absPath(this.interfaceTableModule));
        *.fcsMode = this.fcsMode;
        *.duplexMode = this.duplexMode;
    gates:
        input upperLayerIn;
        output upperLayerOut;
        input cutthroughIn @loose;
        output cutthroughOut @loose;
        inout phys @labels(EthernetSignal);
    submodules:
        egressTC: <default("")> like ITrafficConditioner {
            parameters:
                @display("p=300,100");
        }
        ingressTC: <default("")> like ITrafficConditioner {
            parameters:
                @display("i=block/uparrow;p=700,100");
        }
        queue: <default("EthernetQueue")> like IPacketQueue {
            parameters:
                packetCapacity = default(1000);
                @display("p=300,250");
        }
        mac: NewEthernetCsmaMac {
            parameters:
                @display("p=500,300");
        }
        plca: <default("")> like IProtocolLayer {
            parameters:
                @display("p=500,450");
        }
        phy: NewEthernetCsmaPhy {
            parameters:
                @display("p=500,600");
        }
    connections:
        upperLayerIn --> { @display("m=n"); } --> egressTC.in;
        egressTC.out --> queue.in;
        queue.out --> mac.upperLayerIn;

        mac.lowerLayerOut --> plca.upperLayerIn;
        plca.lowerLayerOut --> phy.upperLayerIn;

        phy.upperLayerOut --> plca.lowerLayerIn;
        plca.upperLayerOut --> mac.lowerLayerIn;

        phy.phys <--> { @display("m=s"); } <--> phys;

        mac.upperLayerOut --> ingressTC.in;
        ingressTC.out --> { @display("m=n"); } --> upperLayerOut;
}

