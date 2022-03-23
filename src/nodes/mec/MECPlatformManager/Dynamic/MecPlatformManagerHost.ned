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

import simu5g.nodes.NR.NRUe;
import inet.linklayer.ppp.PppInterface;

//
// TODO auto-generated type
//
module MecPlatformManagerHost extends NRUe
{
    parameters:
        @networkNode();
        @display("i=device/device");

        string networkLayerType = default("IPv4NetworkLayer");

        //# Network Layer specs
        *.routingTableModule = default(absPath(".ipv4.routingTable"));
        *.interfaceTableModule = default(absPath(".interfaceTable"));

	gates:
	    inout ppp;

    submodules:
        mepmApp: MecPlatformManagerDyn {

            @display("p=549.64,71.271996");
        }
        
        pppIface: PppInterface {

            @display("p=274.216,521.856;i=device/card");
        }
        
    connections allowunconnected:
        
        mepmApp.socketOut --> at.in++;
        mepmApp.socketIn <-- at.out++;
        
        pppIface.upperLayerOut --> nl.in++;
        pppIface.upperLayerIn <-- nl.out++;
        pppIface.phys <--> ppp;
}
