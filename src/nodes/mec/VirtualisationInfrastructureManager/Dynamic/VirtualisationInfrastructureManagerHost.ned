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

package simu5g.nodes.mec.VirtualisationInfrastructureManager.Dynamic;
import simu5g.nodes.mec.VirtualisationInfrastructureManager.Dynamic.VirtualisationInfrastructureManagerDyn;
import simu5g.nodes.NR.NRUe;
import inet.common.MessageDispatcher;
import inet.linklayer.contract.ILoopbackInterface;
import inet.transportlayer.contract.ITcp;
import inet.transportlayer.contract.IUdp;
import inet.linklayer.contract.IEthernetInterface;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.Ipv4RoutingTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.transportlayer.udp.Udp;
import inet.transportlayer.tcp.Tcp;
import simu5g.stack.ICellularNic;
//
// TODO auto-generated type
//
//extends NRUE
module VirtualisationInfrastructureManagerHost extends NRUe
{
    parameters:
        @networkNode();

//        string nicType = default("NRNicUe"); // do not change
//        string nodeType = "UE";  // DO NOT CHANGE
//        
//        //# 5G Parameters
//        int nrMasterId = default(0);
//        int nrMacNodeId = default(0);
//        int nrMacCellId = default(0);

        //# Loopback interfaces
//        int numLoInterfaces = default(1);

        string networkLayerType = default("IPv4NetworkLayer");
        *.interfaceTableModule = default(absPath(".interfaceTable"));

        //# Transport Layer specs
//        bool hasTcp = default(true);
//        bool hasUdp = default(true);

        //# Network Layer specs
        //        bool hasIpv4 = default(true);
        //        bool hasIpv6 = default(false);
        *.routingTableModule = default(absPath(".ipv4.routingTable"));
        *.interfaceTableModule = default(absPath(".interfaceTable"));

        //# Resource of the VimHost
        double localRam @unit("B") = default(32GB);
        double localDisk @unit("B") = default(100TB);
        double localCpuSpeed = default(300000);

        //# App parameters
//        numApps = 1;
//        app[0].typename = "VirtualisationInfrastructureManagerDyn";

        @display("i=device/device");
    gates:
        inout mm7GnB;
//        input mm7d2d @directIn;

		// to gnodes
        inout pppENB[];
        
        // to server
        inout ethiface[];

    submodules:

//         transport layer 
//        udp: <default(firstAvailableOrEmpty("Udp"))> like IUdp if hasUdp {
//            parameters:
//                @display("p=375,226");
//        }
//        tcp: <default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc"))> like ITcp if hasTcp {
//            parameters:
//                @display("p=676.5,225.09");
//        }
//
//        //# network layer
//        ipv4: <default("Ipv4NetworkLayer")> like INetworkLayer if hasIpv4 {
//            parameters:
//                @display("p=389.91,375.15;q=queue");
//        }
//        ipv6: <default("Ipv6NetworkLayer")> like INetworkLayer if hasIpv6 {
//            parameters:
//                @display("p=709.71,375.15;q=queue");
//        }
//
//        //# Link layer
//        lo[numLoInterfaces]: <default("LoopbackInterface")> like ILoopbackInterface {
//            parameters:
//                @display("p=373.92,521.52,row,150");
//        }

       	 // ethernet interface used for communicating with Virtualisation Infrastructure
        mm7Eth: <default("EthernetInterface")> like IEthernetInterface {
            parameters:
                @display("p=813,523,row,150;q=txQueue");
        }

//        cellularNic: <nicType> like ICellularNic {
//            nodeType = nodeType;
//            @display("p=246,530;i=block/ifcard");
//        }
//
//        //# message dispatcher for SAP between application and transport layer
//        at: MessageDispatcher {
//            parameters:
//                @display("p=550,146;b=600,5,,,,1");
//        }
//        //# message dispatcher for SAP between transport and network layer
//        tn: MessageDispatcher {
//            parameters:
//                @display("p=550,296;b=600,5,,,,1");
//        }
//        //# message dispatcher for SAP to link layer
//        nl: MessageDispatcher {
//            parameters:
//                @display("p=550,446;b=600,5,,,,1");
//        }

    connections allowunconnected:

        //        //# Virtualisation infrastructure manager logic binding  
        //        vim.socketIn <-- at.out++;
        //        vim.socketOut --> at.in++;
        //
        //
        //        at.out++ --> udp.appIn if hasUdp;
        //        at.in++ <-- udp.appOut if hasUdp;
        //
        //        at.out++ --> tcp.appIn if hasTcp;
        //        at.in++ <-- tcp.appOut if hasTcp;
        //
        //        //# 
        //		//# Internal transport layer connections to network layer
        //		//# 
        //
        //        udp.ipOut --> tn.in++ if hasUdp;
        //        udp.ipIn <-- tn.out++ if hasUdp;
        //
        //        tcp.ipOut --> tn.in++ if hasTcp;
        //        tcp.ipIn <-- tn.out++ if hasTcp;
        //
        //        ipv4.ifIn <-- nl.out++ if hasIpv4;
        //        ipv4.ifOut --> nl.in++ if hasIpv4;
        //
        //        ipv4.transportIn <-- tn.out++ if hasIpv4;
        //        ipv4.transportOut --> tn.in++ if hasIpv4;
        //
        //        ipv6.ifIn <-- nl.out++ if hasIpv6;
        //        ipv6.ifOut --> nl.in++ if hasIpv6;
        //
        //        ipv6.transportIn <-- tn.out++ if hasIpv6;
        //        ipv6.transportOut --> tn.in++ if hasIpv6;
        //
        //        tn.out++ --> nl.in++;
        //        tn.in++ <-- nl.out++;
        //
        //        //# Phy layer
        nl.out++ --> mm7Eth.upperLayerIn;
        nl.in++ <-- mm7Eth.upperLayerOut;
//        cellularNic.upperLayerOut --> nl.in++;
//        cellularNic.upperLayerIn <-- nl.out++;

//        mm7Eth.phys <--> mm7GnB;
        //pppENB++ <--> mm7Eth.phys;
		ethiface++ <--> mm7Eth.phys;
//        cellularNic.radioIn <-- mm7d2d;


}
