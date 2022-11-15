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

package simu5g.apps.mec.ViApp;

import inet.applications.contract.IApp;

//
// TODO auto-generated type
//
simple VirtualisationInfrastructureApp like IApp
{
    parameters:
    	@display("i=block/control");
    	@signal[parkingReleased](type=string);
    	@signal[numApp](type=int);
    	@statistic[maxNumApp](source=numApp; record=max);
    	
    	string localAddress = default("") @mutable;
    	int localPort = default(2222);
    	
    	string vimAddress = default("") @mutable;
    	int vimPort = default(3333);
    	
    	string scheduling = default("segregation");
    	
   	gates:
        output socketOut;
        input socketIn;
        
        inout appGates[];
    	
}
