//
//  VirtualisationInfrastructureManagerDyn.cc
//
//  @author Angelo Feraudo
//  @author Alessandro Calvio
//

// Imports
#include "nodes/mec/VirtualisationInfrastructureManager/Dynamic/VirtualisationInfrastructureManagerDyn.h"


Define_Module(VirtualisationInfrastructureManagerDyn);

VirtualisationInfrastructureManagerDyn::VirtualisationInfrastructureManagerDyn()
{

}

void VirtualisationInfrastructureManagerDyn::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // avoid multiple initializations
//    if (stage!=inet::INITSTAGE_APPLICATION_LAYER-1){
//        std::cout << "sto returnando" << endl;
//        return;
//    }

    EV << "VirtualisationInfrastructureManagerDyn::initialize - stage " << stage << endl;

    // get reference of other modules
    vimHost = getParentModule();
    if(vimHost->hasPar("localRam") && vimHost->hasPar("localDisk") && vimHost->hasPar("localCpuSpeed")){
        HostDescriptor* descriptor = new HostDescriptor();
        ResourceDescriptor* totalResources = new ResourceDescriptor();
        totalResources->ram = vimHost->par("localRam").doubleValue();
        totalResources->cpu = vimHost->par("localCpuSpeed").doubleValue();
        totalResources->disk = vimHost->par("localDisk").doubleValue();

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

    cMessage* print = new cMessage("Print");
    scheduleAt(simTime()+0.01, print);

}

void VirtualisationInfrastructureManagerDyn::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        EV << "VirtualisationInfrastructureManagerDyn::handleMessage - self message received!" << endl;
        printResources();
        allocateResources(1000,1000,1000, "LOCAL_ID");
        printResources();
        deallocateResources(1000,1000,1000, "LOCAL_ID");
        printResources();
        EV << "VirtualisationInfrastructureManagerDyn::handleMessage - bestHost " << findBestHostDyn(1000,1000,1000) << endl;
    }else{
        EV << "VirtualisationInfrastructureManagerDyn::handleMessage - other message received!" << endl;
    }
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
