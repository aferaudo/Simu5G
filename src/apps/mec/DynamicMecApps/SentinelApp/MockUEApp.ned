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

package simu5g.apps.mec.DynamicMecApps.SentinelApp;

import inet.applications.contract.IApp;

//
// @author
// Angelo Feraudo 
// This app has been designed as a MOCK app running on a UE to test 
// the correct deployment of the MecApp in the MecHost
//
simple MockUEApp like IApp
{
    parameters:
        //connection infos
        int localPort = default(4000);
        int deviceAppPort = default(4000); // ipAddress of the DeviceApp
        string deviceAppAddress = default("") @mutable; // port of the DeviceApp
                
        int packetSize = default(10);
        
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        
        //autoscheduling infos
        double period @unit("s") = default(1s);
        double startTime @unit("s") = default(0s); 
        double stopTime @unit("s") = default(60s);
        
        string mecAppName = default("SentinelMecApp"); 
    gates:
        input socketIn;
        output socketOut;

}
