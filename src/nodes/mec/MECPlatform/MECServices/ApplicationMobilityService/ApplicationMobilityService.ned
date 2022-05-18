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

package simu5g.nodes.mec.MECPlatform.MECServices.ApplicationMobilityService;

import simu5g.nodes.mec.MECPlatform.MECServices.MECServiceBase.IMECService;
import inet.applications.contract.IApp;

// @author Alessandro Calvio
// @author Angelo Feraudo
//
// Application Mobility Service ETSI GS MEC 021 v 2.2.1
//

simple ApplicationMobilityService like IMECService, IApp
{	
    int requestQueueSize = default(0);
    int subscriptionQueueSize = default(0);
    string localAddress = default(""); // local address; may be left empty ("")
    int localPort = default(5000);     // localPort number to listen on
    
    double requestServiceTime  @unit(s) = default(0.5us);
    double subscriptionServiceTime @unit(s) = default(0.5us);
        
    string serviceName = default("ApplicationMobilityService");
   	string serviceVersion = default("v1");
   	string serviceSerialize = default("JSON");
   	
   	string transportId = default("AmsAPI");
   	string transportName = default("AmsAPI");
   	string transportType = default("REST_HTTP");
   	string transportProtocol = default("HTTP");
   	
   	string catId = default("");
	string catName = default("ApplicationMobility");
	string catUri = default("");
	string catVersion = default("v1");	
	
	string scopeOfLocality = default("MEC_HOST");
    bool consumedLocalOnly =  default(true); 
	
	bool loadGenerator = default(false);
	double betaa  = default(0);  // used only if loadGenerator is true
	int numBGApps = default(0); // used only if loadGenerator is true
    
    // Socket manager
   	string serverThreadClass = default("simu5g.nodes.mec.MECPlatform.MECServices.MECServiceBase.SocketManager");
    
    string dataTransferMode @enum("bytecount","object","bytestream") = default("bytestream"); // bytestream for emulation
    
    
    @display("i=block/app");
    @signal[requestQueueSize];
    @statistic[requestQueueSizeStat](title="Request queue size"; source="requestQueueSize"; record=mean,vector);
    @signal[responseTime];
    @statistic[responseTimeStat](title="Response time of foreground requests"; source="responseTime"; record=mean,vector);
    
    
    gates:
        input socketIn;
        output socketOut;
}
