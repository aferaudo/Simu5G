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

class GeneralMessage extends inet::FieldsChunk
{
    string deviceAppId;
}

class MeoVimRequest extends GeneralMessage 
{
    string deviceAppId;
    double ram;
    double disk;
    double cpu;
}

class MeoMepmRequest extends GeneralMessage
{
    string requiredServiceNames[];
}

class MecHostResponse extends GeneralMessage
{
   	int mecHostId;
    bool result;    
}

class InstantiationApplicationResponse extends GeneralMessage
{
    int mecHostId;
    int contextId;
    bool status;
    string instanceId;
    string appName;
    // This are used in case of dynamic MEC Host - app executed on car
    inet::L3Address mecAppRemoteAddress;
    int mecAppRemotePort;
}

