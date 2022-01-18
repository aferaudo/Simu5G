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

package simu5g.apps.mec.ResourceSharingApps.Server;

import inet.applications.contract.IApp;

//
// @author Alessandro Calvio
// @author Angelo Feraudo
//
simple ResourceRegisterApp like IApp
{
    parameters:
        @display("i=block/app");
        string localAddress = default("");
        int localPort = default(3000);
        
        // Client App port
        int destPort = default(3001); // Maybe this is not needed
        
        double startTime @unit(s) = default(1s);
	
		// Virtualisation Infrastructure Manager address
		// - address used to send available client-resources to the VIM
		string vimAddress = default(""); // TODO change with a possible address

		// Reward system
		string rewardSystem = default("");
	
	gates:
	    output socketOut;
        input socketIn;
	    
}
