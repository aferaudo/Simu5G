//
//  VirtualisationInfrastructureManagerDyn.cc
//
//  @author Angelo Feraudo
//  @author Alessandro Calvio
//

// Imports
#include <sstream>

#include "nodes/mec/VirtualisationInfrastructureManager/Dynamic/VirtualisationInfrastructureManagerDyn.h"
#include "nodes/mec/VirtualisationInfrastructureManager/Dynamic/RegistrationPacket_m.h"

Define_Module(VirtualisationInfrastructureManagerDyn);

VirtualisationInfrastructureManagerDyn::VirtualisationInfrastructureManagerDyn()
{

}

void VirtualisationInfrastructureManagerDyn::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {

        EV << "VirtualisationInfrastructureManagerDyn::initialize - stage " << stage << endl;

        // Init parameters
        vimHost = getParentModule()->getParentModule();
        host = getParentModule(); // Used to add brokerGates

        //Broker settings
        brokerPort = par("brokerPort");
        localToBrokerPort = par("localBrokerPort");
        radius = par("radius");

        // Meo settings
        meoPort = par("meoPort").intValue();

        // Mepm settings
        mepmPort = par("mepmPort").intValue();

        std::cout << getParentModule()->getSubmodule("interfaceTable") << endl;
        ifacetable = check_and_cast<inet::InterfaceTable*>(getParentModule()->getSubmodule("interfaceTable"));

        // Graphic
        color = getParentModule()->getParentModule()->par("color").stringValue();
        cDisplayString& dispStr = getParentModule()->getParentModule()->getDisplayString();
        dispStr.setTagArg("b", 0, 50);
        dispStr.setTagArg("b", 1, 50);
        dispStr.setTagArg("b", 2, "rect");
        dispStr.setTagArg("b", 3, color.c_str());
        dispStr.setTagArg("b", 4, "black");
    }

    inet::ApplicationBase::initialize(stage);
}

void VirtualisationInfrastructureManagerDyn::handleStartOperation(inet::LifecycleOperation *operation)
{
    EV << "VirtualisationInfrastructureManagerDyn::START.." << endl;
    std::cout << "VirtualisationInfrastructureManagerDyn::START.." << endl;

    // Set up UDP socket for communcation
    localAddress = inet::L3AddressResolver().resolve(par("localAddress"));
    EV << "VirtualisationInfrastructureManagerDyn::initialize - localAddress " << localAddress << endl;
    port = par("localPort");
    EV << "VirtualisationInfrastructureManagerDyn::initialize - binding socket to port " << port << endl;
    if (port != -1)
    {
        socket.setOutputGate(gate("socketOut"));
        socket.bind(port);
    }



    // Broker settings
    brokerIPAddress = inet::L3AddressResolver().resolve(par("brokerAddress").stringValue());

    // Meo settings
    meoAddress = inet::L3AddressResolver().resolve(par("meoAddress").stringValue());

    // Mepm settings
    mepmAddress = inet::L3AddressResolver().resolve(par("mepmAddress").stringValue());
    EV << "VirtualisationInfrastructureManagerDyn::initialize - magic ip " << mepmAddress << endl;

    initResource();

    printResources();

//  Register message
    scheduleAt(simTime()+0.01, new cMessage("register"));

    // Print message
//    cMessage *print = new cMessage("print");
//    scheduleAt(simTime()+0.1, print);

    // unsub message - TEST
//    cMessage *unsub = new cMessage("unsub");
//    scheduleAt(simTime()+2, unsub);

    // set tcp socket VIM <--> Broker
    SubscriberBase::handleStartOperation(operation);

}

