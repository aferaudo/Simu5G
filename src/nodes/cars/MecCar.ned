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

package simu5g.nodes.cars;
import simu5g.nodes.NR.NRUe;

module MecCar extends NRUe
{
    parameters:
    	@display("i=device/car");
    	
    	// Available resources
    	double localRam @unit("B") = default(5GB);
        double localDisk @unit("B") = default(1TB);
        double localCpuSpeed = default(300000);
        
        // App configuration
//        numApps = 1;
//        
//        app[0].typename = "VirtualisationInfrastructureApp";
//        app[0].localPort = 2222;
    	
   	submodules:
   	    
}
