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
import simu5g.nodes.mec.MECPlatformManager.Dynamic.MecPlatformManagerHost;
import simu5g.nodes.mec.MECPlatform.MECPlatform;
import simu5g.nodes.mec.MECPlatformManager.MecPlatformManager;
import simu5g.nodes.Upf;
import inet.node.ethernet.Eth10G;
import inet.applications.contract.IApp;
import inet.linklayer.contract.IEthernetInterface;
import simu5g.nodes.NR.NRUe;
import inet.node.inet.Router;
import inet.node.ethernet.EthernetSwitch;



//
// @author Alessandro Calvio
// @author Angelo Feraudo
//

module MECHostDyn
{
    parameters:
        @display("i=device/mainframe;bgb=1036,665");

        // TODO static resources to be defined (now = normal MEC Host)
        //#	MEC Host Resources
        int maxMECApps = default(0);
        double localRam @unit("B") = default(32GB);
        double localDisk @unit("B") = default(100TB);
        double localCpuSpeed = default(300000); 	 // CPU's clock speed in Million Instruction per Seconds (MIPS)

        //# Node specs
        string nodeType = "MECHost";
        string gateway = default("upf");

        int numBGMecApp = default(0);
        string eNBList = default("");


        // Graphic parameters
        string color = default("red");

    gates:
        inout ppp[];

    submodules:

        vim: VirtualisationInfrastructureManagerHost {
            // Local virtualised resources
            // It contains a virtualisation infrastructure app to deploy mecapp
            // and a virtualisation infrastructure manager app to manage available resources.
            // The latter is able to manage remote resources, such as those provided by cars
            @display("p=232.81201,100.350006");
        }

        mecPlatformManager: MecPlatformManagerHost {
			// MecPlatform manager host
			// It runs an app that manages available services
            @display("p=737.23804,100.350006");
        }

        mecPlatform: MECPlatform {
			// MecPlatform  host
			// It runs available services
            @display("p=483.018,100.350006;i=device/device");
        }

        router: Router {

            @display("p=483.018,259.57202");
        }
        upf_mec: Upf {
            parameters:
                @display("p=483.018,404.07602");
                nodeType = "UPF_MEC";
        }

    connections allowunconnected:
        vim.pppUPFg++ <--> Eth10G <--> router.pppg++;
        mecPlatformManager.pppUPFg++ <--> Eth10G <--> router.pppg++;
        mecPlatform.virtInfr <--> Eth10G <--> router.pppg++; // this may be changed with a customised version

        router.pppg++ <--> Eth10G <--> upf_mec.pppg++;
        router.pppg++ <--> Eth10G <--> upf_mec.filterGate;// Needed for communications between local apps and ues 
		// upf_mec is connected with an antenna and an i-upf
        for i=0..sizeof(ppp)-1 {
            ppp++ <--> upf_mec.pppg++;
        }

}