void VirtualisationInfrastructureManagerDyn::handleMessageWhenUp(omnetpp::cMessage *msg)
{
    EV << "VirtualisationInfrastructureManagerDyn::handleMessage - message received! " << msg->getName() << endl;
    std::cout << "VirtualisationInfrastructureManagerDyn::handleMessage - message received! " << msg->getName() << endl;
    if (msg->isSelfMessage() && strcmp(msg->getName(), "connect") != 0)
    {
        std::cout << "VirtualisationInfrastructureManagerDyn::handleMessage - dentro" << endl;
        if(strcmp(msg->getName(), "print") == 0){
            EV << "VirtualisationInfrastructureManagerDyn::handleMessage - self message received!" << endl;
            printResources();
            allocateResources(1000,1000,1000, getId());
            printResources();
            deallocateResources(1000,1000,1000, getId());
            printResources();
            int addedHostId = registerHost(5555,2222,2222,2222,inet::L3Address("192.168.10.10"), 7890);
            EV << "VirtualisationInfrastructureManagerDyn::handleMessage - bestHost " << findBestHostDyn(1000,1000,1000) << endl;
            unregisterHost(addedHostId);
        }
        else if(strcmp(msg->getName(), "register") == 0){
            // Register to MECOrchestrator
            std::cout << "VirtualisationInfrastructureManagerDyn::handleMessage - register to MEO" << endl;
            std::cout << "MecPlatformManagerDyn::sending custom packet to MEO " << meoAddress.str() << endl;
            std::cout << "MecPlatformManagerDyn::sending custom packet to MEO " << meoPort << endl;
            sendMEORegistration();
        }
        delete msg;
    }
    else if(!msg->isSelfMessage() && socket.belongsToSocket(msg)){
        EV << "VirtualisationInfrastructureManagerDyn::handleMessage - other message received!" << endl;

           // TODO remove after the implementation of the publisher
//        if (!strcmp(msg->getName(), "Register")){
//            EV << "VirtualisationInfrastructureManagerDyn::handleMessage - TYPE: Register" << endl;
//
//            inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
//            if (pPacket == 0)
//               throw cRuntimeError("VirtualisationInfrastructureManagerDyn::handleMessage - FATAL! Error when casting to inet packet");
//
//            auto data = pPacket->peekData<RegistrationPacket>();
//            //        inet::PacketPrinter printer;
//            //        printer.printPacket(std::cout, pPacket);
//
//            registerHost(data->getRam(),data->getDisk(),data->getCpu(),data->getAddress());
//
//            getParentModule()->bubble("Host Registrato");
//            delete msg;
        if (!strcmp(msg->getName(), "InstantiationResponse")){
            EV << "VirtualisationInfrastructureManagerDyn::handleMessage - TYPE: InstantiationResponse" << endl;

            inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
            if (pPacket == 0)
               throw cRuntimeError("VirtualisationInfrastructureManagerDyn::handleMessage - FATAL! Error when casting to inet packet");

            auto data = pPacket->peekData<InstantiationResponse>();
//            inet::PacketPrinter printer;
//            printer.printPacket(std::cout, pPacket);

            int ueAppID = data->getUeAppID();
            int port = data->getAllocatedPort();

            auto it = waitingInstantiationRequests.find(std::to_string(ueAppID));
            if(it == waitingInstantiationRequests.end()){
                throw cRuntimeError("VirtualisationInfrastructureManagerDyn::handleMessage - InstantiationResponse - cannot find registered app");
            }

            MecAppEntryDyn entry = it->second;
            entry.endpoint.port = port;
            handledApp[std::to_string(ueAppID)] = entry;
            waitingInstantiationRequests.erase(it);

            //Allocate resources
            int host_key = findHostIDByAddress(entry.endpoint.addr);
            HostDescriptor* host = &((handledHosts.find(host_key))->second);

            if(port == -1){
                host->numRunningApp -= 1;
                return;
            }

            releaseResources(entry.usedResources.ram, entry.usedResources.disk, entry.usedResources.cpu, host_key);
            allocateResources(entry.usedResources.ram, entry.usedResources.disk, entry.usedResources.cpu, host_key);

//            host->numRunningApp += 1;

            EV << "VirtualisationInfrastructureManagerDyn:: response - print" << endl;
            printResources();

//            printRequests();
//            printHandledApp();

            // Response to Mepm
            inet::Packet* toSend = new inet::Packet("instantiationApplicationResponse");
            auto responsePkt = inet::makeShared<InstantiationApplicationResponse>();
            responsePkt->setStatus(port != -1);
            responsePkt->setMecHostId(getParentModule()->getParentModule()->getId());
            responsePkt->setAppName(entry.moduleName.c_str()); // send back module name to avoid findingLoop
            responsePkt->setDeviceAppId(std::to_string(ueAppID).c_str());
            std::stringstream appName;
            appName << entry.moduleName << "[" <<  entry.contextID << "]";
            responsePkt->setInstanceId(appName.str().c_str());
            responsePkt->setMecAppRemoteAddress(entry.endpoint.addr);
            responsePkt->setMecAppRemotePort(entry.endpoint.port);
            responsePkt->setContextId(entry.contextID);
            responsePkt->setChunkLength(inet::B(1000));
            toSend->insertAtBack(responsePkt);
            toSend->addTag<inet::InterfaceReq>()->setInterfaceId(ifacetable->findInterfaceByName("pppIfRouter")->getInterfaceId());
            socket.sendTo(toSend, mepmAddress, mepmPort);

            delete msg;
        }else if (!strcmp(msg->getName(), "TerminationResponse")){
            EV << "VirtualisationInfrastructureManagerDyn::handleMessage - TYPE: TerminationResponse" << endl;

            inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
            if (pPacket == 0)
               throw cRuntimeError("VirtualisationInfrastructureManagerDyn::handleMessage - FATAL! Error when casting to inet packet");

            auto data = pPacket->peekData<TerminationResponse>();
            inet::PacketPrinter printer;
            printer.printPacket(std::cout, pPacket);

            int ueAppID = data->getUeAppID();
            auto it = handledApp.find(std::to_string(ueAppID));
            if(it == handledApp.end()){
                throw cRuntimeError("VirtualisationInfrastructureManagerDyn::handleMessage - TerminationResponse - cannot find registered app");
            }
            MecAppEntryDyn entry = it->second;
            int host_key = findHostIDByAddress(entry.endpoint.addr);
            HostDescriptor* host = &((handledHosts.find(host_key))->second);
            host->numRunningApp -= 1;
            deallocateResources(entry.usedResources.ram, entry.usedResources.disk, entry.usedResources.cpu, host_key);

            EV << "VirtualisationInfrastructureManagerDyn:: response - terminate" << endl;
            printResources();

            delete msg;
        }else if(!strcmp(msg->getName(), "instantiationApplicationRequest")){
            EV << "VirtualisationInfrastructureManagerDyn::handleMessage - TYPE: instantiationApplicationRequest" << endl;

            handleMepmMessage(msg);
            delete msg;
        }
        else if (!strcmp(msg->getName(), "ResourceRequest")){
            EV << "VirtualisationInfrastructureManagerDyn::handleMessage - TYPE: ResourceRequest" << endl;

            inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
            handleResourceRequest(pPacket);
            delete msg;
        }
    }
    else{
        EV << "VirtualisationInfrastructureManagerDyn::handleMessage - TCP message received!" << endl;
        SubscriberBase::handleMessageWhenUp(msg);
    }

//    delete msg;
}

