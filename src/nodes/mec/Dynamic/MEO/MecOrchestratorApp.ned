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

package simu5g.nodes.mec.Dynamic.MEO;

import inet.applications.contract.IApp;

// @author Alessandro Calvio
// @author Angelo Feraudo

simple MecOrchestratorApp like IApp
{
    parameters:
        @display("i=block/app");
        int localPort;
        string localAddress = default("");
        
        // List of onboarded appliaction package
        string mecApplicationPackageList = default("");
    gates:
	    output socketOut;
        input socketIn;
        
		output toUALCMP;
        input fromUALCMP;
}



