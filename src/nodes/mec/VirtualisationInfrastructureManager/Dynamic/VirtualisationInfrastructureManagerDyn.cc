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

            descriptor->totalAmount = *totalResources;
            descriptor->usedAmount = *usedResources;
            descriptor->numRunningApp = 0;
            descriptor->address = inet::L3Address("127.0.0.1");
            descriptor->viPort = 2222;

            // TODO Quale chiave inserire? l'indirizzo IP dell'host?
            std::string key = "LOCAL_ID";
            handledHosts[key] = *descriptor;
        }
        else
            throw cRuntimeError("VirtualisationInfrastructureManagerDyn::initialize - \tFATAL! Cannot find static resource definition!");

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

    // Set up UDP socket for communcation
    localAddress = inet::L3AddressResolver().resolve(par("localAddress"));
    EV << "VirtualisationInfrastructureManagerDyn::initialize - localAddress " << localAddress << endl;
    int port = par("localPort");
    EV << "VirtualisationInfrastructureManagerDyn::initialize - binding socket to port " << port << endl;
    if (port != -1)
    {
        socket.setOutputGate(gate("socketOut"));
        socket.bind(port);
    }


    // Broker settings
    brokerIPAddress = inet::L3AddressResolver().resolve(par("brokerAddress").stringValue());

    // Adding new gates and connect
    // TODO can we use the same socketIn/out gate, instead of creating new ones?
    cModule *at = host->getSubmodule("at");
    cGate* newAtInGate = at->getOrCreateFirstUnconnectedGate("in", 0, false, true);
    cGate* newAtOutGate = at->getOrCreateFirstUnconnectedGate("out", 0, false, true);
    gate("socketBrokerOut")->connectTo(newAtInGate);
    newAtOutGate->connectTo(gate("socketBrokerIn"));

    // Print message
    cMessage *print = new cMessage("print");
    scheduleAt(simTime()+0.1, print);

    // set tcp socket VIM <--> Broker
//    SubscriberBase::handleStartOperation(operation);

}

void VirtualisationInfrastructureManagerDyn::handleMessageWhenUp(omnetpp::cMessage *msg)
{
    EV << "VirtualisationInfrastructureManagerDyn::handleMessage - message received! " << msg->getName() << endl;
    if (msg->isSelfMessage() && strcmp(msg->getName(), "print") == 0)
    {
        EV << "VirtualisationInfrastructureManagerDyn::handleMessage - self message received!" << endl;
        printResources();
        allocateResources(1000,1000,1000, "LOCAL_ID");
        printResources();
        deallocateResources(1000,1000,1000, "LOCAL_ID");
        printResources();
        std::string addedHostId = registerHost(2222,2222,2222,inet::L3Address("192.168.10.10"));
        EV << "VirtualisationInfrastructureManagerDyn::handleMessage - bestHost " << findBestHostDyn(1000,1000,1000) << endl;
        unregisterHost(addedHostId);
        delete msg;
    }
    else if(!msg->isSelfMessage() &&  socket.belongsToSocket(msg)){
        EV << "VirtualisationInfrastructureManagerDyn::handleMessage - other message received!" << endl;

           // TODO remove after the implementation of the publisher
        if (!strcmp(msg->getName(), "Register")){
            EV << "VirtualisationInfrastructureManagerDyn::handleMessage - TYPE: Register" << endl;

            inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
            if (pPacket == 0)
               throw cRuntimeError("VirtualisationInfrastructureManagerDyn::handleMessage - FATAL! Error when casting to inet packet");

            auto data = pPacket->peekData<RegistrationPacket>();
            //        inet::PacketPrinter printer;
            //        printer.printPacket(std::cout, pPacket);

            registerHost(data->getRam(),data->getDisk(),data->getCpu(),data->getAddress());

            getParentModule()->bubble("Host Registrato");
            delete msg;
        }else if (!strcmp(msg->getName(), "InstantiationResponse")){
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

//            printRequests();
//            printHandledApp();
            delete msg;
        }else if (!strcmp(msg->getName(), "TerminationResponse")){
            EV << "VirtualisationInfrastructureManagerDyn::handleMessage - TYPE: TerminationResponse" << endl;

            delete msg;
        }
    }
    else{
        EV << "VirtualisationInfrastructureManagerDyn::handleMessage - TCP message received!" << endl;
        SubscriberBase::handleMessageWhenUp(msg);
    }

//    delete msg;
}

