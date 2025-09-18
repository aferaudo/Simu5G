#ifndef __SIMU5G_FEDERATOR_H
#define __SIMU5G_FEDERATOR_H

#include <omnetpp.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

// inet
#include "inet/applications/base/ApplicationBase.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"


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


struct FederatorEntry{
    std::string address;
    int port;
};






class FederatorBrokerApp : public cSimpleModule, public inet::UdpSocket::ICallback{

private:
    std::string appName;
    std::vector<FederatorEntry*> federatorRegistry;


    int localPort;
    inet::L3Address localIPAddress;
    inet::UdpSocket socket;


public:
    FederatorBrokerApp ();
    ~FederatorBrokerApp();

protected:
    virtual void initialize() override;
    std::string generateUniqueId();

    virtual void socketDataArrived(inet::UdpSocket *socket, inet::Packet *packet) override;
    virtual void socketErrorArrived(inet::UdpSocket *socket, inet::Indication *indication) override {};
    virtual void socketClosed(inet::UdpSocket *socket) override {};

    void handleMessage(cMessage *msg) override;
    void handleRegistration(inet::Packet *packet);
    void handleAppMigrationReqeustMEFtoMEFB(inet::Packet *contAppMsg);
    void handleAppRequestMEFtoMEFB(inet::Packet *contAppMsg);
    void handleAppResponseMEFtoMEFB(inet::Packet *contAppMsg);
    void handleAckMEFtoMEFB(inet::Packet *contAppMsg);

};

#endif
