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

        string networkLayerType = default("IPv4NetworkLayer");

        //# Network Layer specs
        *.routingTableModule = default(absPath(".ipv4.routingTable"));
        *.interfaceTableModule = default(absPath(".interfaceTable"));

        //# Resource of the VimHost
//        double localRam @unit("B") = default(32GB);
//        double localDisk @unit("B") = default(100TB);
//        double localCpuSpeed = default(300000);

        //# App parameters
        numApps = 2;
        app[0].typename = "VirtualisationInfrastructureManagerDyn";
        app[0].localPort = 3333;
        app[1].typename = "VirtualisationInfrastructureApp";
		app[1].localPort = 2222;

        @display("i=device/device");
    gates:
        
		// to gnodes
        inout pppENB[];
        
        // to server
        inout ethiface;

    submodules:
       	// ethernet interface used for communicating with Server
        serverEth: <default("EthernetInterface")> like IEthernetInterface {
            parameters:
                @display("p=813,523,row,150;q=txQueue");
        }
        
    connections allowunconnected:

        nl.out++ --> serverEth.upperLayerIn;
        nl.in++ <-- serverEth.upperLayerOut;
        
		ethiface <--> serverEth.phys;


}
