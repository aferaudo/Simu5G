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
    std::string schedulingMode = par("scheduling").stringValue();
    if(std::strcmp(schedulingMode.c_str(), "segregation") == 0)
    {
        EV << "VirtualisationInfrastructureManager::initialize - scheduling mode is: segregation" << endl;
        scheduling = SEGREGATION;
    }
    else if(std::strcmp(schedulingMode.c_str(), "fair") == 0)
    {
        EV << "VirtualisationInfrastructureManager::initialize - scheduling mode is: fair" << endl;
        scheduling = FAIR_SHARING;
    }
    else
    {
        EV << "VirtualisationInfrastructureManager::initialize - scheduling mode: " << schedulingMode<< " not recognized. Using default mode: segregation" << endl;
        scheduling = SEGREGATION;
    }


    cModule* modulewresdefinition = nullptr;
    if(!std::strcmp(getParentModule()->getFullName(), "vim"))
        modulewresdefinition = getParentModule()->getParentModule();
    else
        modulewresdefinition = getParentModule();

    maxCpu = modulewresdefinition->par("localCpuSpeed").doubleValue();
    maxRam = modulewresdefinition->par("localRam").doubleValue();
    maxDisk = modulewresdefinition->par("localDisk").doubleValue();
    allocatedCpu = 0.0;
    allocatedRam = 0.0;
    allocatedDisk = 0.0;

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

        cModule* module = getParentModule()->getSubmodule("MECWarningAlertApp[0]");
        std::cout << module << endl;
//        module->callFinish();
        module->deleteModule();

    }else{
        EV << "VirtualisationInfrastructureApp::handleMessage - other message received!" << endl;

        if (!strcmp(msg->getName(), "Instantiation")){
            inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
            if (pPacket == 0)
                throw cRuntimeError("VirtualisationInfrastructureApp::handleMessage - FATAL! Error when casting to inet packet");

            auto data = pPacket->peekData<CreateAppMessage>();
//            CreateAppMessage * createAppMsg = new CreateAppMessage();
//            createAppMsg->setUeAppID(data->getUeAppID());
//            createAppMsg->setMEModuleName(data->getMEModuleName());
//            createAppMsg->setMEModuleType(data->getMEModuleType());
//
//            createAppMsg->setRequiredCpu(data->getRequiredCpu());
//            createAppMsg->setRequiredRam(data->getRequiredRam());
//            createAppMsg->setRequiredDisk(data->getRequiredDisk());
//
//            createAppMsg->setRequiredService(data->getRequiredService());
//            createAppMsg->setContextId(data->getContextId());

            bool res = handleInstantiation(const_cast<CreateAppMessage*>(data.get()));
//            bool res = handleInstantiation(createAppMsg);

            if(res){
                // send response back to vim
                inet::L3Address vimAddress = pPacket->getTag<inet::L3AddressInd>()->getSrcAddress();
                int vimPort = pPacket->getTag<inet::L4PortInd>()->getSrcPort();
                inet::Packet* packet = new inet::Packet("InstantiationResponse");
                auto responsepck = inet::makeShared<InstantiationResponse>();
                responsepck->setAllocatedPort(portCounter);
                responsepck->setUeAppID(data->getUeAppID());
                responsepck->setChunkLength(inet::B(100));
                packet->insertAtBack(responsepck);
                socket.sendTo(packet, vimAddress, vimPort);

                EV << "VirtualisationInfrastructureApp::handleMessage - sending back to " << vimAddress << ":" << vimPort << endl;

            }

            cMessage* message = new cMessage("Terminazione");
            scheduleAt(simTime()+0.2, message);

        }
    }

    delete msg;
}

bool VirtualisationInfrastructureApp::handleInstantiation(CreateAppMessage* data)
{
    EV << "VirtualisationInfrastructureApp::handleInstantiation - " << data << endl;

    appcounter++;
    portCounter++;

    // Creation of requested app
    char* meModuleName = (char*)data->getMEModuleName();
    cModuleType* moduleType = cModuleType::get(data->getMEModuleType());
    cModule* module = moduleType->create(meModuleName, getParentModule());
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
    module->par("localUePort") = portCounter;
    module->par("mp1Address") = "mechost";
    module->par("mp1Port") = 3333;

    module->finalizeParameters();

    cModule *at = getParentModule()->getSubmodule("at");
    cGate* newAtInGate = at->getOrCreateFirstUnconnectedGate("in", 0, false, true);
    cGate* newAtOutGate = at->getOrCreateFirstUnconnectedGate("out", 0, false, true);

    newAtOutGate->connectTo(module->gate("socketIn"));
    module->gate("socketOut")->connectTo(newAtInGate);

    module->buildInside();
    module->scheduleStart(simTime());
    module->callInitialize();

    runningApp[data->getUeAppID()] = *data;
    allocatedCpu += cpu;
    allocatedRam += ram;
    allocatedDisk += disk;

    return true;
}

double VirtualisationInfrastructureApp::calculateProcessingTime(int ueAppID, int numOfInstructions)
{
    EV << "VirtualisationInfrastructureApp::calculateProcessingTime - (" << ueAppID << ", " << numOfInstructions << ")" << endl;

    ASSERT(numOfInstructions >= 0);

    auto ueApp = runningApp.find(ueAppID);
    if(ueApp != runningApp.end())
    {
        double time;
        if(scheduling == FAIR_SHARING)
        {
            double currentSpeed = ueApp->second.getRequiredCpu() *(maxCpu/allocatedCpu);
            time = numOfInstructions/currentSpeed;
        }
        else
        {
            double currentSpeed = ueApp->second.getRequiredCpu();
            time = numOfInstructions/currentSpeed;
        }
        EV << "VirtualisationInfrastructureApp::calculateProcessingTime - calculated time: " << time << endl;

        return time;
    }
    else
    {
        EV << "VirtualisationInfrastructureApp::calculateProcessingTime - ZERO " << endl;
        return 0;
    }
}
