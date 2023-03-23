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

package simu5g.apps.mec.ResourceSharingApps.Client;

import inet.applications.contract.IApp;

simple ClientResourceApp like IApp{
	parameters:
	    @display("i=block/app");
	    // resources may be loaded from a json file
	    // localAddress
	    @signal[joiningTime](type=long);
    	@statistic[joiningTime](source=joiningTime; unit=s; record=mean; interpolationmode=none);
    	@signal[leavingTime](type=long);
    	@statistic[leavingTime](source=leavingTime; unit=s; record=mean; interpolationmode=none);
    	
	    string localAddress = default("") @mutable;
	    int localPort = default(3001);
	    
	    string destAddress = default("");
	    int destPort = default(3000);
	   
	    int minReward = default(15);
	    
	    int viPort;
	    
	    double startTime @unit(s) = default(1s); 
    	double parkTime @unit(s) = default(1s) @mutable; // for how long a car will be parked
	    
	    
	    
	gates:
	   	output socketOut;
        input socketIn;

}