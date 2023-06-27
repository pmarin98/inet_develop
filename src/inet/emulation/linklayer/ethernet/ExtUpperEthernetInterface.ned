//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


package inet.emulation.linklayer.ethernet;

import inet.emulation.common.ExtInterface;
import inet.linklayer.contract.IEtherMac;
import inet.linklayer.contract.IEthernetInterface;
import inet.physicallayer.wired.ethernet.IEthernetPhy;
import inet.queueing.contract.IPacketQueue;


//
// This module provides an Ethernet network interface suitable for emulation.
// The upper part of the network interface is realized in the real world using
// a real TAP device of the host computer which is running the simulation.
//
module ExtUpperEthernetInterface extends ExtInterface like IEthernetInterface
{
    parameters:
        bool csmacdSupport = default(false);  // by default CSMA/CD is turned off, so only point-to-point duplex links are supported.
        double bitrate @unit(bps) = default(nan bps);
        @display("bgb=450,600");
    gates:
        input cutthroughIn @loose;
        output cutthroughOut @loose;
        inout phys @labels(EthernetSignal);
    submodules:
        tap: ExtEthernetTapDevice {
            parameters:
                @display("p=350,100");
        }
        queue: <default("EthernetQueue")> like IPacketQueue {
            parameters:
                packetCapacity = default(10000);
                @display("p=250,200");
        }
        mac: <default(csmacdSupport ? "EthernetCsmaMac" : "EthernetMac")> like IEtherMac {
            parameters:
                @display("p=350,300");
        }
        phy: <default("EthernetCsmaPhy")> like IEthernetPhy {
            parameters:
                @display("p=350,450");
        }
    connections allowunconnected:
        tap.lowerLayerOut --> queue.in;
        queue.out --> mac.upperLayerIn;
        tap.lowerLayerIn <-- mac.upperLayerOut;
        mac.lowerLayerOut --> phy.upperLayerIn;
        phy.upperLayerOut --> mac.lowerLayerIn;

        phy.phys <--> { @display("m=s"); } <--> phys;
}