int VirtualisationInfrastructureManagerDyn::registerHost(int host_id, double ram, double disk, double cpu, inet::L3Address ip_addr, int viPort)
{
    // TODO add viPort
    EV << "VirtualisationInfrastructureManagerDyn::registerHost" << endl;

    auto it = handledHosts.find(host_id);
    if(it != handledHosts.end()){
        EV << "VirtualisationInfrastructureManagerDyn::registerHost - Host already exists!" << endl;
        return -1;
    }

    HostDescriptor* descriptor = new HostDescriptor();
    ResourceDescriptor* totalResources = new ResourceDescriptor();
    totalResources->ram = ram;
    totalResources->cpu = cpu;
    totalResources->disk = disk;

    ResourceDescriptor* usedResources = new ResourceDescriptor();
    usedResources->ram = 0;
    usedResources->cpu = 0;
    usedResources->disk = 0;

    ResourceDescriptor* reservedResources = new ResourceDescriptor();
    reservedResources->ram = 0;
    reservedResources->cpu = 0;
    reservedResources->disk = 0;

    descriptor->totalAmount = *totalResources;
    descriptor->usedAmount = *usedResources;
    descriptor->reservedAmount = *reservedResources;
    descriptor->numRunningApp = 0;
    descriptor->address = ip_addr;
    descriptor->viPort = viPort;

    handledHosts[host_id] = *descriptor;

    printResources();

    // Add circle around host
    cModule* module = inet::L3AddressResolver().findHostWithAddress(ip_addr);
    if(module != nullptr){
        cDisplayString& dispStr = module->getDisplayString();
        dispStr.setTagArg("b", 0, 50);
        dispStr.setTagArg("b", 1, 50);
        dispStr.setTagArg("b", 2, "oval");
        dispStr.setTagArg("b", 3, color.c_str());
        dispStr.setTagArg("b", 4, "black");
//        dispStr.setTagArg("b", 5, 4);
//        EV << "VirtualisationInfrastructureManagerDyn::registerHost - display string of host " << ip_addr << " is " << dispStr << endl;
    }


    return host_id;
}

void VirtualisationInfrastructureManagerDyn::unregisterHost(int host_id)
{
    EV << "VirtualisationInfrastructureManagerDyn::unregisterHost" << endl;
    auto it = handledHosts.find(host_id);
    if(it == handledHosts.end()){
        EV_ERROR << "VirtualisationInfrastructureManagerDyn::unregisterHost - Host doesn't exists!" << endl;
        return;
    }

    printResources();

    // Remove red circle around host
    HostDescriptor* host = &(it->second);
    cModule* module = inet::L3AddressResolver().findHostWithAddress(host->address);
    if(module != nullptr){
        cDisplayString& dispStr = module->getDisplayString();
        dispStr.removeTag("b");
    }

    handledHosts.erase(it);
}

bool VirtualisationInfrastructureManagerDyn::isAllocable(double ram, double disk, double cpu)
{
    std::list<HostDescriptor> hosts = getAllHosts();
    bool available = false;

    for(auto it = hosts.begin(); it != hosts.end() && available == false; ++it){
        available = ram < it->totalAmount.ram - it->usedAmount.ram - it->reservedAmount.ram
                    && disk < it->totalAmount.disk - it->usedAmount.disk - it->reservedAmount.disk
                    && cpu  < it->totalAmount.cpu - it->usedAmount.cpu - it->reservedAmount.cpu;
    }

    return available;
}