std::string VirtualisationInfrastructureManagerDyn::registerHost(double ram, double disk, double cpu, inet::L3Address ip_addr)
{
    // TODO add viPort
    EV << "VirtualisationInfrastructureManagerDyn::registerHost" << endl;

    hostCounter += 1;
    std::stringstream ss;
    ss << hostCounter << "_" << ip_addr;
    std::string host_id = ss.str();

    auto it = handledHosts.find(host_id);
    if(it != handledHosts.end()){
        EV << "VirtualisationInfrastructureManagerDyn::registerHost - Host already exists!" << endl;
        return "";
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

    descriptor->totalAmount = *totalResources;
    descriptor->usedAmount = *usedResources;
    descriptor->numRunningApp = 0;
    descriptor->address = ip_addr;
    descriptor->viPort = 2222;

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

void VirtualisationInfrastructureManagerDyn::unregisterHost(std::string host_id)
{
    EV << "VirtualisationInfrastructureManagerDyn::unregisterHost" << endl;
    auto it = handledHosts.find(host_id);
    if(it == handledHosts.end()){
        EV << "VirtualisationInfrastructureManagerDyn::unregisterHost - Host doesn't exists!" << endl;
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
        available = ram < it->totalAmount.ram - it->usedAmount.ram
                    && disk < it->totalAmount.disk - it->usedAmount.disk
                    && cpu  < it->totalAmount.cpu - it->usedAmount.cpu;
    }

    return available;
}

void VirtualisationInfrastructureManagerDyn::allocateResources(double ram, double disk, double cpu, std::string hostId)
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

void VirtualisationInfrastructureManagerDyn::deallocateResources(double ram, double disk, double cpu, std::string hostId)
{
    auto it = handledHosts.find(hostId);
    if(it == handledHosts.end()){
        EV << "VirtualisationInfrastructureManagerDyn::deallocateResources - Host not found!" << endl;
    }
    EV << "VirtualisationInfrastructureManagerDyn::allocateResources - Deallocating resources on " <<  hostId << endl;

    HostDescriptor* host = &(it->second);

    host->usedAmount.ram -= ram;
    host->usedAmount.disk -= disk;
    host->usedAmount.cpu -= cpu;
}

std::string VirtualisationInfrastructureManagerDyn::findBestHostDyn(double ram, double disk, double cpu)
{
    EV << "VirtualisationInfrastructureManagerDyn::findBestHostDyn - Start" << endl;

    for(auto it = handledHosts.begin(); it != handledHosts.end(); ++it){
        std::string key = it->first;
        HostDescriptor descriptor = (it->second);

        if (!strcmp(it->first.c_str(), "LOCAL_ID")){
            continue;
        }

        bool available = ram < descriptor.totalAmount.ram - descriptor.usedAmount.ram
                    && disk < descriptor.totalAmount.disk - descriptor.usedAmount.disk
                    && cpu  < descriptor.totalAmount.cpu - descriptor.usedAmount.cpu;

        if(available){
            EV << "VirtualisationInfrastructureManagerDyn::findBestHostDyn - Found " << key << endl;
            return key;
        }
    }

    EV << "VirtualisationInfrastructureManagerDyn::findBestHostDyn - Best host not found!" << endl;
    return "";
}

MecAppInstanceInfo* VirtualisationInfrastructureManagerDyn::instantiateMEApp(CreateAppMessage* msg)
{
    EV << "VirtualisationInfrastructureManagerDyn:: instantiateMEApp" << endl;

    Enter_Method_Silent();

    std::string bestHostKey = findBestHostDyn(1,1,1);
    HostDescriptor bestHost = handledHosts[bestHostKey];

    inet::L3Address bestHostAddress = bestHost.address;
    int bestHostPort = bestHost.viPort;


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
    newAppEntry.usedResources.ram  = msg->getRequiredRam();
    newAppEntry.usedResources.disk = msg->getRequiredDisk();
    newAppEntry.usedResources.cpu  = msg->getRequiredCpu();
    waitingInstantiationRequests[std::to_string(msg->getUeAppID())] = newAppEntry;

    socket.sendTo(packet, bestHostAddress, bestHostPort);

    MecAppInstanceInfo* appInfo = new MecAppInstanceInfo();
    appInfo->status = true;
    appInfo->instanceId = msg->getMEModuleName(); //TODO Cambiare con un id dell'istanza creata (?)
    appInfo->endPoint.addr = bestHostAddress;
    appInfo->endPoint.port = bestHostPort;

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
    int port = 2222;

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
        std::string key = it->first;
        HostDescriptor descriptor = (it->second);

        EV << "VirtualisationInfrastructureManagerDyn::printResources - HOST: " << key << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printResources - IP Address: " << descriptor.address << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printResources - allocated Ram: " << descriptor.usedAmount.ram << " / " << descriptor.totalAmount.ram << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printResources - allocated Disk: " << descriptor.usedAmount.disk << " / " << descriptor.totalAmount.disk << endl;
        EV << "VirtualisationInfrastructureManagerDyn::printResources - allocated CPU: " << descriptor.usedAmount.cpu << " / " << descriptor.totalAmount.cpu << endl;
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


/*
 * Private Functions
 */

std::list<HostDescriptor> VirtualisationInfrastructureManagerDyn::getAllHosts()
{
    std::list<HostDescriptor> hosts;
    for(auto it = handledHosts.begin(); it != handledHosts.end(); ++it){
        hosts.push_front(it->second);
    }

    return hosts;
}

bool VirtualisationInfrastructureManagerDyn::isAllocableOnBuffer(double ram, double disk, double cpu)
{
    return  ram < totalBufferResources->ram - usedBufferResources->ram
            && disk < totalBufferResources->disk - usedBufferResources->disk
            && cpu  < totalBufferResources->cpu - usedBufferResources->cpu;
}


void VirtualisationInfrastructureManagerDyn::manageNotification(int type)
{
    if(type == RESPONSE)
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
            registerHost((*it)["ram"], (*it)["disk"], (*it)["cpu"], inet::L3Address(ipAddress_str.c_str()));

        }
    }
    else if(type == REQUEST)
    {
        HttpRequestMessage *request = dynamic_cast<HttpRequestMessage*> (currentHttpMessage);
        EV << "VIM::received a request - new resource available" << endl;
        //EV << "VIM::Request received: " << request->getBody() << endl;
        std::string uri = request->getUri();
        // TODO only post request
        // use webhook
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
            registerHost((*it)["ram"], (*it)["disk"], (*it)["cpu"], inet::L3Address(ipAddress_str.c_str()));
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


