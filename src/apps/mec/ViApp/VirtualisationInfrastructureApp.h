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
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

#include "nodes/mec/utils/MecCommon.h"

#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <omnetpp.h>

#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"
#include "apps/mec/ViApp/msg/InstantiationResponse_m.h"


using namespace omnetpp;

class VirtualisationInfrastructureApp : public cSimpleModule
{

    inet::UdpSocket socket;

    inet::L3Address localAddress;
    int localPort;

    inet::L3Address vimAddress;
    int vimPort;

    // Business logic parameters
    int appcounter;
    int portCounter = 10000;    // counter to assign port to app
    std::list<std::string> managedApp;
    std::map<int, CreateAppMessage> runningApp;
    SchedulingMode scheduling;

    double maxCpu;
    double allocatedCpu;

    double maxRam;
    double allocatedRam;

    double maxDisk;
    double allocatedDisk;

    // Graphic parameters
    float posx;
    float posy;

    public:
        VirtualisationInfrastructureApp();

        double calculateProcessingTime(int ueAppID, int numOfInstructions);

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }

        void initialize(int stage);

        virtual void handleMessage(cMessage *msg);

    private:

        bool handleInstantiation(CreateAppMessage* data);

};


#endif