void VirtualisationInfrastructureManagerDyn::allocateResources(double ram, double disk, double cpu, int hostId)
{
    auto it = handledHosts.find(hostId);
    if(it == handledHosts.end()){
        EV << "VirtualisationInfrastructureManagerDyn::allocateResources - Host not found!" << endl;
    }
    EV << "VirtualisationInfrastructureManagerDyn::allocateResources - Allocating resources on " <<  hostId << endl;

    HostDescriptor* host = &(it->second);

    host->usedAmount.ram += ram;
    host->usedAmount.disk += disk;
    host->usedAmount.cpu += cpu;
}

void VirtualisationInfrastructureManagerDyn::deallocateResources(double ram, double disk, double cpu, int hostId)
{
    auto it = handledHosts.find(hostId);
    if(it == handledHosts.end()){
        EV << "VirtualisationInfrastructureManagerDyn::deallocateResources - Host not found!" << endl;
    }
    EV << "VirtualisationInfrastructureManagerDyn::deallocateResources - Deallocating resources on " <<  hostId << endl;

    HostDescriptor* host = &(it->second);

    host->usedAmount.ram -= ram;
    host->usedAmount.disk -= disk;
    host->usedAmount.cpu -= cpu;
}

void VirtualisationInfrastructureManagerDyn::reserveResources(double ram, double disk, double cpu, int hostId)
{
    auto it = handledHosts.find(hostId);
    if(it == handledHosts.end()){
        EV << "VirtualisationInfrastructureManagerDyn::reserveResources - Host not found!" << endl;
    }
    EV << "VirtualisationInfrastructureManagerDyn::reserveResources - Reserving resources on " <<  hostId << endl;

    HostDescriptor* host = &(it->second);

    host->reservedAmount.ram += ram;
    host->reservedAmount.disk += disk;
    host->reservedAmount.cpu += cpu;
}

void VirtualisationInfrastructureManagerDyn::releaseResources(double ram, double disk, double cpu, int hostId)
{
    auto it = handledHosts.find(hostId);
    if(it == handledHosts.end()){
        EV << "VirtualisationInfrastructureManagerDyn::releaseResources - Host not found!" << endl;
    }
    EV << "VirtualisationInfrastructureManagerDyn::releaseResources - Releasing resources on " <<  hostId << endl;

    HostDescriptor* host = &(it->second);

    host->reservedAmount.ram -= ram;
    host->reservedAmount.disk -= disk;
    host->reservedAmount.cpu -= cpu;
}

int VirtualisationInfrastructureManagerDyn::findBestHostDyn(double ram, double disk, double cpu)
{
    EV << "VirtualisationInfrastructureManagerDyn::findBestHostDyn - Start" << endl;

    int besthost_key = -1;

    const char * searchType = par("searchType").stringValue();
    if(std::strcmp(searchType, "BEST_FIRST") == 0){
        besthost_key = findBestHostDynBestFirst(ram, disk, cpu);
    }
    else if(std::strcmp(searchType, "ROUND_ROBIN") == 0){
        besthost_key = findBestHostDynRoundRobin(ram, disk, cpu);
    }
    else{
        throw cRuntimeError("VirtualisationInfrastructureManagerDyn::findBestHostDyn - search type not supported");
    }

    return besthost_key;
}

