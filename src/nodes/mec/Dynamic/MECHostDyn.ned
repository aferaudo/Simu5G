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

package simu5g.nodes.mec.Dynamic;
import simu5g.nodes.mec.VirtualisationInfrastructure.VirtualisationInfrastructure;
import simu5g.nodes.mec.VirtualisationInfrastructureManager.Dynamic.VirtualisationInfrastructureManagerHost;
import simu5g.nodes.mec.MECPlatform.Dynamic.MECPlatformDyn;
import simu5g.nodes.mec.MECPlatformManager.MecPlatformManager;
import simu5g.nodes.Upf;
import inet.node.ethernet.Eth10G;
import inet.applications.contract.IApp;
import simu5g.nodes.NR.NRUe;

//
// @author Alessandro Calvio
// @author Angelo Feraudo
//

// This MEC host module has two type of resources:
// - static 
// - non static (this are managed by the Dynamic virutalisation infrastructure Manager)
// Static resources are less if compared with a normal MEC Host
module MECHostDyn extends NRUe
{
    parameters:
        @display("i=device/mainframe;bgb=1036,665");

        // TODO static resources to be defined (now = normal MEC Host)
        //#	MEC Host Resources
        int maxMECApps = default(0);
        double localRam @unit("B") = default(32GB);
        double localDisk @unit("B") = default(100TB);
        double localCpuSpeed = default(300000); 	 // CPU's clock speed in Million Instruction per Seconds (MIPS)
        
        // Setup Mec components
        numApps = 2;
        
        app[0].typename = "VirtualisationInfrastructureManagerDyn";
        app[0].localPort = 3333;
        
        app[1].typename = "MecPlatformManagerDyn";
        app[1].localPort = 3334;

    gates:
        inout ethAntenna[];
//        input radioIn[] @directin; // for d2d communication

    submodules:



    connections allowunconnected:

//        vimH.mm7GnB <--> Eth10G <--> ethAntenna++;
//        mecPlatformD.ethExt <--> Eth10G <--> ethAntenna++;
//
//        vi.mePlatform <--> Eth10G <--> mecPlatformD.virtInfr;

        // TODO define cellular communications
	   	// Cellular channels do not need an external interface
	   	// Two solutions:
	   	// 1. Connect via ethernet the cellular components (VIM and MECPlatform) to an antenna
	   	// 2. Be completely independent from the Dynamic MEC Host



}
