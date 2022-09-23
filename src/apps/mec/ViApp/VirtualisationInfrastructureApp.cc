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

simsignal_t VirtualisationInfrastructureApp::parkingReleased_ = registerSignal("parkingReleased");
simsignal_t VirtualisationInfrastructureApp::numApp_ = registerSignal("numApp");

VirtualisationInfrastructureApp::VirtualisationInfrastructureApp()
{

}

VirtualisationInfrastructureApp::~VirtualisationInfrastructureApp()
{
    if(deleteModuleMessage->isScheduled())
    {
        std::cout << "Message is scheduled deleting it..."<<endl;
        cancelAndDelete(deleteModuleMessage);
    }
    else
    {
        delete deleteModuleMessage; // even though message is not scheduled it has to be deleted
    }
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
    maxappcounter = 0;
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

    if(strcmp(getParentModule()->getName(), "vim") != 0){
        resourceApp = check_and_cast<ClientResourceApp*>(getParentModule()->getSubmodule("app",0));
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
    deleteModuleMessage = new cMessage("deleteModule");

}

void VirtualisationInfrastructureApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if(std::strcmp(msg->getFullName(), "deleteModule") == 0)
        {
            handleModuleRemoval(msg);
        }


    }else{
        EV << "VirtualisationInfrastructureApp::handleMessage - other message received!" << endl;

        if (!strcmp(msg->getName(), "Instantiation")){
            inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
            if (pPacket == 0)
                throw cRuntimeError("VirtualisationInfrastructureApp::handleMessage - FATAL! Error when casting to inet packet");

            inet::PacketPrinter printer;
            printer.printPacket(std::cout, pPacket);
            auto data = pPacket->peekData<InstantiationApplicationRequest>();

            bool res = handleInstantiation(const_cast<InstantiationApplicationRequest*>(data.get()));

            if(res){
                // send response back to vim
                inet::L3Address vimAddress = pPacket->getTag<inet::L3AddressInd>()->getSrcAddress();
                int vimPort = pPacket->getTag<inet::L4PortInd>()->getSrcPort();
                inet::Packet* packet = new inet::Packet("InstantiationResponse");
                auto responsepck = inet::makeShared<InstantiationResponse>();
                responsepck->setAllocatedPort(portCounter);
                responsepck->setUeAppID(data->getUeAppID());
                responsepck->setChunkLength(inet::B(100));
                responsepck->setStartAllocationTime(data->getStartAllocationTime());
                packet->insertAtBack(responsepck);
                socket.sendTo(packet, vimAddress, vimPort);

                EV << "VirtualisationInfrastructureApp::handleMessage - sending back to " << vimAddress << ":" << vimPort << endl;

            }

//            cMessage* message = new cMessage("Terminazione");
//            scheduleAt(simTime()+0.2, message);

        }
        else if (!strcmp(msg->getName(), "Termination")){
            inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
            if (pPacket == 0)
                throw cRuntimeError("VirtualisationInfrastructureApp::handleMessage - FATAL! Error when casting to inet packet");

            auto data = pPacket->peekData<DeleteAppMessage>();

            bool res = handleTermination(const_cast<DeleteAppMessage*>(data.get()));

            inet::L3Address vimAddress = pPacket->getTag<inet::L3AddressInd>()->getSrcAddress();
            int vimPort = pPacket->getTag<inet::L4PortInd>()->getSrcPort();
            inet::Packet* packet = new inet::Packet("TerminationResponse");
            auto responsepck = inet::makeShared<TerminationResponse>();
            responsepck->setResponse(res);
            responsepck->setRequestId(data->getSno());
            responsepck->setUeAppID(data->getUeAppID());
            responsepck->setIsMigrating(data->isMigrating()); //migration
            responsepck->setAppInstanceId(data.get()->getAppInstanceId());
            responsepck->setChunkLength(inet::B(100));
            packet->insertAtBack(responsepck);
            socket.sendTo(packet, vimAddress, vimPort);

            EV << "VirtualisationInfrastructureApp::handleMessage - sending back to " << vimAddress << ":" << vimPort << endl;
        }
        else if(strcmp(msg->getName(), "endTerminationProcedure") == 0)
        {
           handleEndTerminationProcedure(msg);
           std::cout << "here " << simTime() << endl;
           if(strcmp(getParentModule()->getName(), "vim") != 0 && appcounter == 0 && resourceApp->getState() == RELEASING){
               emit(parkingReleased_, getParentModule()->getName());
           }
        }
        delete msg;
    }


}

int VirtualisationInfrastructureApp::getHostedAppNum(){
    return appcounter;
}


