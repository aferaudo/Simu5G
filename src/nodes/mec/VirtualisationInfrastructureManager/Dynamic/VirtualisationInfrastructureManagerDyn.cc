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
    handledApp.clear();
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
        subscribeURI = std::string(par("subscribeURI").stringValue());
        unsubscribeURI = subscribeURI + std::to_string(getId());
        webHook = std::string(par("webHook").stringValue());

        // Meo settings
        meoPort = par("meoPort").intValue();

        // Mepm settings
        mepmPort = par("mepmPort").intValue();

        // Mep settings
        mp1Port = par("mp1Port").intValue();

        std::cout << getParentModule()->getSubmodule("interfaceTable") << endl;
        //ifacetable = check_and_cast<inet::InterfaceTable*>(getParentModule()->getSubmodule("interfaceTable"));

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
    // Get binder
    binder_ = getBinder();

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
    // Registering VIM as MECHost component that communicates with ues
    EV << "VirtualisationInfrastructureManagerDyn::Registering upf_mechost" << endl;
    inet::L3Address gtpAddress = inet::L3AddressResolver().resolve(getParentModule()->getParentModule()->getSubmodule("upf_mec")->getFullPath().c_str());
    binder_->registerMecHostUpfAddress(localAddress, gtpAddress);
    binder_->registerMecHost(localAddress);


    // Broker settings
    brokerIPAddress = inet::L3AddressResolver().resolve(par("brokerAddress").stringValue());

    // Meo settings
    meoAddress = inet::L3AddressResolver().resolve(par("meoAddress").stringValue());

    // Mepm settings
    mepmAddress = inet::L3AddressResolver().resolve(par("mepmAddress").stringValue());
    EV << "VirtualisationInfrastructureManagerDyn::initialize - magic ip " << mepmAddress << endl;

    // Mep settings
    mp1Address = inet::L3AddressResolver().resolve(par("mp1Address").stringValue());

    // Registering mp1Address (MECPlatform) as MECHost component
    binder_->registerMecHostUpfAddress(mp1Address, gtpAddress);


    initResource();

    printResources();

    //  Register message - meo and broker
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

    if(msg->isSelfMessage() && strcmp(msg->getName(), "print") == 0){
        EV << "VirtualisationInfrastructureManagerDyn::handleMessage - self message received!" << endl;
        printResources();
        allocateResources(1000,1000,1000, getParentModule()->getId());
        printResources();
        deallocateResources(1000,1000,1000, getParentModule()->getId());
        printResources();
        int addedHostId = registerHost(5555,2222,2222,2222,inet::L3Address("192.168.10.10"), 7890);
        EV << "VirtualisationInfrastructureManagerDyn::handleMessage - bestHost " << findBestHostDyn(1000,1000,1000) << endl;
        unregisterHost(addedHostId);
        delete msg;
    }
    else if(msg->isSelfMessage() && strcmp(msg->getName(), "register") == 0){
        // Register to MECOrchestrator
        EV << "VirtualisationInfrastructureManagerDyn::subscribing to broker and registring to MEO" << endl;
        // connectToBroker
        connectToBroker();

        // send MEC Orchestrastor registration
        sendMEORegistration();
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

            handleInstantiationResponse(msg);

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
            MecAppEntryDyn entry;
            if(data->isMigrating())
            {
                // migration
                EV << "VirtualisationInfrastructureManagerDyn::TerminationResponse for migrating app" << endl;
                auto itMigrating = migratingApps.find(std::to_string(ueAppID));
                if(itMigrating == migratingApps.end())
                {
                    throw cRuntimeError("VirtualisationInfrastructureManagerDyn::handleMessage - TerminationResponse - cannot find registered app");
                }
                entry = itMigrating->second;

                migratingApps.erase(itMigrating);
                std::cout << "Dimension Migrating apps after erase " << migratingApps.size() << endl;
                std::cout << "Dimension handled apps after erase on migration " << handledApp.size() << endl;
            }
            else
            {
                //printHandledApp();
                std::cout << "An app has been terminated before find: " << handledApp.size() << endl;
                auto itHandled = handledApp.find(std::to_string(ueAppID));
                EV << "VirtualisationInfrastructureManagerDyn::TerminationResponse for normal app" << endl;
                if(itHandled == handledApp.end()){
                    throw cRuntimeError("VirtualisationInfrastructureManagerDyn::handleMessage - TerminationResponse - cannot find registered app");
                }
                entry = *(itHandled->second);


                handledApp.erase(itHandled);
                std::cout << "An app has been terminated: " << handledApp.size() << " entry: " << entry.appInstanceId << endl;
            }

            int host_key = findHostIDByAddress(entry.endpoint.addr);
            HostDescriptor* host = &((handledHosts.find(host_key))->second);
            host->numRunningApp -= 1;
            deallocateResources(entry.usedResources.ram, entry.usedResources.disk, entry.usedResources.cpu, host_key);
            unregisterHost(host_key);
            EV << "VirtualisationInfrastructureManagerDyn::handleMessage - Termination response - sending reply to orchestrator" << endl;

            inet::Packet *packet = new inet::Packet("terminationAppInstResponse");
            auto terminationResponse = inet::makeShared<TerminationAppInstResponse>();
            terminationResponse->setDeviceAppId(std::to_string(entry.ueAppID).c_str());
            terminationResponse->setContextId(entry.contextID);
            terminationResponse->setMecHostId(getParentModule()->getParentModule()->getId());
            terminationResponse->setRequestId(data->getRequestId());
            terminationResponse->setStatus(true);
            terminationResponse->setIsMigrating(data->isMigrating()); // migration
            terminationResponse->setAppInstanceId(data->getAppInstanceId());
            terminationResponse->setChunkLength(inet::B(1000));
            std::cout<< "before sending: " << handledApp.size() << " " << simTime() << endl;
            printHandledApp();
            packet->insertAtBack(terminationResponse);
            //packet->addTag<inet::InterfaceReq>()->setInterfaceId(ifacetable->findInterfaceByName("pppIfRouter")->getInterfaceId());

            socket.sendTo(packet, mepmAddress, mepmPort);

            EV << "VirtualisationInfrastructureManagerDyn:: response - terminate" << endl;
            printResources();
            std::cout<< "After sending: " << handledApp.size() << " " << simTime() << endl;
            printHandledApp();
            delete msg;
        }else if(!strcmp(msg->getName(), "instantiationApplicationRequest") || !strcmp(msg->getName(), "terminationAppInstRequest")){
            EV << "VirtualisationInfrastructureManagerDyn::handleMessage - TYPE:" << msg->getName() << endl;

            handleMepmMessage(msg);
            delete msg;
        }
        else if (!strcmp(msg->getName(), "ResourceRequest")){
            EV << "VirtualisationInfrastructureManagerDyn::handleMessage - TYPE: ResourceRequest" << endl;

            inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
            handleResourceRequest(pPacket);
            delete msg;
        }
        else if(!strcmp(msg->getName(), "ServiceMobilityRequest"))
        {
            EV << "VirtualisationInfrastructureManagerDyn::handleMessage - TYPE: ServiceMobilityRequest" << endl;
            handleMobilityRequest(msg);
        }
    }
    else{
        std::cout << "Else virtualisationinfrastracturedyn " << endl;
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

    if(it->second.numRunningApp == 0 && it->second.state == LEAVING)
    {
        // Remove red circle around host
        HostDescriptor* host = &(it->second);
        cModule* module = inet::L3AddressResolver().findHostWithAddress(host->address);
        if(module != nullptr){
            cDisplayString& dispStr = module->getDisplayString();
            dispStr.removeTag("b");
        }

        handledHosts.erase(it);

    }
    else
    {
        EV << "VirtualisationInfrastructureManagerDyn::Host cannot be deleted: there are " << it->second.numRunningApp <<" apps running..." << endl;
    }
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
    std::cout << "ALLOCATE REOURCES HAS BEEN CALLED: " << hostId << endl;
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
    std::cout << "DEALLOCATE REOURCES HAS BEEN CALLED: " << hostId << endl;
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

MecAppInstanceInfo* VirtualisationInfrastructureManagerDyn::instantiateMEApp(const InstantiationApplicationRequest* msg)
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
        //toSend->addTag<inet::InterfaceReq>()->setInterfaceId(ifacetable->findInterfaceByName("pppIfRouter")->getInterfaceId());

        socket.sendTo(toSend, mepmAddress, mepmPort);
        return nullptr;
    }

    HostDescriptor* bestHost = &handledHosts[bestHostKey];
    inet::L3Address bestHostAddress = bestHost->address;
    int bestHostPort = bestHost->viPort;
    EV << "VirtualisationInfrastructureManagerDyn::BestHost address: " << bestHost->address.str() << ", port: " << bestHost->viPort << endl;
    // Reserve resources on selected host
    reserveResources(msg->getRequiredRam(), msg->getRequiredDisk(), msg->getRequiredCpu(), bestHostKey);

    // Get mepm data for mp1 link
    //std::string mp1Address_ = inet::L3AddressResolver().addressOf(inet::L3AddressResolver().findHostWithAddress(mp1Address), "wlan").str();
    //int mp1Port_ = mp1Port;

    EV << "VirtualisationInfrastructureManagerDyn:: instantiate - MEP endpoint is " << mp1Address << ":" << mp1Port << endl;


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
    newAppEntry.ueEndpoint = msg->getUeIpAddress();
    inet::Packet* packet = createInstantiationRequest(newAppEntry, msg->getRequiredService());
    waitingInstantiationRequests[std::to_string(msg->getUeAppID())] = newAppEntry;
    std::cout<<"instantiateMEAppReq " << waitingInstantiationRequests.size() << endl;

    EV << "VirtualisationInfrastructureManagerDyn:: instantiateMEApp - sending to " << bestHostAddress << ":" << bestHostPort << endl;
    socket.sendTo(packet, bestHostAddress, bestHostPort);

    bestHost->numRunningApp += 1;

    MecAppInstanceInfo* appInfo = new MecAppInstanceInfo();
    appInfo->status = true;
    appInfo->instanceId = msg->getMEModuleName(); //TODO Cambiare con un id dell'istanza creata (?)
    appInfo->endPoint.addr = bestHostAddress;
    appInfo->endPoint.port = bestHostPort;

    EV << "VirtualisationInfrastructureManagerDyn:: instantiateMEApp - " << appInfo->instanceId << " - print" << endl;
    printResources();
