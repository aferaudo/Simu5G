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

package simu5g.apps.mec.PingPongApp;

import inet.applications.contract.IApp;
import simu5g.nodes.mec.MECPlatform.IMECApp;


simple UEPingPongApp like IApp
{ 
    parameters:
        @display("i=block/source"); 
        
        string deviceAppAddress;
        int deviceAppPort;
        double startTime @unit(s);
        int localPort = default(4242);
        
        int tos = default(-1);
        
        string mecAppName = default("PingPongApp");
        
        int packetSize = default(10);
        bool logger = default(false);
        string interfaceTableModule = default("");
    gates:
        input socketIn;
        output socketOut;
}      