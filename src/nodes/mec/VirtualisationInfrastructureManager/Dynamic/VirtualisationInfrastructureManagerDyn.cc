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

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER-1){
        return;
    }

    EV << "VirtualisationInfrastructureManagerDyn::initialize - stage " << stage << endl;

    // Init parameters
    vimHost = getParentModule();
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

        // TODO Quale chiave inserire? l'indirizzo IP dell'host?
        std::string key = "LOCAL_ID";
        handledHosts[key] = *descriptor;
    }
    else
        throw cRuntimeError("VirtualisationInfrastructureManagerDyn::initialize - \tFATAL! Cannot find static resource definition!");

    // Set up socket for communcation
    localAddress = inet::L3AddressResolver().resolve(par("localAddress"));
    EV << "VirtualisationInfrastructureManagerDyn::initialize - localAddress " << localAddress << endl;
    int port = par("localPort");
    EV << "VirtualisationInfrastructureManagerDyn::initialize - binding socket to port " << port << endl;
    if (port != -1)
    {
        socket.setOutputGate(gate("socketOut"));
        socket.bind(port);
    }


    cMessage* print = new cMessage("Print");
//    scheduleAt(simTime()+0.01, print);
    scheduleAt(simTime()+1.00, print);

}

void VirtualisationInfrastructureManagerDyn::handleMessage(cMessage *msg)
{
    EV << "VirtualisationInfrastructureManagerDyn::handleMessage - message received!" << endl;
    if (msg->isSelfMessage())
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

        destAddress = inet::L3AddressResolver().resolve(par("destAddress"));
        inet::Packet* packet = new inet::Packet("Register");
        auto registrationpck = inet::makeShared<RegistrationPacket>();
        registrationpck->setRam(7777);
        registrationpck->setDisk(7777);
        registrationpck->setCpu(7777);
        registrationpck->setAddress(localAddress);
        registrationpck->setChunkLength(inet::B(200));
        packet->insertAtBack(registrationpck);
        socket.sendTo(packet, destAddress, 3333);
    }else{
        EV << "VirtualisationInfrastructureManagerDyn::handleMessage - other message received!" << endl;

        inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
        if (pPacket == 0)
            throw cRuntimeError("VirtualisationInfrastructureManagerDyn::handleMessage - FATAL! Error when casting to inet packet");

        auto data = pPacket->peekData<RegistrationPacket>();
//        inet::PacketPrinter printer;
//        printer.printPacket(std::cout, pPacket);

        registerHost(data->getRam(),data->getDisk(),data->getCpu(),data->getAddress());

        getParentModule()->bubble("Host Registrato");
    }
}

std::string VirtualisationInfrastructureManagerDyn::registerHost(double ram, double disk, double cpu, inet::L3Address ip_addr)
{
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

    handledHosts[host_id] = *descriptor;

    printResources();

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

void VirtualisationInfrastructureManagerDyn::printResources()
{
    EV << "VirtualisationInfrastructureManagerDyn:: printing..." << endl;
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


