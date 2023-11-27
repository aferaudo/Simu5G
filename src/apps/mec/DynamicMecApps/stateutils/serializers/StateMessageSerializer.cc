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

#include "StateMessageSerializer.h"
#include "apps/mec/DynamicMecApps/stateutils/packets/MecAppSyncMessage_m.h"
#include "apps/mec/DynamicMecApps/stateutils/StateUtils.h"
#include "inet/common/packet/serializer/ChunkSerializerRegistry.h"

namespace inet
{
Register_Serializer(MecAppSyncMessage, StateMessageSerializer);

void StateMessageSerializer::serialize(MemoryOutputStream &stream,
        const Ptr<const Chunk> &chunk) const {
        EV << "StateMessageSerializer::serialize" << endl;
        auto startPosition = stream.getLength();
        EV << "StateMessageSerializer::serialize - stream length before: " << stream.getLength() << endl;

        const auto &applicationPacket = staticPtrCast<const MecAppSyncMessage>(chunk);
        int dim = 0;
        std::string payload = stateutils::getPayload(applicationPacket, dim);
        stream.writeBytes((const uint8_t*)payload.c_str(), B(payload.size()));
        EV << "StateMessageSerializer::serialize - stream length after: " << stream.getLength() << endl;

        int64_t remainders = B(applicationPacket->getChunkLength() - (stream.getLength() - startPosition)).get();

        if (remainders < 0)
                throw cRuntimeError("StateMessageSerializer::ApplicationPacket length = %d smaller than required %d bytes", (int)B(applicationPacket->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        
        stream.writeByteRepeatedly('?', remainders);
}

const Ptr<Chunk> StateMessageSerializer::deserialize(
        MemoryInputStream &stream) const {
        EV << "StateMessageSerializer::deserialize" << endl;
        auto applicationPacket = makeShared<MecAppSyncMessage>();
        // stream.
        return applicationPacket;
}
}
