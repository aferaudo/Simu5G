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
        int localBrokerPort = default(3333);
        
        int brokerPort = default(3005);
        string brokerAddress = default("127.0.0.1");
        
        string  serverThreadClass = default("simu5g.apps.mec.ResourceSharingApps.Server.ResourceRegisterThread");
                        
        double startTime @unit(s) = default(1s);
        
		// Reward system
		string rewardSystem = default("");
	
	gates:
	    output socketOut;
        input socketIn;
	    
}
