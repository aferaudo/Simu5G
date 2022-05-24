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
import inet.networklayer.common.L3Address;
cplusplus {{
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/AssociateId.h"
}}

class AssociateId
{
    @existingClass;
    @opaque;
}

class ServiceMobilityRequest extends inet::FieldsChunk
{
    // TODO here we should add a field that indicate the notification type
    AssociateId associateId[];
    string appInstanceId; 
}

class ServiceMobilityResponse extends inet::FieldsChunk
{
	 string appInstanceId;
	 inet::L3Address targetAddress;
	 int targetPort;
	 AssociateId associateId[];
}
