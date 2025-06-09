#ifndef __SIMU5G_FEDERATOR_H
#define __SIMU5G_FEDERATOR_H

#include <omnetpp.h>

// inet
#include "inet/applications/base/ApplicationBase.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

// MECOrchestrator interface
#include "nodes/mec/Dynamic/MEO/IMecOrchestrator.h"

// Registration Packet
#include "nodes/mec/Dynamic/MEO/Messages/RegistrationPkt_m.h"

// Application Descriptor class
#include "nodes/mec/MECOrchestrator/ApplicationDescriptor/ApplicationDescriptor.h"

// UALCMP messages
#include "nodes/mec/UALCMP/UALCMPMessages/UALCMPMessages_m.h"
#include "nodes/mec/UALCMP/UALCMPMessages/UALCMPMessages_types.h"
#include "nodes/mec/UALCMP/UALCMPMessages/CreateContextAppMessage.h"
#include "nodes/mec/UALCMP/UALCMPMessages/CreateContextAppAckMessage.h"


// mm4 messages
//#include "apps/mec/MEOApp/Messages/MeoVimPackets_m.h"
//
//// mm3 messages
//#include "apps/mec/MEOApp/Messages/MeoMepmPackets_m.h"

//mm3 and mm4 messages
#include "nodes/mec/Dynamic/MEO/Messages/MeoPackets_m.h"

#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"


using namespace omnetpp;

struct MecHostInfo{
    std::string hostName;
    int hostId;
};


//Boh
struct ServiceInfo{
    std::string id;
};

struct FedServiceInfo{
    std::string systemId;
    MecHostInfo mecHostInformation;
    ServiceInfo serviceInfo;
};

struct SystemInfoEntry{
    std::string systemId;
    std::string systemName;
    std::string systemProvider;
    std::string uniqueId;
};






class FederatorApp : public cSimpleModule, public inet::UdpSocket::ICallback {

private:
    std::string appName;
    std::vector<SystemInfoEntry*> systemRegistrer;

    int localPort;
    inet::L3Address localIPAddress;
    inet::UdpSocket socket;

    int MEOPort;
    inet::L3Address MEOAddress;

protected:
    virtual void initialize() override;
    std::string generateUniqueId();

    virtual void socketDataArrived(inet::UdpSocket *socket, inet::Packet *packet) override;
    virtual void socketErrorArrived(inet::UdpSocket *socket, inet::Indication *indication) override {};
    virtual void socketClosed(inet::UdpSocket *socket) override {};

    void handleMessage(cMessage *msg) override;
    void handleRegistration(inet::Packet *packet);
    void handleCancelRegistration(inet::Packet *packet);
    void handleUpdateRegistration(inet::Packet *packet);

    void handleMECSystemReq(inet::Packet *packet);
    void handleForwardReqToMEF(inet::Packet *packet);
    void handleMECSystemInfoRes(inet::Packet *packet);
    void handleMEFSystemInfoRes(inet::Packet *packet);

    void handleAppRequestMEOtoMEF(inet::Packet *contAppMsg);
    void handleAppRequestMEFtoMEF(inet::Packet *contAppMsg);
    void handleAppResponseMEOtoMEF(inet::Packet *contAppMsg);
    void handleAppResponseMEFtoMEF(inet::Packet *contAppMsg);

    void handleAppMigrationReqeustMEOtoMEF(inet::Packet *contAppMsg);
    void handleAppMigrationReqeustMEFtoMEF(inet::Packet *contAppMsg);
};

#endif
