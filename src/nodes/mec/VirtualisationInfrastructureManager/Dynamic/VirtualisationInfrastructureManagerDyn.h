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
#include "apps/mec/ResourceSharingApps/PubSub/Subscriber/SubscriberBase.h"

//INET
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/common/packet/printer/PacketPrinter.h"
#include "inet/networklayer/common/InterfaceTable.h"

#include "nodes/mec/utils/MecCommon.h"

#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"
#include "apps/mec/ViApp/msg/InstantiationResponse_m.h"
#include "apps/mec/ViApp/msg/TerminationResponse_m.h"

#include "apps/mec/MEOApp/Messages/RegistrationPkt_m.h"
#include "apps/mec/MEOApp/Messages/MeoPackets_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"

//###########################################################################
//data structures and values

#define NO_SERVICE -1
#define SERVICE_NOT_AVAILABLE -2

using namespace omnetpp;

struct HostDescriptor // CarDescriptor
{
    ResourceDescriptor totalAmount;
    ResourceDescriptor usedAmount;
    ResourceDescriptor reservedAmount;
    int numRunningApp;
    inet::L3Address address;
    int viPort;
};

struct MecAppEntryDyn
{
    int ueAppID;
    ResourceDescriptor usedResources;
    SockAddr endpoint;
    std::string moduleName;
    std::string moduleType;
    bool isBuffered;
    int contextID;
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

class VirtualisationInfrastructureManagerDyn: public SubscriberBase
{
    // Reference to other modules
    cModule* vimHost;

    inet::UdpSocket socket;
    inet::L3Address destAddress;
    inet::L3Address localAddress;
    int port;

    std::map<int, HostDescriptor> handledHosts; // Resource Handling
    std::map<std::string, MecAppEntryDyn> handledApp; // App Handling
    std::map<std::string, MecAppEntryDyn> waitingInstantiationRequests; // vim waits for response from cars on which have requested instantiation

    int hostCounter = 0;    // counter to generate host ids
    int requestCounter = 0;

    // buffer
    double bufferSize;
    ResourceDescriptor* totalBufferResources;
    ResourceDescriptor* usedBufferResources;

    // meo parameters
    inet::L3Address meoAddress;
    int meoPort;

    // mepm parameters
    inet::L3Address mepmAddress;
    int mepmPort;

    // mep parameters
    inet::L3Address mp1Address;
    int mp1Port;


    inet::IInterfaceTable* ifacetable;


    std::string color;


    public:
        VirtualisationInfrastructureManagerDyn();

        /*
         * Istantiate ME Application on an handled car
         */
        MecAppInstanceInfo* instantiateMEApp(const CreateAppMessage*);

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
        int registerHost(int host_id, double ram, double disk, double cpu, inet::L3Address ip_addr, int viPort);

        /*
         * Remove managed host
         */
        void unregisterHost(int host_id);

        /*
         * Check if a MecApp is allocable according to available resources among the cars
         */
        bool isAllocable(double ram, double disk, double cpu);

        /*
        * Get all managed hosts
        */
       std::list<HostDescriptor> getAllHosts();

    protected:

        virtual void initialize(int stage) override;
        //virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

        //virtual void handleMessage(cMessage *msg);
        virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
        virtual void handleStartOperation(inet::LifecycleOperation *operation) override;


        /*
         * Find a MecService on the Host
         */
//        int findService(const char* serviceName);

        /*
         * Get available resources divided per car
         */
//        ResourceDescriptor getAvailableResources() const ;

        void printResources();

        void printHandledApp();

        void printRequests();

        /*
         * Allocate resources on specific host (car)
         */
        void allocateResources(double ram, double disk, double cpu, int hostId);

        /*
         * Deallocate resources on specific host (car)
         */
        void deallocateResources(double ram, double disk, double cpu, int hostId);

        /*
         * Reserve resources on specific host (car)
         */
        void reserveResources(double ram, double disk, double cpu, int hostId);

        /*
         * Release reserved resources on specific host (car)
         */
        void releaseResources(double ram, double disk, double cpu, int hostId);

        /*
         * Find the best managed host on which allocate App according to several metrics
         */
        int findBestHostDyn(double ram, double disk, double cpu);

        /*
         * Manage HTTP Notification
         * - Response: List of available resources in that zone
         * - Request: Notification of new available resources
         */
        virtual void manageNotification() override;

    private:

        void initResource();

        bool isAllocableOnBuffer(double ram, double disk, double cpu);

        HostDescriptor* findHostByAddress(inet::L3Address address);

        int findHostIDByAddress(inet::L3Address address);

        int findBestHostDynBestFirst(double ram, double disk, double cpu);

        int findBestHostDynRoundRobin(double ram, double disk, double cpu);

        void sendMEORegistration();
        void handleResourceRequest(inet::Packet* resourcePacket);
        void handleMepmMessage(cMessage* instantiationPacket);
};

#endif
