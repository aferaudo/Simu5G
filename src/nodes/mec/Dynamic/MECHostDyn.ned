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

//
// @author Alessandro Calvio
// @author Angelo Feraudo
//

// This MEC host module has two type of resources:
// - static 
// - non static (this are managed by the Dynamic virutalisation infrastructure Manager)
// Static resources are less if compared with a normal MEC Host
module MECHostDyn
{
    parameters:
        @display("i=device/server2;bgb=1000,500");

        // TODO static resources to be defined (now = normal MEC Host)
        //#	MEC Host Resources
        int maxMECApps = default(0);
        double maxRam @unit("B") = default(32GB);
        double maxDisk @unit("B") = default(100TB);
        double maxCpuSpeed = default(300000); 	 // CPU's clock speed in Million Instruction per Seconds (MIPS)

    gates:
        inout ethAntenna[];
        input radioIn[] @directin; // for d2d communication

    submodules:

        vi: VirtualisationInfrastructure { //defined for local apps
            @display("p=461.84,214.605");
        }

        vimH: VirtualisationInfrastructureManagerHost {

            @display("p=414,400");
        }
        mecPlatformD: MECPlatformDyn {
            @display("p=643.815,399.09");
        }
        mecPlatformManager: MecPlatformManager {

            @display("p=853,126");
        }
        // UPF module used to connect the MEC host to the 5G Core Network (either a BS or a UPF)
        upf_mec: Upf {
            parameters:
                @display("p=150,400");
                nodeType = "UPF_MEC";
        }

        bgApp[numBGMecApp]: <> like IApp {
            @display("p=390.305,69.025,row,140");
        }

    connections allowunconnected:

        // TODO define App communications Local apps
        for i=0..numBGMecApp-1 {
            bgApp[i].socketIn <-- vi.meBGAppOut[i];
            bgApp[i].socketOut --> vi.meBGAppIn[i];
        }

        vimH.mm7GnB <--> Eth10G <--> ethAntenna++;
        mecPlatformD.ethExt <--> Eth10G <--> ethAntenna++;
		
		vi.mePlatform <--> Eth10G <--> mecPlatformD.virtInfr;
		
        // TODO define cellular communications
	   	// Cellular channels do not need an external interface
	   	// Two solutions:
	   	// 1. Connect via ethernet the cellular components (VIM and MECPlatform) to an antenna
	   	// 2. Be completely independent from the Dynamic MEC Host



}
