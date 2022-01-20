//
//  VirtualisationInfrastructureManagerDyn.h
//
//  @author Angelo Feraudo
//  @author Alessandro Calvio
//


#ifndef DYNAMIC_VIM_H_
#define DYNAMIC_VIM_H_

#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h" //MecAppInstanceInfo struct

//BINDER and UTILITIES
#include "common/LteCommon.h"
#include "common/binder/Binder.h"
#include "inet/networklayer/common/InterfaceTable.h"

//INET
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/common/packet/printer/PacketPrinter.h"

#include "nodes/mec/utils/MecCommon.h"

#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"

//###########################################################################
//data structures and values

#define NO_SERVICE -1
#define SERVICE_NOT_AVAILABLE -2

using namespace omnetpp;

struct HostDescriptor // CarDescriptor
{
    ResourceDescriptor totalAmount;
    ResourceDescriptor usedAmount;
    int numRunningApp;
    inet::L3Address address;
};

struct AppDescriptor
{
    ResourceDescriptor usedResources;
    inet::L3Address address;
    int port;
    std::string name;
    std::string hostingHost;
};

// used to calculate processing time needed to execute a number of instructions
//enum SchedulingMode {SEGREGATION, FAIR_SHARING};

//
//  VirtualisationInfrastructureManagerDyn
//
//  Simple Module for handling the availability of virtualised resources on the Dynamic MEC host.
//  It keep tracks of resources on each VI and handles the allocation/deallocation policies.
//

class CreateAppMessage;
class DeleteAppMessage;

class VirtualisationInfrastructureManagerDyn : public cSimpleModule
{
    // Reference to other modules
    cModule* vimHost;

    inet::UdpSocket socket;
    inet::L3Address destAddress;
    inet::L3Address localAddress;

    std::map<std::string, HostDescriptor> handledHosts; // Resource Handling
    std::map<std::string, AppDescriptor> handledApp; //App Handling

    // counter to generate host ids
    int hostCounter = 0;

    // buffer
    double bufferSize;
    ResourceDescriptor* totalBufferResources;
    ResourceDescriptor* usedBufferResources;


    std::string color;


    public:
        VirtualisationInfrastructureManagerDyn();

        /*
         * Istantiate ME Application on an handled car
         */
        MecAppInstanceInfo* instantiateMEApp(CreateAppMessage*);

        /*
         * Istantiate Emulated ME Application from MECPM
         */
        bool instantiateEmulatedMEApp(CreateAppMessage*);

        /*
         * Terminate ME Application from MECPM
         */
        bool terminateMEApp(DeleteAppMessage*);

        /*
         * Terminate Emulated ME Application from MECPM
         */
        bool terminateEmulatedMEApp(DeleteAppMessage*);

        /*
         * Logic for computing simulated processing time
         */
//        virtual double calculateProcessingTime(int  mecAppID, int numOfInstructions);

        /*
         * Register new MecApp
         */
//        bool registerMecApp(int mecAppID, int reqRam, int reqDisk, double reqCpu);

        /*
         * Unregister MecAPP
         */
//        bool deRegisterMecApp(int mecAppID);

        /*
         * Add a new Host to those managed by VIM
         */
        std::string registerHost(double ram, double disk, double cpu, inet::L3Address ip_addr);

        /*
         * Remove managed host
         */
        void unregisterHost(std::string host_id);

        /*
         * Check if a MecApp is allocable according to available resources among the cars
         */
        bool isAllocable(double ram, double disk, double cpu);

        /*
        * Get all managed hosts
        */
       std::list<HostDescriptor> getAllHosts();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }

        void initialize(int stage);

        virtual void handleMessage(cMessage *msg);

        /*
         * Find a MecService on the Host
         */
//        int findService(const char* serviceName);

        /*
         * Get available resources divided per car
         */
//        ResourceDescriptor getAvailableResources() const ;

        void printResources();

        /*
         * Allocate resources on MecHost and car(?)
         */
        void allocateResources(double ram, double disk, double cpu, std::string hostId);

        /*
         * Deallocate resources on MecHost and car(?)
         */
        void deallocateResources(double ram, double disk, double cpu, std::string hostId);

        /*
         * Find the best managed host on which allocate App according to several metrics
         */
        std::string findBestHostDyn(double ram, double disk, double cpu);

    private:

        bool isAllocableOnBuffer(double ram, double disk, double cpu);

};

#endif
