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

enum ReferencePointType {  
  NONE = 0; 
  MM3 = 1;
  MM4 = 2;
 };
 
class RegistrationPkt extends inet::FieldsChunk 
{
	ReferencePointType type = NONE;
	int hostId;
	inet::L3Address sourceAddress;
	int sourcePort;
}