MecAppInstanceInfo* VirtualisationInfrastructureManagerDyn::instantiateMEApp(const CreateAppMessage* msg)
{
    EV << "VirtualisationInfrastructureManagerDyn::instantiateMEApp - Start" << endl;

//    Enter_Method_Silent();

    double ram = msg->getRequiredRam();
    double cpu = msg->getRequiredCpu();
    double disk = msg->getRequiredDisk();

    int bestHostKey = findBestHostDyn(ram,disk,cpu);
    if(bestHostKey == -1){
        std::cout << "VirtualisationInfrastructureManagerDyn::instantiateMEApp sending false response" << endl;
        // Response to Mepm
        inet::Packet* toSend = new inet::Packet("instantiationApplicationResponse");
        auto responsePkt = inet::makeShared<InstantiationApplicationResponse>();
        responsePkt->setStatus(false);
        responsePkt->setMecHostId(getParentModule()->getParentModule()->getId());
        responsePkt->setDeviceAppId(std::to_string(msg->getUeAppID()).c_str());
        responsePkt->setChunkLength(inet::B(1000));
        toSend->insertAtBack(responsePkt);
        socket.sendTo(toSend, mepmAddress, mepmPort);

        return nullptr;
    }

    HostDescriptor* bestHost = &handledHosts[bestHostKey];
    inet::L3Address bestHostAddress = bestHost->address;
    int bestHostPort = bestHost->viPort;

    // Reserve resources on selected host
    reserveResources(msg->getRequiredRam(), msg->getRequiredDisk(), msg->getRequiredCpu(), bestHostKey);

    inet::Packet* packet = new inet::Packet("Instantiation");
    auto registrationpck = inet::makeShared<CreateAppMessage>();

    registrationpck->setUeAppID(msg->getUeAppID());
    registrationpck->setMEModuleName(msg->getMEModuleName());
    registrationpck->setMEModuleType(msg->getMEModuleType());
    registrationpck->setRequiredCpu(msg->getRequiredCpu());
    registrationpck->setRequiredRam(msg->getRequiredRam());
    registrationpck->setRequiredDisk(msg->getRequiredDisk());
    registrationpck->setRequiredService(msg->getRequiredService());
    registrationpck->setContextId(msg->getContextId());
    registrationpck->setChunkLength(inet::B(2000));
    packet->insertAtBack(registrationpck);

    EV << "VirtualisationInfrastructureManagerDyn:: instantiateMEApp - sending to " << bestHostAddress << ":" << bestHostPort << endl;

    // request registration
    requestCounter++;
    MecAppEntryDyn newAppEntry;

    newAppEntry.isBuffered = false;
    newAppEntry.moduleName = msg->getMEModuleName();
    newAppEntry.moduleType = msg->getMEModuleType();
    newAppEntry.endpoint.addr = bestHostAddress;
    newAppEntry.endpoint.port = -1;
    newAppEntry.ueAppID = msg->getUeAppID();
    newAppEntry.contextID = msg->getContextId();
    newAppEntry.usedResources.ram  = msg->getRequiredRam();
    newAppEntry.usedResources.disk = msg->getRequiredDisk();
    newAppEntry.usedResources.cpu  = msg->getRequiredCpu();
    waitingInstantiationRequests[std::to_string(msg->getUeAppID())] = newAppEntry;

    socket.sendTo(packet, bestHostAddress, bestHostPort);

    bestHost->numRunningApp += 1;

    MecAppInstanceInfo* appInfo = new MecAppInstanceInfo();
    appInfo->status = true;
    appInfo->instanceId = msg->getMEModuleName(); //TODO Cambiare con un id dell'istanza creata (?)
    appInfo->endPoint.addr = bestHostAddress;
    appInfo->endPoint.port = bestHostPort;

    EV << "VirtualisationInfrastructureManagerDyn:: instantiateMEApp - print" << endl;
    printResources();
//    printRequests();

    return appInfo;
}

bool VirtualisationInfrastructureManagerDyn::instantiateEmulatedMEApp(CreateAppMessage*)
{
    EV << "VirtualisationInfrastructureManagerDyn:: instantiateEmulatedMEApp" << endl;

    return true;
}

bool VirtualisationInfrastructureManagerDyn::terminateMEApp(DeleteAppMessage* msg)
{
    EV << "VirtualisationInfrastructureManagerDyn:: terminateMEApp" << endl;

    Enter_Method_Silent();


    inet::Packet* packet = new inet::Packet("Termination");
    auto terminationpck = inet::makeShared<DeleteAppMessage>();

    int ueAppID = msg->getUeAppID();
    EV << "VirtualisationInfrastructureManagerDyn:: terminateMEApp - looking for " << ueAppID << " ID" << endl;
    auto it = handledApp.find(std::to_string(ueAppID));
    if(it == handledApp.end()){
        EV << "VirtualisationInfrastructureManagerDyn:: terminateMEApp - App not found" << endl;
        return false;
    }

    MecAppEntryDyn instantiatedApp = it->second;
    inet::L3Address address = instantiatedApp.endpoint.addr;
    int port = 2222; // TODO Load this from viPort

    terminationpck->setUeAppID(ueAppID);
    terminationpck->setChunkLength(inet::B(2000));
    packet->insertAtBack(terminationpck);

    EV << "VirtualisationInfrastructureManagerDyn:: terminateMEApp - sending to " << address << ":" << port <<endl;

    socket.sendTo(packet, address, port);

    return true;
}

bool VirtualisationInfrastructureManagerDyn::terminateEmulatedMEApp(DeleteAppMessage*)
{
    EV << "VirtualisationInfrastructureManagerDyn:: terminateEmulatedMEApp" << endl;

    return true;
}


