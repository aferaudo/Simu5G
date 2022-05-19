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

import inet.applications.contract.IApp;

//
// TODO auto-generated type
//
simple MecPlatformManagerDyn like IApp
{
    parameters:
        @class(MecPlatformManagerDyn);
		
		// broker adress (ams service)
		string brokerAddress = default(""); // address for notification system
       	//int localBrokerPort = default(3334); // port for TCP communications VIM <--> Broker
    	int brokerPort = default(3005); // destination broker port
    	
    	string subscribeURI = default("/example/amsi/v1/subscriptions/");
		string webHook = default("/mobilityProcedure/");
    	
        string localAddress;
        int localPort = default(3334);

        string meoAddress;
        int meoPort = default(3300);
        
        // vim
        string vimAddress;
        int vimPort = default(3333);

        @display("i=block/control");
    gates:
        output socketOut;
        input socketIn;
}
