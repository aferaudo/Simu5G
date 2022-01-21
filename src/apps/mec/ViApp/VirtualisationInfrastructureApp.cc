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

// Imports
#include <sstream>

#include "VirtualisationInfrastructureApp.h"
#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"

Define_Module(VirtualisationInfrastructureApp);

VirtualisationInfrastructureApp::VirtualisationInfrastructureApp()
{

}

void VirtualisationInfrastructureApp::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER-1){
        return;
    }

    EV << "VirtualisationInfrastructureApp::initialize - stage " << stage << endl;

    // Get parameters
    localPort = par("localPort");
    vimPort = par("vimPort");

    localAddress = inet::L3AddressResolver().resolve(par("localAddress"));
    vimAddress = inet::L3AddressResolver().resolve(par("vimAddress"));

    if (localPort != -1)
    {
        socket.setOutputGate(gate("socketOut"));
        socket.bind(localPort);
    }
    EV << "VirtualisationInfrastructureApp::initialize - binding to port: local:" << localPort << " , dest: " << vimAddress.str() << ":" << vimPort << endl;

    appcounter = 0;

    // Init Graph
    cDisplayString& dispStr = getDisplayString();
    posx = atof(dispStr.getTagArg("p", 0));
    posy = atof(dispStr.getTagArg("p", 1));

    EV << "VirtualisationInfrastructureApp::initialize - reference position: " << posx << " , " << posy << endl;

}

void VirtualisationInfrastructureApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        EV << "VirtualisationInfrastructureApp::handleMessage - self message received!" << endl;

    }else{
        EV << "VirtualisationInfrastructureApp::handleMessage - other message received!" << endl;

        if (!strcmp(msg->getName(), "Instantiation")){
            inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
            if (pPacket == 0)
                throw cRuntimeError("VirtualisationInfrastructureApp::handleMessage - FATAL! Error when casting to inet packet");

            auto data = pPacket->peekData<CreateAppMessage>();

            appcounter++;

            handleInstantiation(const_cast<CreateAppMessage*>(data.get()));

        }
    }

    delete msg;
}

void VirtualisationInfrastructureApp::handleInstantiation(CreateAppMessage* data)
{
    EV << "VirtualisationInfrastructureApp::handleInstantiation - " << data << endl;

    // Creation of requested app
    char* meModuleName = (char*)data->getMEModuleName();
    cModuleType *moduleType = cModuleType::get(data->getMEModuleType());
    cModule *module = moduleType->create(meModuleName, getParentModule());
    std::stringstream appName;
    appName << meModuleName << "[" <<  data->getContextId() << "]";
    module->setName(appName.str().c_str());
    EV << "VirtualisationInfrastructureApp::handleMessage - meModuleName: " << appName.str() << endl;

    double ram = data->getRequiredRam();
    double disk = data->getRequiredDisk();
    double cpu = data->getRequiredCpu();

    //displaying ME App dynamically created (after 70 they will overlap..)
    std::stringstream display;
    display << "p=" << posx + (appcounter*300) << "," << posy << ";i=block/control";
    module->setDisplayString(display.str().c_str());

    module->par("mecAppId") = 22;
    module->par("requiredRam") = ram;
    module->par("requiredDisk") = disk;
    module->par("requiredCpu") = cpu;
    module->par("localUePort") = 1;
    module->par("mp1Address") = "mechost";
    module->par("mp1Port") = 3333;

    module->finalizeParameters();

    cModule *at = getParentModule()->getSubmodule("at");
    cGate* newAtInGate = at->getOrCreateFirstUnconnectedGate("in", 0, false, true);
    cGate* newAtOutGate = at->getOrCreateFirstUnconnectedGate("out", 0, false, true);

    newAtOutGate->connectTo(module->gate("socketIn"));
    module->gate("socketOut")->connectTo(newAtInGate);

//    module->buildInside();
//    module->scheduleStart(simTime());
//    module->callInitialize();
}