void VirtualisationInfrastructureManagerDyn::printResources()
{
    EV << "VirtualisationInfrastructureManagerDyn::printResources" << endl;
    for(auto it = handledHosts.begin(); it != handledHosts.end(); ++it){
        int key = it->first;
        HostDescriptor descriptor = (it->second);

        EV << "VirtualisationInfrastructureManagerDyn::printResources - HOST: " << key << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printResources - IP Address: " << descriptor.address << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printResources - allocated Ram: " << descriptor.usedAmount.ram << " / " << descriptor.totalAmount.ram << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printResources - allocated Disk: " << descriptor.usedAmount.disk << " / " << descriptor.totalAmount.disk << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printResources - allocated CPU: " << descriptor.usedAmount.cpu << " / " << descriptor.totalAmount.cpu << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printResources - reserved Ram: " << descriptor.reservedAmount.ram << " / " << descriptor.totalAmount.ram << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printResources - reserved Disk: " << descriptor.reservedAmount.disk << " / " << descriptor.totalAmount.disk << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printResources - reserved CPU: " << descriptor.reservedAmount.cpu << " / " << descriptor.totalAmount.cpu << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printResources - running app: " << descriptor.numRunningApp << endl;
    }

    EV << "VirtualisationInfrastructureManagerDyn:: BUFFER" << endl;
    EV << "VirtualisationInfrastructureManagerDyn::printResources - allocated Ram: " << usedBufferResources->ram << " / " << totalBufferResources->ram << endl;
    EV << "VirtualisationInfrastructureManagerDyn::printResources - allocated Disk: " << usedBufferResources->disk << " / " << totalBufferResources->disk << endl;
    EV << "VirtualisationInfrastructureManagerDyn::printResources - allocated CPU: " << usedBufferResources->cpu << " / " << totalBufferResources->cpu << endl;
}

void VirtualisationInfrastructureManagerDyn::printHandledApp()
{
    EV << "VirtualisationInfrastructureManagerDyn::printHandledApp" << endl;
    for(auto it = handledApp.begin(); it != handledApp.end(); ++it){
        std::string key = it->first;
        MecAppEntryDyn entry = (it->second);

        EV << "VirtualisationInfrastructureManagerDyn::printHandledApp - APP ID: " << key << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printHandledApp - Endpoint: " << entry.endpoint.str() << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printHandledApp - Module name: " << entry.moduleName << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printHandledApp - Buffered: " << entry.isBuffered << endl;
    }
}

void VirtualisationInfrastructureManagerDyn::printRequests()
{
    EV << "VirtualisationInfrastructureManagerDyn::printRequests" << endl;
    for(auto it = waitingInstantiationRequests.begin(); it != waitingInstantiationRequests.end(); ++it){
        std::string key = it->first;
        MecAppEntryDyn entry = (it->second);

        EV << "VirtualisationInfrastructureManagerDyn::printRequests - APP ID: " << key << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printRequests - Endpoint: " << entry.endpoint.str() << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printRequests - Module name: " << entry.moduleName << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printRequests - Buffered: " << entry.isBuffered << endl;
    }

}



std::list<HostDescriptor> VirtualisationInfrastructureManagerDyn::getAllHosts()
{
    std::list<HostDescriptor> hosts;
    for(auto it = handledHosts.begin(); it != handledHosts.end(); ++it){
        hosts.push_front(it->second);
    }

    return hosts;
}

/*
 * Private Functions
 */

void VirtualisationInfrastructureManagerDyn::initResource(){
    // Init BUFFER
    bufferSize = par("bufferSize");
    totalBufferResources = new ResourceDescriptor();
    totalBufferResources->ram = vimHost->par("localRam").doubleValue() * bufferSize;
    totalBufferResources->cpu = vimHost->par("localCpuSpeed").doubleValue() * bufferSize;
    totalBufferResources->disk = vimHost->par("localDisk").doubleValue() * bufferSize;

    usedBufferResources = new ResourceDescriptor();
    usedBufferResources->ram = 0;
    usedBufferResources->cpu = 0;
    usedBufferResources->disk = 0;

    if(vimHost->hasPar("localRam") && vimHost->hasPar("localDisk") && vimHost->hasPar("localCpuSpeed")){
        HostDescriptor* descriptor = new HostDescriptor();
        ResourceDescriptor* totalResources = new ResourceDescriptor();
        totalResources->ram = vimHost->par("localRam").doubleValue() * (1-bufferSize);
        totalResources->cpu = vimHost->par("localCpuSpeed").doubleValue() * (1-bufferSize);
        totalResources->disk = vimHost->par("localDisk").doubleValue() * (1-bufferSize);

        ResourceDescriptor* usedResources = new ResourceDescriptor();
        usedResources->ram = 0;
        usedResources->cpu = 0;
        usedResources->disk = 0;

        ResourceDescriptor* reservedResources = new ResourceDescriptor();
        reservedResources->ram = 0;
        reservedResources->cpu = 0;
        reservedResources->disk = 0;

        descriptor->totalAmount = *totalResources;
        descriptor->usedAmount = *usedResources;
        descriptor->reservedAmount = *reservedResources;
        descriptor->numRunningApp = 0;
        descriptor->address = localAddress;
        descriptor->viPort = 2222;

        // using unique componentId - omnet++ feature
        int key = getId();
        handledHosts[key] = *descriptor;
    }
    else
        throw cRuntimeError("VirtualisationInfrastructureManagerDyn::initialize - \tFATAL! Cannot find static resource definition!");
}

