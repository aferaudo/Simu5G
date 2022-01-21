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
        
        //# App parameters
        numApps = 1;
       	app[0].typename = "MecPlatformManagerDyn";
        app[0].localPort = 3334;
        
}
