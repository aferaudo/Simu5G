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
//#include "nodes/mec/MECPlatform/MECServices/ApplicationMobilityService/resources/MobilityProcedureSubscription.h"

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

#include "nodes/mec/Dynamic/MEO/Messages/RegistrationPkt_m.h"
#include "nodes/mec/Dynamic/MEO/Messages/MeoPackets_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"

// subscriber class
#include "apps/mec/ResourceSharingApps/PubSub/Subscriber/SubscriberBase.h"

using namespace omnetpp;

class ServiceInfo;
class ServiceRegistry;

class MecPlatformManagerDyn : public SubscriberBase
{

    VirtualisationInfrastructureManagerDyn* vim;
    ServiceRegistry* serviceRegistry;

    bool amsEnabled;

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

        MecAppInstanceInfo* instantiateMEApp(InstantiationApplicationRequest*);
        bool terminateMEApp(DeleteAppMessage*);

        bool instantiateEmulatedMEApp(CreateAppMessage*);
        bool terminateEmulatedMEApp(DeleteAppMessage*);

        const std::vector<ServiceInfo>* getAvailableMecServices() const;
        const std::set<std::string>* getAvailableOmnetServices() const;

        void registerMecService(ServiceDescriptor&) const;

    protected:
        virtual void initialize(int stage) override;
        virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
        virtual void handleStartOperation(inet::LifecycleOperation *operation) override ;
        //virtual void handleMessage(cMessage *msg);

        virtual void manageNotification() override;

    private:
        void sendMEORegistration();
        void handleServiceRequest(inet::Packet* resourcePacket);
        void handleInstantiationRequest(inet::Packet* instantiationPacket);
        void handleInstantiationResponse(inet::Packet* instantiationPacket);
        void handleTerminationRequest(inet::Packet *packet);
        void handleTerminationResponse(inet::Packet * packet);
        bool checkServiceAvailability(const char* serviceName);

};

#endif
