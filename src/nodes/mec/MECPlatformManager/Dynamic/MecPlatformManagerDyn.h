//
//  MecPlatformManagerDyn.h
//
//  @author Angelo Feraudo
//  @author Alessandro Calvio
//

#ifndef DYNAMIC_MECPM_H_
#define DYNAMIC_MECPM_H_

#include "nodes/mec/VirtualisationInfrastructureManager/Dynamic/VirtualisationInfrastructureManagerDyn.h"
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

using namespace omnetpp;

class ServiceInfo;
class ServiceRegistry;

class MecPlatformManagerDyn : public cSimpleModule
{

    VirtualisationInfrastructureManagerDyn* vim;
    ServiceRegistry* serviceRegistry;

    inet::UdpSocket orchestratorSocket;

    inet::L3Address orchestratorAddress;
    inet::L3Address localAddress;

    int orchestratorPort;
    int localPort;

    public:
        MecPlatformManagerDyn();

        MecAppInstanceInfo* instantiateMEApp(CreateAppMessage*);
        bool terminateMEApp(DeleteAppMessage*);

        bool instantiateEmulatedMEApp(CreateAppMessage*);
        bool terminateEmulatedMEApp(DeleteAppMessage*);

        const std::vector<ServiceInfo>* getAvailableMecServices() const;
        const std::set<std::string>* getAvailableOmnetServices() const;

        void registerMecService(ServiceDescriptor&) const;

    protected:
        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        virtual void finish();

};

#endif
