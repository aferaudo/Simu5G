package simu5g.nodes.mec.Federator;

import inet.linklayer.ppp.PppInterface;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.Ipv4RoutingTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.applications.contract.IApp;
import inet.transportlayer.udp.Udp;
import inet.transportlayer.tcp.Tcp;
import inet.common.MessageDispatcher;
import inet.linklayer.ethernet.EthernetInterface;
import inet.linklayer.contract.IEthernetInterface;
import inet.node.ethernet.Eth10G;
import inet.linklayer.contract.ILoopbackInterface;

module FederatorBroker
{
    parameters:
        @networkNode();
		string name = default("");
		
		bool hasIpv4 = default(true);
        bool hasIpv6 = default(false);
        int numExtEthInterfaces = default(0); 
        
        *.interfaceTableModule = default(absPath(".interfaceTable"));

    gates:
		inout pppg[];
        inout ethg[];

    submodules:
        
        federatorApp: FederatorApp {
            @display("p=450,100,row");
        }
        
        federatorBrokerApp: FederatorBrokerApp {
            @display("p=650,100,row");
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
        
        // Interfaces
        pppIf[sizeof(pppg)]: PppInterface {
            @display("p=446.832,506.688");
        }

        // ethernet interface used for communicating with external applications in emulation mode
        extEth[numExtEthInterfaces]: <default("ExtLowerEthernetInterface")> like IEthernetInterface {
            parameters:
                @display("p=641.712,506.688,row,150;q=txQueue");
        }
        
        eth[sizeof(ethg)]: EthernetInterface {
            parameters:
                @display("p=900,676,row,150;q=txQueue");
        }
        
        lo: <default("LoopbackInterface")> like ILoopbackInterface {
            parameters:
                @display("p=750,676,row,150");
        }
        

    connections allowunconnected:
        
        //# 
        //# MECOrchestrator to transport layer connections
        //#        
        federatorApp.socketOut --> at.in++;
        federatorApp.socketIn <-- at.out++;
        
        federatorBrokerApp.socketOut --> at.in++;
        federatorBrokerApp.socketIn <-- at.out++;

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
        
        for i=0..sizeof(pppg)-1 {
            pppIf[i].upperLayerOut --> nl.in++;
            pppIf[i].upperLayerIn <-- nl.out++;
            pppIf[i].phys <--> pppg++;
        }
        
        for i=0..sizeof(ethg)-1 {
            eth[i].upperLayerOut --> nl.in++;
            eth[i].upperLayerIn <-- nl.out++;
            eth[i].phys <--> ethg++;
        }

        for i=0..numExtEthInterfaces-1 {
            extEth[i].upperLayerOut --> nl.in++;
            extEth[i].upperLayerIn <-- nl.out++;
        }
        
        // loopback
        lo.upperLayerOut --> nl.in++;
        lo.upperLayerIn <-- nl.out++;
        

}
