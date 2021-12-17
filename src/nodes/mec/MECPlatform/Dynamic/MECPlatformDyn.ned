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

package simu5g.nodes.mec.MECPlatform.Dynamic;
import simu5g.nodes.mec.MECPlatform.MECPlatform;
import simu5g.stack.ICellularNic;
import inet.linklayer.contract.IEthernetInterface;

//
// MEC Platform supporting D2D Communications
// @author Alessandro Calvio
// @author Angelo Feraudo
//
module MECPlatformDyn extends MECPlatform
{
    parameters:
        string nicType = default("NRNicUe"); // do not change
        string nodeType = "UE";  // DO NOT CHANGE


    gates:
        input radioUE @directIn; // d2d
        inout ethExt; // this gate is used for communications involving antenna (no d2d)

    submodules:
        cellularNic: <nicType> like ICellularNic {
            nodeType = nodeType;
            @display("p=143.565,520.775");
        }

        // ethernet interface used for communicating with external MEC applications in external virtualization infrastructures
        mp1EthExt: <default("EthernetInterface")> like IEthernetInterface {
            parameters:
                @display("p=955.6925,520.775,row,150;q=txQueue");
        }

    connections allowunconnected:
        mp1EthExt.upperLayerOut --> nl.in++;
        mp1EthExt.upperLayerIn <-- nl.out++;
        mp1EthExt.phys <--> ethExt;

        cellularNic.upperLayerOut --> nl.in++;
        cellularNic.upperLayerIn <-- nl.out++;

        cellularNic.radioIn <-- radioUE;
}