//    printRequests();

    return appInfo;
}

bool VirtualisationInfrastructureManagerDyn::instantiateEmulatedMEApp(CreateAppMessage*)
{
    EV << "VirtualisationInfrastructureManagerDyn:: instantiateEmulatedMEApp" << endl;

    return true;
}

bool VirtualisationInfrastructureManagerDyn::terminateMEApp(const TerminationAppInstRequest* msg)
{
    EV << "VirtualisationInfrastructureManagerDyn:: terminateMEApp" << endl;

    //    Enter_Method_Silent();

    int ueAppID = atoi(msg->getDeviceAppId());
    bool migrated = false;
    MecAppEntryDyn *instantiatedApp = new MecAppEntryDyn();
    inet::Packet* packet = new inet::Packet("Termination");
    auto terminationpck = inet::makeShared<DeleteAppMessage>();
    std::cout << "Termination me app outside: " << handledApp.size() << endl;
    if(ueAppID == -1)
    {
        // app migrated case
        migrated = true;
        std::string appInstanceId = std::string(msg->getAppInstanceId());
        EV << "VirtualisationInfrastructureManagerDyn:: terminate migrated app - looking for " << appInstanceId << " ID" << endl;
        bool found = false;
        for(auto &app : migratingApps)
        {
            if(app.second.appInstanceId.compare(appInstanceId) == 0)
            {
                EV << "VirtualisationInfrastructureManagerDyn:: Migrating app has been found on address " << app.second.endpoint.addr.str() << endl;
                instantiatedApp = &app.second;
                ueAppID = app.second.ueAppID;
                //migratingApps.erase(app.fi);
                found = true;
                break;
            }
        }
        if(!found)
        {
            inet::Packet *packet = new inet::Packet();
            auto terminationResponse = inet::makeShared<TerminationAppInstResponse>();
            terminationResponse->setStatus(false);
            terminationResponse->setIsMigrating(migrated);
            terminationResponse->setChunkLength(inet::B(8));
            packet->insertAtBack(terminationResponse);
            socket.sendTo(packet, mepmAddress, mepmPort);
            return false;
        }

        terminationpck->setSno(0); // this parameter is actually ignored in migration case - so it can be any value
        //std::cout<<"VIM::snumber: " << terminationpck->getSno() << endl;
    }

    if(!migrated)
    {
        // standard case - looking in handled app
        EV << "VirtualisationInfrastructureManagerDyn:: terminateMEApp - looking for " << ueAppID << " ID" << endl;
        std::cout << handledApp.size() << endl;

        auto it = handledApp.find(std::to_string(ueAppID));
        std::cout << "THIS IS AN APP THAT IS NOT MIGRATING" << endl;
        std::cout << handledApp.size() << endl;
        if(it == handledApp.end()){
            EV << "VirtualisationInfrastractureManagerDyn::terminateMEApp - App not found - error" << endl;
            inet::Packet *packet = new inet::Packet();
            auto terminationResponse = inet::makeShared<TerminationAppInstResponse>();
            terminationResponse->setDeviceAppId(msg->getDeviceAppId());
            terminationResponse->setMecHostId(getParentModule()->getParentModule()->getId());
            terminationResponse->setRequestId(msg->getRequestId());
            terminationResponse->setContextId(msg->getContextId());
            terminationResponse->setStatus(false);
            terminationResponse->setChunkLength(inet::B(1000));
            packet->insertAtBack(terminationResponse);
            //packet->addTag<inet::InterfaceReq>()->setInterfaceId(ifacetable->findInterfaceByName("pppIfRouter")->getInterfaceId());
            socket.sendTo(packet, mepmAddress, mepmPort);
            return false;

        }
        instantiatedApp = it->second;
        terminationpck->setSno(msg->getRequestId());
    }



    inet::L3Address address = instantiatedApp->endpoint.addr;
    int port = 2222; // TODO Load this from viPort

    terminationpck->setUeAppID(ueAppID);
    terminationpck->setIsMigrating(migrated); // migration
    terminationpck->setAppInstanceId(msg->getAppInstanceId());
    terminationpck->setChunkLength(inet::B(8 + std::string(msg->getAppInstanceId()).size())); // 4bytes + 4bytes
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
        MecAppEntryDyn *entry = (it->second);

        EV << "VirtualisationInfrastructureManagerDyn::printHandledApp - APP ID: " << key << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printHandledApp - APP instance Id: " << entry->appInstanceId << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printHandledApp - Endpoint: " << entry->endpoint.str() << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printHandledApp - Module name: " << entry->moduleName << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printHandledApp - Buffered: " << entry->isBuffered << endl;
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
    EV << "VirtualisationInfrastructureManagerDyn::Init Resource" << endl;
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
        int key = getParentModule()->getId();
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

        if (it->first == getParentModule()->getId()){
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

//        if (it->first == getParentModule()->getId()){
//            continue;
//        }

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
        std::cout << "comparing " << host->address << " with " << address << endl;
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

void VirtualisationInfrastructureManagerDyn::handleMepmMessage(cMessage* msg){

    inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);

    if(!strcmp(msg->getName(), "instantiationApplicationRequest"))
    {
        if (pPacket == 0)
           throw cRuntimeError("VirtualisationInfrastructureManagerDyn::handleMepmMessage - FATAL! Error when casting to inet packet");

        const InstantiationApplicationRequest* data = pPacket->peekData<InstantiationApplicationRequest>().get();

        instantiateMEApp(data);
    }
    else if(!strcmp(msg->getName(), "terminationAppInstRequest"))
    {
        EV << "VirtualisationInfrastructureManagerDyn::handleMepmMessage termination MEApp started!" << endl;
        const TerminationAppInstRequest* data = pPacket->peekData<TerminationAppInstRequest>().get();
        terminateMEApp(data);
    }
}


void VirtualisationInfrastructureManagerDyn::manageNotification()
{
    if(currentHttpMessageServed_->getType() == RESPONSE)
    {
        HttpResponseMessage *response = dynamic_cast<HttpResponseMessage*> (currentHttpMessageServed_);
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
    else if(currentHttpMessageServed_->getType()  == REQUEST)
    {
        HttpRequestMessage *request = dynamic_cast<HttpRequestMessage*> (currentHttpMessageServed_);
        EV << "VIM::received a request - new resource available" << endl;
        std::string uri = request->getUri();
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
                // before unregistring the dynamic host, an event signaling
                // app migration should be generated
                EV << "VIM::finding mecapp running on that host" << endl;
                auto host = handledHosts.find(std::atoi(uri.c_str()));
                HostDescriptor *hostDesc = &(host->second);
                hostDesc->state = LEAVING;

                for(auto meAppEntry = handledApp.begin(); meAppEntry != handledApp.end(); ++meAppEntry)
                {
                    std::cout << "Inside loop????" << endl;
                    //std::cout << meAppEntry.second.appInstanceId << endl;
                    // Generate Mobility Procedure event
                    std::cout << "VIM::analysing app: " << meAppEntry->second->appInstanceId << " with address: " << meAppEntry->second->endpoint.addr << endl;
                    //EV << "VIM::analysing app: " << meAppEntry.second->appInstanceId << " with address: " << meAppEntry.second->endpoint.addr << endl;
                    if(meAppEntry->second->endpoint.addr == hostDesc->address)
                    {
                        EV << "VIM::APP " << meAppEntry->second->appInstanceId << " running on a leaving host: migration starts..." << endl;
                        mobilityTrigger(meAppEntry->second->appInstanceId);
                    }
                }
                std::cout << "OutsideLoop2" << endl;
                std::cout << handledApp.size() << endl;
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

void VirtualisationInfrastructureManagerDyn::socketEstablished(
        inet::TcpSocket *socket) {

    EV << "VirtualisationInfrastructureManagerDyn::connection established with:  " << socket->getRemoteAddress() << endl;
    // We should distinguish between two sockets
    if(socket->getRemoteAddress() == brokerIPAddress)
    {
        serverHost = tcpSocket.getRemoteAddress().str() + ":" + std::to_string(tcpSocket.getRemotePort());

        EV << "VirtualisationInfrastructureManagerDyn::Preparing subscription body" << endl;
        subscriptionBody_ = infoToJson();
        sendSubscription();
    }
}

void VirtualisationInfrastructureManagerDyn::handleMobilityRequest(cMessage* msg)
{
    /*
     * This method manages only local apps - migration from dynamic resource to
     * local resource.
     * TODO implement case where MECApp is already allocated on the buffer
     */
    EV << "VIM::handleMobilityRequest" << endl;
    inet::Packet* resourcePacket = check_and_cast<inet::Packet*>(msg);
    auto data = resourcePacket->peekData<ServiceMobilityRequest>();
    const ServiceMobilityRequest *receivedData = data.get();


    if(receivedData->getAssociateIdArraySize() == 0)
    {
        EV << "VIM::managing migration from dynamic resources!" << endl;
        for(auto value : handledApp)
        {
            if(std::strcmp(receivedData->getAppInstanceId(), value.second->appInstanceId.c_str()) == 0)
            {
                std::cout << "vim local instantiation " << receivedData->getAppInstanceId() << endl;
                EV <<  "VIM::MECApp found on host: " << value.second->endpoint.addr.str() << ":" << std::to_string(value.second->endpoint.port) << endl;
                // get partial data
                if(isAllocableOnBuffer(value.second->usedResources.ram, value.second->usedResources.disk, value.second->usedResources.cpu))
                {
                    instantiateMEAppLocally((*value.second), true);
                }
                return;
            }
        }
    }
    else
    {
        EV_ERROR << "VIM::vim does not manage associateId migration type so far.."<< endl;
        /*
         * In this scenario multiple mecapps may be migrated
         * The correspondence MECapp UE is 1 to 1 so far, thus
         * only one MECApp at time will be migrated
         */
        return;
    }

}

void VirtualisationInfrastructureManagerDyn::instantiateMEAppLocally(
         MecAppEntryDyn meapp, bool migration) {

    HostDescriptor* bestHost = &handledHosts[getParentModule()->getId()];

    EV << "VirtualisationInfrastructureManagerDyn:: instantiateMEAppLocally - " << bestHost->address.str() << ":" << bestHost->viPort << endl;


    bestHost->numRunningApp += 1;

    // FIXME the value of migration should be loaded from an upper method
    inet::Packet *packet =  createInstantiationRequest(meapp, "NULL", migration);
    // changing endpoint characteristics
    meapp.endpoint.addr = bestHost->address;
    meapp.endpoint.port = -1;
    waitingInstantiationRequests[std::to_string(meapp.ueAppID)] = meapp;
    reserveResources(meapp.usedResources.ram, meapp.usedResources.disk, meapp.usedResources.cpu, getParentModule()->getId());
    EV << "VirtualisationInfrastructureManagerDyn::create new app instance - " << meapp.appInstanceId << " -" << endl;
    socket.sendTo(packet, bestHost->address, bestHost->viPort);

}
inet::Packet* VirtualisationInfrastructureManagerDyn::createInstantiationRequest(
        MecAppEntryDyn& meapp, std::string requiredOmnetppService, bool migration)
{

    inet::Packet* packet = new inet::Packet("Instantiation");
    auto registrationpck = inet::makeShared<InstantiationApplicationRequest>();

    registrationpck->setUeAppID(meapp.ueAppID);
    registrationpck->setMEModuleName(meapp.moduleName.c_str());
    registrationpck->setMEModuleType(meapp.moduleType.c_str());
    registrationpck->setRequiredCpu(meapp.usedResources.cpu);
    registrationpck->setRequiredRam(meapp.usedResources.ram);
    registrationpck->setRequiredDisk(meapp.usedResources.disk);
    registrationpck->setRequiredService(requiredOmnetppService.c_str());
    registrationpck->setMp1Address(mp1Address.str().c_str());
    registrationpck->setMp1Port(mp1Port);
    registrationpck->setContextId(meapp.contextID);
    registrationpck->setIsMigrating(migration);
    registrationpck->setStartAllocationTime(simTime());
    registrationpck->setChunkLength(inet::B(sizeof(meapp) + mp1Address.str().size() + 16));
    packet->insertAtBack(registrationpck);

    return packet;
}

void VirtualisationInfrastructureManagerDyn::handleInstantiationResponse(
        cMessage* msg)
{
    inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
    if (pPacket == 0)
       throw cRuntimeError("VirtualisationInfrastructureManagerDyn::handleMessage - FATAL! Error when casting to inet packet");

    auto data = pPacket->peekData<InstantiationResponse>();
//            inet::PacketPrinter printer;
//            printer.printPacket(std::cout, pPacket);

    int ueAppID = data->getUeAppID();
    int port = data->getAllocatedPort();
    int packetLength = 0;
    auto it = waitingInstantiationRequests.find(std::to_string(ueAppID));
    if(it == waitingInstantiationRequests.end()){
        throw cRuntimeError("VirtualisationInfrastructureManagerDyn::handleMessage - InstantiationResponse - cannot find registered app");
    }

    MecAppEntryDyn *entry = new MecAppEntryDyn();
    entry->appInstanceId = it->second.appInstanceId;
    entry->contextID = it->second.contextID;
    entry->endpoint = it->second.endpoint;
    entry->isBuffered = false;
    entry->moduleName = it->second.moduleName;
    entry->moduleType = it->second.moduleType;
    entry->ueAppID = it->second.ueAppID;
    entry->ueEndpoint = it->second.ueEndpoint;
    entry->usedResources = it->second.usedResources;
    entry->endpoint.port = port;
    auto existingApp = handledApp.find(std::to_string(entry->ueAppID));
    if(existingApp != handledApp.end() && port != -1)
    {
        printHandledApp();
        // App migration ----
        EV << "VirtualisationInfrastructureManagerDyn::migration status: APP CREATED" << endl;
        // Next step create a method that:
        // - sends an ServiceMobilityResponse to the MEC platform manager
        // - deletes and replace the old meapp from the list (release only diminish the used resources)
        EV << "VirtualisationInfrastructureManagerDyn::new mec app address: " << entry->endpoint.addr.str() << endl;

        // Remove from the waiting queue
        waitingInstantiationRequests.erase(it);

        // isBuffered
        //entry->isBuffered = true;

        // Delete old app and adding at the map of app in migration phase (they still need context synchronization)
        EV << "VirtualisationInfrastructureManagerDyn::adding address in migrating " << existingApp->second->endpoint.addr << endl;
        migratingApps[std::to_string(existingApp->second->ueAppID)] = (*existingApp->second);
        //handledApp.erase(existingApp);

        // Add new mecApp at list of handledApp
        //handledApp[std::to_string(entry->ueAppID)] = entry;
        existingApp->second->isBuffered = true;
        existingApp->second->endpoint.addr = entry->endpoint.addr;
        existingApp->second->endpoint.port = port;
        printHandledApp();
        // sending service mobility response
        inet::Packet* packet = new inet::Packet("ServiceMobilityResponse");
        auto toSend = inet::makeShared<ServiceMobilityResponse>();
        toSend->setAppInstanceId(entry->appInstanceId.c_str());
        packetLength = packetLength + entry->appInstanceId.size();

        toSend->setTargetAddress(entry->endpoint.addr);
        toSend->setTargetPort(entry->endpoint.port);
        packetLength = packetLength + entry->endpoint.addr.str().size() + 4;

        // FIXME Correspondence one-to-one
        toSend->setAssociateIdArraySize(1);
        AssociateId associateId;
        associateId.setType("UE_IPv4_ADDRESS");
        associateId.setValue(entry->ueEndpoint.str());
        toSend->setAssociateId(0, associateId);
        packetLength = packetLength + associateId.getType().size() + associateId.getValue().size();

        toSend->setChunkLength(inet::B(packetLength));

        packet->insertAtBack(toSend);
        EV << "VirtualisationInfrastructureManagerDyn::sending application mobility response to mepm" << endl;

        releaseResources(entry->usedResources.ram, entry->usedResources.disk, entry->usedResources.cpu, getParentModule()->getId());
        allocateResources(entry->usedResources.ram, entry->usedResources.disk, entry->usedResources.cpu, getParentModule()->getId());

        socket.sendTo(packet, mepmAddress, mepmPort);

        return;

        // done
    }

    //Allocate resources
    int host_key = findHostIDByAddress(entry->endpoint.addr);
    HostDescriptor* host = &((handledHosts.find(host_key))->second);

    if(port == -1){
        host->numRunningApp -= 1;
        return;
    }

    simtime_t totalAllocationTime = simTime() - data->getStartAllocationTime();
    emit(allocationTimeSignal_, totalAllocationTime);

    releaseResources(entry->usedResources.ram, entry->usedResources.disk, entry->usedResources.cpu, host_key);
    allocateResources(entry->usedResources.ram, entry->usedResources.disk, entry->usedResources.cpu, host_key);

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
    responsePkt->setAppName(entry->moduleName.c_str()); // send back module name to avoid findingLoop
    responsePkt->setDeviceAppId(std::to_string(ueAppID).c_str());
    std::stringstream appName;
    appName << entry->moduleName << "[" <<  entry->contextID << "]";

    responsePkt->setInstanceId(appName.str().c_str());
    responsePkt->setMecAppRemoteAddress(entry->endpoint.addr);
    responsePkt->setMecAppRemotePort(entry->endpoint.port);
    responsePkt->setContextId(entry->contextID);
    responsePkt->setChunkLength(inet::B(1000));
    toSend->insertAtBack(responsePkt);
    //toSend->addTag<inet::InterfaceReq>()->setInterfaceId(ifacetable->findInterfaceByName("pppIfRouter")->getInterfaceId());

    entry->appInstanceId = appName.str();
    handledApp[std::to_string(ueAppID)] = entry;
    printHandledApp();
    waitingInstantiationRequests.erase(it);
    printHandledApp();

    socket.sendTo(toSend, mepmAddress, mepmPort);
}

void VirtualisationInfrastructureManagerDyn::mobilityTrigger(
        std::string appInstanceId) {
    inet::Packet* toSend = new inet::Packet("ParkMigrationTrigger");
    auto trigger = inet::makeShared<ParkMigrationTrigger>();
    trigger->setAppInstanceId(appInstanceId.c_str());
    trigger->setChunkLength(inet::B(appInstanceId.size()));
    toSend->insertAtBack(trigger);

    socket.sendTo(toSend, mepmAddress, mepmPort);

}
