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
#include "nodes/mec/Dynamic/MEO/Messages/MeoPackets_m.h" // new messages
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"
#include "apps/mec/ResourceSharingApps/Client/ClientResourceApp.h"
#include "apps/mec/ViApp/msg/InstantiationResponse_m.h"
#include "apps/mec/ViApp/msg/TerminationResponse_m.h"
#include "inet/common/packet/printer/PacketPrinter.h"


using namespace omnetpp;

struct RunningAppEntry
{
    int ueAppID;
    ResourceDescriptor resources;
    std::string moduleName;
    std::string moduleType;
    std::string requiredService;
    cModule* module;
    int port;
    cGate* inputGate;
    cGate* outputGate;
};

class ClientResourceApp;

class VirtualisationInfrastructureApp : public cSimpleModule
{

    inet::UdpSocket socket;

    inet::L3Address localAddress;
    int localPort;

    inet::L3Address vimAddress;
    int vimPort;

    // Business logic parameters
    int appcounter;
    int maxappcounter;
    int portCounter = 10000;    // counter to assign port to app
    std::list<std::string> managedApp;
    std::map<int, RunningAppEntry> runningApp;
    SchedulingMode scheduling;
    cModule *toDelete;
    std::queue<cModule *> terminatingModules;
    cMessage *deleteModuleMessage;

    double maxCpu;
    double allocatedCpu;

    double maxRam;
    double allocatedRam;

    double maxDisk;
    double allocatedDisk;

    // Graphic parameters
    float posx;
    float posy;

    static simsignal_t parkingReleased_;
    static simsignal_t numApp_;

    ClientResourceApp* resourceApp;

    public:
        VirtualisationInfrastructureApp();
        ~VirtualisationInfrastructureApp();

        double calculateProcessingTime(int ueAppID, int numOfInstructions);
        int getHostedAppNum();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }

        void initialize(int stage);

        virtual void handleMessage(cMessage *msg);

        virtual void finish() override;

    private:

        bool handleInstantiation(InstantiationApplicationRequest* data);
        bool handleTermination(DeleteAppMessage* data);

        void handleEndTerminationProcedure(cMessage *);
        void handleModuleRemoval(cMessage *);

};


#endif
