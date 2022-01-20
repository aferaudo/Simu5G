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

#ifndef APP_VI_H
#define APP_VI_H

//BINDER and UTILITIES
#include "common/LteCommon.h"
#include "common/binder/Binder.h"
#include "inet/networklayer/common/InterfaceTable.h"

//INET
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "nodes/mec/utils/MecCommon.h"

#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <omnetpp.h>

using namespace omnetpp;

class VirtualisationInfrastructureApp : public cSimpleModule
{

    inet::UdpSocket socket;

    inet::L3Address localAddress;
    int localPort;

    inet::L3Address vimAddress;
    int vimPort;

    public:
    VirtualisationInfrastructureApp();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }

        void initialize(int stage);

        virtual void handleMessage(cMessage *msg);

    private:

};


#endif
