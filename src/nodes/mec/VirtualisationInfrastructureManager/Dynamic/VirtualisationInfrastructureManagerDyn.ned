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

import simu5g.nodes.mec.VirtualisationInfrastructureManager.VirtualisationInfrastructureManager;
import inet.applications.contract.IApp;

//
// TODO auto-generated module
//
simple VirtualisationInfrastructureManagerDyn extends VirtualisationInfrastructureManager like IApp
{
    parameters:
    	@class(VirtualisationInfrastructureManagerDyn);
    	@signal[allocationTime](type=long);
    	@statistic[remoteAllocationTime](source=allocationTime; unit=s; record=histogram,vector; interpolationmode=none);
    	int localPort = default(3333); // port for UDP communications VIM <--> VI
    	
//    	string destAddress;
        string localAddress; // address used for local binding
        
        //broker parameters
       	string brokerAddress; // address for notification system
       	int localBrokerPort = default(3334); // port for TCP communications VIM <--> Broker
    	int brokerPort = default(3005); // destination broker port
    	
    	// pubsub parameters
		string subscribeURI = default("/broker/subscribe/");
		string webHook = default("/newresources/");
    	
    	// mecorchestrator parameters
    	string meoAddress;
    	int meoPort;
    	
    	// mepm parameters
		string mepmAddress;
		int mepmPort = default(3334);
		
		// mep parameters - port is already in the VirtualisationInfrastructureManager ned base
		string mp1Address;
		

    	double bufferSize = default(0.15);				// Percentage value of resources buffer
    	
//    	string searchType = default("BEST_FIRST");
    	string searchType @enum("BEST_FIRST", "ROUND_ROBIN") = default("BEST_FIRST");
    	bool skipLocalResources = default(true) @mutable;
    	
    
    gates:
        output socketOut;
        input socketIn;
     	
     	
}