bool VirtualisationInfrastructureApp::handleInstantiation(InstantiationApplicationRequest* data)
{
    EV << "VirtualisationInfrastructureApp::handleInstantiation - " << data << endl;
    std::cout << "viapp with id " << getId() << " " << data->getContextId() << endl;
    appcounter++;
    maxappcounter++;
    portCounter++;

    // Creation of requested app
    char* meModuleName = (char*)data->getMEModuleName();
    cModuleType* moduleType = cModuleType::get(data->getMEModuleType());
    cModule* module = moduleType->create(meModuleName, getParentModule());
    std::stringstream appName;
    appName << meModuleName << "[" <<  data->getContextId() << "]";
    module->setName(appName.str().c_str());
    EV << "VirtualisationInfrastructureApp::handleInstantiation - meModuleName: " << appName.str() << endl;
    EV << "VirtualisationInfrastructureApp::mp1 - address: " << data->getMp1Address() << ":" << data->getMp1Port() << endl;

    double ram = data->getRequiredRam();
    double disk = data->getRequiredDisk();
    double cpu = data->getRequiredCpu();

    //displaying ME App dynamically created (after 70 they will overlap..)
    std::stringstream display;
    display << "p=" << posx + (appcounter*300) << "," << posy << ";i=block/control";
    module->setDisplayString(display.str().c_str());

    module->par("mecAppId") = data->getUeAppID();
    module->par("requiredRam") = ram;
    module->par("requiredDisk") = disk;
    module->par("requiredCpu") = cpu;
    module->par("localUePort") = portCounter;
    module->par("mp1Address") = data->getMp1Address();
    module->par("mp1Port") = data->getMp1Port();
    module->par("isMigrating") = data->isMigrating();

    module->finalizeParameters();

    cModule *at = getParentModule()->getSubmodule("at");
    cGate* newAtInGate = at->getOrCreateFirstUnconnectedGate("in", 0, false, true);
    cGate* newAtOutGate = at->getOrCreateFirstUnconnectedGate("out", 0, false, true);

    newAtOutGate->connectTo(module->gate("socketIn"));
    module->gate("socketOut")->connectTo(newAtInGate);

    // Add gates for vi interaction
    cGate* newAppInGate = this->getOrCreateFirstUnconnectedGate("appGates$i", 0, false, true);
    cGate* newAppOutGate = this->getOrCreateFirstUnconnectedGate("appGates$o", 0, false, true);

    newAppOutGate->connectTo(module->gate("viAppGate$i"));
    module->gate("viAppGate$o")->connectTo(newAppInGate);

    module->buildInside();
    module->scheduleStart(simTime());
    std::cout << "VirtualisationInfrastructureApp::handleInstantiation - before initialization "<< endl;
    module->callInitialize();

    std::cout << "VirtualisationInfrastructureApp::handleInstantiation - initialization "<< endl;

    RunningAppEntry* entry = new RunningAppEntry();
    entry->module = module;
    entry->port = portCounter;
    entry->inputGate = newAppInGate;
    entry->outputGate = newAppOutGate;
    entry->ueAppID = data->getUeAppID();
    entry->moduleName = data->getMEModuleName();
    entry->moduleType = data->getMEModuleType();
    entry->requiredService = data->getRequiredService();
    entry->resources.cpu = data->getRequiredCpu();
    entry->resources.ram = data->getRequiredRam();
    entry->resources.disk = data->getRequiredDisk();
    runningApp[data->getUeAppID()] = *entry;
    allocatedCpu += cpu;
    allocatedRam += ram;
    allocatedDisk += disk;

    return true;
}

bool VirtualisationInfrastructureApp::handleTermination(DeleteAppMessage* data)
{
    EV << "VirtualisationInfrastructureApp::handleTermination - " << data << endl;
    std::cout << "VirtualisationInfrastructureApp::handleTermination - " << data << endl;


    auto it = runningApp.find(data->getUeAppID());
    if(it == runningApp.end())
        return false;

    RunningAppEntry entry = it->second;
//    cModule* module = entry.module;
//    std::cout << module << endl;
//    module->callFinish();
//    toDelete = module;
//    cMessage *msg = new cMessage("deleteModule");
//    scheduleAt(simTime()+0.1, msg);

    cGate* gate = entry.outputGate;
    cMessage* msg = new cMessage("startTerminationProcedure");
    send(msg, gate->getName(), gate->getIndex());


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
            double currentSpeed = ueApp->second.resources.cpu *(maxCpu/allocatedCpu);
            time = numOfInstructions/currentSpeed;
        }
        else
        {
            double currentSpeed = ueApp->second.resources.cpu;
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

void VirtualisationInfrastructureApp::handleEndTerminationProcedure(cMessage* msg)
{
    EV << "VirtualisationInfrastructureApp::handleMessage - delete module! " << msg->getArrivalGate()->getIndex() <<endl;
    int index = msg->getArrivalGate()->getIndex();

    RunningAppEntry* selected = nullptr;
    for(auto it = runningApp.begin(); it != runningApp.end(); ++it){
        RunningAppEntry entry = (it->second);

        if(entry.inputGate->getIndex() == index){
            selected = &entry;
            break;
        }
    }

    if(selected != nullptr){

        appcounter--;
        runningApp.erase(selected->ueAppID);
        allocatedCpu -= selected->resources.cpu;
        allocatedRam -= selected->resources.ram;
        allocatedDisk -= selected->resources.disk;

//        cModule* module = selected->module;
//        module->callFinish();
//
//        toDelete = module;
//        terminatingModules.push(module);
//        if(!deleteModuleMessage->isScheduled())
//        {
//            scheduleAt(simTime()+0.1, deleteModuleMessage);
//        }
     }

    return;
}

void VirtualisationInfrastructureApp::handleModuleRemoval(cMessage*)
{
    if(terminatingModules.size() != 0)
    {
        std::cout << "Removing module " << terminatingModules.size() << endl;
        cModule *module = terminatingModules.front();
        EV << "VirtualisationInfrastructureApp::Removing module from car" << endl;
        module->deleteModule();
        std::cout << "module deleted" << endl;
        terminatingModules.pop();
        EV << "VirtualisationInfrastructureApp::Module removed. Still to be removed: " << terminatingModules.size() << endl;
        std::cout << "Module removed " << endl;
        if(!deleteModuleMessage->isScheduled() && terminatingModules.size() > 0)
            scheduleAt(simTime()+0.1, deleteModuleMessage);
    }

}

void VirtualisationInfrastructureApp::finish(){
    std::cout << "finish called" << endl;
    emit(numApp_, maxappcounter);
}


