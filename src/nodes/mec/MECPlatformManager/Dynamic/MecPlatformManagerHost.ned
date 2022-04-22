//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package simu5g.nodes.mec.MECPlatformManager.Dynamic;

import inet.linklayer.ppp.PppInterface;
import inet.networklayer.contract.INetworkLayer;
import inet.common.MessageDispatcher;
import inet.transportlayer.contract.ITcp;
import inet.transportlayer.contract.IUdp;
import inet.transportlayer.udp.Udp;
import inet.transportlayer.tcp.Tcp;
import inet.networklayer.common.InterfaceTable;
import inet.linklayer.contract.ILoopbackInterface;
import inet.linklayer.ethernet.EthernetInterface;

//
// @author Alessandro Calvio
// @author Angelo Feraudo
//
module MecPlatformManagerHost
{
    parameters:
        @networkNode();
        @display("i=device/device");
        bool hasIpv4 = default(true);
        bool hasIpv6 = default(false);

        *.interfaceTableModule = default(absPath(".interfaceTable"));

    gates:
        inout pppUPFg[];
        inout ethUPFg[];

    submodules:
        mepmApp: MecPlatformManagerDyn {

            @display("p=549.64,71.271996");
        }
        
        interfaceTable: InterfaceTable {
            @display("p=61.344,74.976;is=s");
        }

		//#
        //# Network layer module
        //#
        ipv4: <default("Ipv4NetworkLayer")> like INetworkLayer if hasIpv4 {
            parameters:
                @display("p=420.28,372.88;q=queue");
        }
        ipv6: <default("Ipv6NetworkLayer")> like INetworkLayer if hasIpv6 {
            parameters:
                @display("p=657.28,374.46;q=queue");
        }

        //#
        //# Transport layer modules and corresponding application modules
        //#
		// =============== UDP ============== //
        udp: Udp {
            @display("p=459,217");
        }

        tcp: Tcp {
            @display("p=627,217");
        }

        // message dispatcher for SAP between application and transport layer
        at: MessageDispatcher {
            parameters:
                @display("p=550,146;b=600,5,,,,1");
        }
        
        // message dispatcher for SAP between transport and network layer
        tn: MessageDispatcher {
            parameters:
                @display("p=550,296;b=600,5,,,,1");
        }
        // message dispatcher for SAP to link layer
        nl: MessageDispatcher {
            parameters:
                @display("p=550,446;b=600,5,,,,1");
        }
        
        //Interfaces
        pppIfUPF[sizeof(pppUPFg)]: PppInterface {

            @display("p=274.216,521.856;i=device/card");
        }
        
        eth[sizeof(ethUPFg)]: EthernetInterface {
            parameters:
                @display("p=900,676,row,150;q=txQueue");
        }
        
        lo: <default("LoopbackInterface")> like ILoopbackInterface {
            parameters:
                @display("p=750,676,row,150");
        }
    connections allowunconnected:

        mepmApp.socketOut --> at.in++;
        mepmApp.socketIn <-- at.out++;
		
		at.out++ --> udp.appIn;
        at.in++ <-- udp.appOut;

        at.out++ --> tcp.appIn;
        at.in++ <-- tcp.appOut;


        //# 
        //# Transport layer to network layer connections
        //#
        udp.ipOut --> tn.in++;
        udp.ipIn <-- tn.out++;

        tcp.ipOut --> tn.in++;
        tcp.ipIn <-- tn.out++;


        ipv4.ifIn <-- nl.out++ if hasIpv4;
        ipv4.ifOut --> nl.in++ if hasIpv4;

        ipv4.transportIn <-- tn.out++ if hasIpv4;
        ipv4.transportOut --> tn.in++ if hasIpv4;

        ipv6.ifIn <-- nl.out++ if hasIpv6;
        ipv6.ifOut --> nl.in++ if hasIpv6;

        ipv6.transportIn <-- tn.out++ if hasIpv6;
        ipv6.transportOut --> tn.in++ if hasIpv6;

        for i=0..sizeof(pppUPFg)-1 {
            pppIfUPF[i].upperLayerOut --> nl.in++;
            pppIfUPF[i].upperLayerIn <-- nl.out++;
            pppIfUPF[i].phys <--> pppUPFg++;
        }
        
        for i=0..sizeof(ethUPFg)-1 {
            eth[i].upperLayerOut --> nl.in++;
            eth[i].upperLayerIn <-- nl.out++;
            eth[i].phys <--> ethUPFg++;
        }
        
        // loopback
        lo.upperLayerOut --> nl.in++;
        lo.upperLayerIn <-- nl.out++;
        
}
