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

import inet.common.packet.chunk.Chunk;

class SystemInfo extends inet::FieldsChunk
{
    string systemId;
    string systemName;
    string systemProvider;
    string ipMefRequest;
}

class SystemInfoUpdate extends inet::FieldsChunk
{
    string systemName;
    string endpoint;
}

class MsgId extends inet::FieldsChunk
{
    string id;
}

class AckPk extends inet::FieldsChunk{
    string ackId;
}

class AppRequest extends inet::FieldsChunk{
	string appId;
	string appName;
	
	string ipMefRequest;
	string ipRequest;
	int portRequest;
	
	int hostId;
}
    
class AppResponse extends inet::FieldsChunk{
	string appId;
	string appName;
	
	string ipMefRequest;
	string ipRequest;
	int portRequest;
	
	string appAddress;
	int appPort;
	
	int hostId;
}   

class StartAppContextChunk extends inet::FieldsChunk{
    string appName;
    string appPackageSource;
    bool appIsOnboarded;
    int devAppId;
    
}

class AppMigrationRequest extends inet::FieldsChunk{
    string appName;
    string appPackageSource;
    bool appIsOnboarded;
    string devAppId;
    
    string appAddress;
	int appPort;
	
	int gbNode;
	
	string ueIpAddress;
	
	string ipMefRequest;
	simtime_t startTime;
    
}

class MecPingAppSyncMessage extends inet::FieldsChunk {
    int contextId;
    
    int pingSending;

    
    bool isAck = false;
    bool result = false;
    
    
}


class FederationMigrationTrigger extends inet::FieldsChunk {
    int gbNode;
    string appId;
    int appIdMigration;
    
    string ueIpAddress;
    
    int portMigration;
    string addressMigration;
    
    int targetPort;
    string targetAddress;
    
    simtime_t startTime;
}

class FederatorInfo extends inet::FieldsChunk {
    int port;
    string address;
    double tempo;
}

class AckMigration extends inet::FieldsChunk {
    string appId;
    string ipMefRequest;
}


class MigrateState extends inet::FieldsChunk {
    @packetData;
    int pingSending; 
    int port;
}
    