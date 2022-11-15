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

package simu5g.nodes.cars;
import simu5g.nodes.NR.NRUe;
import simu5g.apps.mec.ViApp.VirtualisationInfrastructureApp;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.networklayer.configurator.ipv4.HostAutoConfigurator;

module SpawnedMecCar extends NRUe
{
    parameters:
        @display("i=veins/node/car");

        // Available resources
        double localRam @unit("B") = default(5GB);
        double localDisk @unit("B") = default(0.5TB);
        double localCpuSpeed = default(300000);
        
        *.routingTableModule = default(absPath(".ipv4.routingTable"));
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        
        ipv4.configurator.typename = "HostAutoConfigurator";
        ipv4.configurator.interfaces = "cellular lo0";
        ipv4.configurator.addressBase = "10.7.0.0";
        ipv4.configurator.netmask = "255.255.0.0";

        // App configuration
//        numApps = 1;
//        
//        app[0].typename = "VirtualisationInfrastructureApp";
//        app[0].localPort = 2222;

    submodules:
        
    	viApp: VirtualisationInfrastructureApp {

            @display("p=751.376,71.271996");
        }
        
    connections allowunconnected:

        viApp.socketOut --> at.in++;
        viApp.socketIn <-- at.out++;

}