int VirtualisationInfrastructureManagerDyn::findBestHostDynBestFirst(double ram, double disk, double cpu){

    EV << "VirtualisationInfrastructureManagerDyn::findBestHostDyn [best first] - Start" << endl;

    for(auto it = handledHosts.begin(); it != handledHosts.end(); ++it){
        int key = it->first;
        HostDescriptor descriptor = (it->second);

        if (it->first == getId()){
            continue;
        }

        bool available = ram < descriptor.totalAmount.ram - descriptor.usedAmount.ram - descriptor.reservedAmount.ram
                    && disk < descriptor.totalAmount.disk - descriptor.usedAmount.disk - descriptor.reservedAmount.disk
                    && cpu  < descriptor.totalAmount.cpu - descriptor.usedAmount.cpu - descriptor.reservedAmount.cpu;

        if(available){
            EV << "VirtualisationInfrastructureManagerDyn::findBestHostDyn [best first] - Found " << key << endl;
            return key;
        }
    }

    EV << "VirtualisationInfrastructureManagerDyn::findBestHostDyn [best first] - Best host not found!" << endl;
    return -1;
}

int VirtualisationInfrastructureManagerDyn::findBestHostDynRoundRobin(double ram, double disk, double cpu){

    EV << "VirtualisationInfrastructureManagerDyn::findBestHostDyn [round robin] - Start" << endl;

    int besthost_key = -1;
    int min = INT_MAX;
    for(auto it = handledHosts.begin(); it != handledHosts.end(); ++it){
        int key = it->first;
        HostDescriptor descriptor = (it->second);

        if (it->first == getId()){
            continue;
        }

        bool available = ram < descriptor.totalAmount.ram - descriptor.usedAmount.ram - descriptor.reservedAmount.ram
                    && disk < descriptor.totalAmount.disk - descriptor.usedAmount.disk - descriptor.reservedAmount.disk
                    && cpu  < descriptor.totalAmount.cpu - descriptor.usedAmount.cpu - descriptor.reservedAmount.cpu;

        if(available && descriptor.numRunningApp < min){
            min = descriptor.numRunningApp;
            besthost_key = key;
        }
    }

    if(besthost_key == -1){
        EV << "VirtualisationInfrastructureManagerDyn::findBestHostDyn [round robin] - Best host not found!" << endl;
        return -1;
    }

    EV << "VirtualisationInfrastructureManagerDyn::findBestHostDyn [round robin] - Found " << besthost_key << endl;

    return besthost_key;
}

HostDescriptor* VirtualisationInfrastructureManagerDyn::findHostByAddress(inet::L3Address address){

    for(auto it = handledHosts.begin(); it != handledHosts.end(); ++it){
        HostDescriptor* host = &(it->second);

        if(host->address == address){
            return host;
        }
    }

    return nullptr;
}

int VirtualisationInfrastructureManagerDyn::findHostIDByAddress(inet::L3Address address){

    for(auto it = handledHosts.begin(); it != handledHosts.end(); ++it){
        int key = it->first;
        HostDescriptor* host = &(it->second);

        if(host->address == address){
            return key;
        }
    }

    return -1;
}

bool VirtualisationInfrastructureManagerDyn::isAllocableOnBuffer(double ram, double disk, double cpu)
{
    return  ram < totalBufferResources->ram - usedBufferResources->ram
            && disk < totalBufferResources->disk - usedBufferResources->disk
            && cpu  < totalBufferResources->cpu - usedBufferResources->cpu;
}

void VirtualisationInfrastructureManagerDyn::sendMEORegistration()
{
    inet::Packet* packet = new inet::Packet("Registration");
    auto registrationPkt = inet::makeShared<RegistrationPkt>();
    registrationPkt->setHostId(getParentModule()->getParentModule()->getId());
    registrationPkt->setType(MM4);
    registrationPkt->setSourcePort(port);
    registrationPkt->setChunkLength(inet::B(1000));
    packet->insertAtBack(registrationPkt);

    EV << "VirtualisationInfrastructureManagerDyn::sending custom packet to MEO " << endl;
    std::cout << "VirtualisationInfrastructureManagerDyn::sending custom packet to MEO " << endl;
    socket.sendTo(packet, meoAddress, meoPort);

}

void VirtualisationInfrastructureManagerDyn::handleResourceRequest(inet::Packet* resourcePacket){

    auto data = resourcePacket->peekData<MeoVimRequest>();
    const MeoVimRequest *receivedData = data.get();

    double ram = receivedData->getRam();
    double cpu = receivedData->getCpu();
    double disk = receivedData->getDisk();

    bool res = isAllocable(ram, disk, cpu);

    inet::Packet* packet = new inet::Packet("AvailabilityResponseVim");
    auto responsePkt = inet::makeShared<MecHostResponse>();
    responsePkt->setDeviceAppId(receivedData->getDeviceAppId());
    responsePkt->setMecHostId(getParentModule()->getParentModule()->getId());
    responsePkt->setResult(res);
    responsePkt->setChunkLength(inet::B(1000));
    packet->insertAtBack(responsePkt);
    socket.sendTo(packet, meoAddress, meoPort);

    EV << "VirtualisationInfrastructureManagerDyn::handleResourceRequest - request result:  " << res << endl;
}

