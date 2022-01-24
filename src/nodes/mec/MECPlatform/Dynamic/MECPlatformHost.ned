//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//


package simu5g.nodes.mec.MECPlatform.Dynamic;

import inet.common.MessageDispatcher;
import inet.linklayer.contract.ILoopbackInterface;
import inet.transportlayer.contract.ITcp;
import inet.transportlayer.contract.IUdp;
import inet.linklayer.contract.IEthernetInterface;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.Ipv4RoutingTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.applications.contract.IApp;
import inet.transportlayer.udp.Udp;
import inet.transportlayer.tcp.Tcp;

import simu5g.nodes.mec.MECPlatform.IMEService;
import simu5g.nodes.mec.MECPlatform.ServiceRegistry.ServiceRegistry;
import simu5g.nodes.NR.NRUe;

// 
//# MEC platform compuond module representing the MEC platform entity 
// of the ETSI MEC architecture described in ETSI GS MEC 003
//

module MECPlatformHost extends NRUe
{
    parameters:
        @networkNode();
        @display("bgb=1122.99,610.08;i=block/cogwheel");

        //# Network Layer specs
        string networkLayerType = default("IPv4NetworkLayer");
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        
        //# Num Services
        int numOmnetServices = default(0);
        int numMecServices = default(0);

        //# External Ethernet interfaces
        int numExtEthInterfaces = default(0); 

    gates:
        inout virtInfr; // connection to the virtualization infrastructure to communicate with the MEapps
        input meAppIn[];
        output meAppOut[];
    submodules:

        serviceRegistry: ServiceRegistry {
            @display("p=767.0875,74.5975;");
        }

		//# old mec services NO ETSI MEC compliant
        omnetService[numOmnetServices]: <> like IMEService {
            @display("p=237.25,81.9,row");
        }


		//# app layer 
        mecService[numMecServices]: <> like IApp {
            @display("p=551.04,73.8,row,140");
        }

    connections allowunconnected:

	  	//# 
        //# Internal Tcp/Udp applications connections with transport layer
        //#

        for i=0..numMecServices-1 {
            mecService[i].socketOut --> at.in++;
            mecService[i].socketIn <-- at.out++;
        }
        
        serviceRegistry.socketOut --> at.in++;
        serviceRegistry.socketIn <-- at.out++;


}
