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
#include "inet/networklayer/common/InterfaceTable.h"

#include "nodes/mec/utils/MecCommon.h"

#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "apps/mec/MEOApp/Messages/RegistrationPkt_m.h"
#include "apps/mec/MEOApp/Messages/MeoPackets_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"

using namespace omnetpp;

class ServiceInfo;
class ServiceRegistry;

class MecPlatformManagerDyn : public cSimpleModule
{

    VirtualisationInfrastructureManagerDyn* vim;
    ServiceRegistry* serviceRegistry;

    inet::UdpSocket socket;

    // local parameters
    inet::L3Address localAddress;
    int localPort;

    // meo parameters
    inet::L3Address meoAddress;
    int meoPort;

    // vim parameters
    inet::L3Address vimAddress;
    int vimPort;

    inet::IInterfaceTable* ifacetable;

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

    private:
        void sendMEORegistration();
        void handleServiceRequest(inet::Packet* resourcePacket);
        void handleInstantiationRequest(inet::Packet* instantiationPacket);

};

#endif