void VirtualisationInfrastructureManagerDyn::handleMepmMessage(cMessage* instantiationPacket){

    inet::Packet* pPacket = check_and_cast<inet::Packet*>(instantiationPacket);
    if (pPacket == 0)
       throw cRuntimeError("VirtualisationInfrastructureManagerDyn::handleMepmMessage - FATAL! Error when casting to inet packet");

    const CreateAppMessage* data = pPacket->peekData<CreateAppMessage>().get();

    instantiateMEApp(data);
}


void VirtualisationInfrastructureManagerDyn::manageNotification()
{
    if(currentHttpMessage->getType() == RESPONSE)
    {
        HttpResponseMessage *response = dynamic_cast<HttpResponseMessage*> (currentHttpMessage);
        EV << "VIM received a response - resources available for my zone" << endl;
        //EV << "VIM::Response received: " << response->getBody() << endl;
        nlohmann::json jsonResponseBody = nlohmann::json::parse(response->getBody());
        // We may have multiple clients
        for(nlohmann::json::iterator it = jsonResponseBody.begin(); it != jsonResponseBody.end(); ++it)
        {
            // Client
            EV << "VirtualisationInfrastructureManagerDyn::ClientID: " << it.key() << endl;
            EV << "VirtualisationInfrastructureManagerDyn::IP addr: " << (*it)["ipAddress"] << endl;
            EV << "VirtualisationInfrastructureManagerDyn::vi port: " << (*it)["viPort"] << endl;
            EV << "VirtualisationInfrastructureManagerDyn::ram: " << (*it)["ram"] << endl;
            EV << "VirtualisationInfrastructureManagerDyn::disk: " << (*it)["disk"] << endl;
            EV << "VirtualisationInfrastructureManagerDyn::cpu: " << (*it)["cpu"] << endl;
            std::string ipAddress_str = (*it)["ipAddress"];
            registerHost(std::atoi(it.key().c_str()), (*it)["ram"], (*it)["disk"], (*it)["cpu"], inet::L3Address(ipAddress_str.c_str()), (*it)["viPort"]);
        }
    }
    else if(currentHttpMessage->getType()  == REQUEST)
    {
        HttpRequestMessage *request = dynamic_cast<HttpRequestMessage*> (currentHttpMessage);
        EV << "VIM::received a request - new resource available" << endl;
        //EV << "VIM::Request received: " << request->getBody() << endl;
        std::string uri = request->getUri();
        // TODO only post request
        // use webhook
        EV << "Response body " << request->getBody() << endl;
        if(uri.compare(webHook) == 0 && std::strcmp(request->getMethod(),"POST") == 0)
        {
            nlohmann::json jsonBody = nlohmann::json::parse(request->getBody());
            nlohmann::json::iterator it = jsonBody.begin(); // just one element (TODO change without using iterator)
            EV << "VIM::Correct WebHook - registering resources" << endl;
            EV << "VirtualisationInfrastructureManagerDyn::ClientID: " << it.key() << endl;
            EV << "VirtualisationInfrastructureManagerDyn::IP addr: " << (*it)["ipAddress"] << endl;
            EV << "VirtualisationInfrastructureManagerDyn::vi port: " << (*it)["viPort"] << endl;
            EV << "VirtualisationInfrastructureManagerDyn::ram: " << (*it)["ram"] << endl;
            EV << "VirtualisationInfrastructureManagerDyn::disk: " << (*it)["disk"] << endl;
            EV << "VirtualisationInfrastructureManagerDyn::cpu: " << (*it)["cpu"] << endl;
            std::string ipAddress_str = (*it)["ipAddress"];
            registerHost(std::atoi(it.key().c_str()), (*it)["ram"], (*it)["disk"], (*it)["cpu"], inet::L3Address(ipAddress_str.c_str()), (*it)["viPort"]);
        }
        else if(std::strcmp(request->getMethod(),"DELETE") == 0)
        {
            // URI check here: webhook/id
            EV << "VIM received DELETE request!" <<  endl;
            std::string uriToCheck = uri.substr(0, uri.find(webHook) + webHook.length());
            if(uriToCheck.compare(webHook) == 0)
            {
                uri.erase(0, uri.find(webHook) + webHook.length()); //now id
                EV << "VIM::Unregister host: " << uri << endl;
                unregisterHost(std::atoi(uri.c_str()));
            }
            else
            {
                EV_ERROR << "VIM::Bad URI in releasing phase..." << endl;
            }
        }
        else
        {
            EV << "VIM:: Not recognised uri ignore..." << uri << endl;
        }
    }
    else
    {
        EV << "VIM::not recognised HTTP message" << endl;
    }
}
