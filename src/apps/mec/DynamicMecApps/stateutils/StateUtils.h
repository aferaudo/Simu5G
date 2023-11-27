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

#ifndef APPS_MEC_DYNAMICMECAPPS_STATEUTILS_STATEUTILS_H_
#define APPS_MEC_DYNAMICMECAPPS_STATEUTILS_STATEUTILS_H_

#include<iostream>
#include<string>
#include "apps/mec/DynamicMecApps/stateutils/packets/MecAppSyncMessage_m.h"

namespace stateutils
{
    enum MsgState {
        COMPLETE_DATA,
        INCOMPLETE_DATA
    };

    bool parseStateData(int sockId, std::string& packet, omnetpp::cQueue& messageQueue, MecAppSyncMessage** currentMsg);
    MecAppSyncMessage* parseHeader(const std::string &data);
    MsgState addStateChunk(std::string* data, MecAppSyncMessage* currentMsg);
    std::string getPayload(const inet::Ptr<const MecAppSyncMessage>& msg, int &dim);
}

#endif /* APPS_MEC_DYNAMICMECAPPS_STATEUTILS_STATEUTILS_H_ */
