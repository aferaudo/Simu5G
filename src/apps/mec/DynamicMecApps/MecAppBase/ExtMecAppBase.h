// Authors:
// Angelo Feraudo, Alessandro Calvio
// University of Bologna



#ifndef APPS_MEC_DYNAMICMECAPPS_MECAPPBASE_EXTMECAPP_H_
#define APPS_MEC_DYNAMICMECAPPS_MECAPPBASE_EXTMECAPP_H_

#include <omnetpp.h>
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/common/socket/SocketMap.h"


/*
 * This class adapts the apps/mec/MecApps/MecAppBase.* models provided in Simu5G
 * to support our system.
 * Thus, end-user can easily deploy app compliant with the Standard ETSI MEC
 * and deploy them in our scenario.
 */

class ExtMecAppBase : public omnetpp::cSimpleModule, public inet::TcpSocket::ICallback
{
  protected:

    // Scheduling message processing
    omnetpp::cMessage* processMessage_;

    // Message Queue received at MECApp
    omnetpp::cQueue packetQueue_;

    /*
     * To manage MEC App sockets
     */
    inet::SocketMap sockets_;

    /*
     * It contains the sockets id corresponding to a service
     * This enables a MEC app to communicate with multiple MEC Service
     */
    std::map<int, std::string> serviceNames_;

    /*
     * Service Registry information uris
     */
    std::vector<std::string> serviceRegistryGetUris;


    // endpoint for contacting the Service Registry
    inet::L3Address mp1Address;
    int mp1Port;

    int mecAppId;
    int mecAppIndex_;
    double requiredRam;
    double requiredDisk;
    double requiredCpu;

    // simple module methods
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(omnetpp::cMessage *msg) override;
    virtual void finish() override;

    virtual void handleProcessedMessage(omnetpp::cMessage *msg);
    virtual inet::TcpSocket* addNewSocket();
    virtual void removeSocket(inet::TcpSocket* tcpSock);

    // to be implemented by subclasses
    virtual void handleSelfMessage(omnetpp::cMessage *msg) = 0;
    virtual void established(int connId) = 0;
    virtual void handleHttpMessage(int connId) = 0;

    virtual void connect(inet::TcpSocket* socket, const inet::L3Address& address, const int port);


    // INET socket management methods
    virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) override;
    virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override { socket->accept(availableInfo->getNewSocketId()); }
    virtual void socketEstablished(inet::TcpSocket *socket) override;
    virtual void socketPeerClosed(inet::TcpSocket *socket) override;
    virtual void socketClosed(inet::TcpSocket *socket) override;
    virtual void socketFailure(inet::TcpSocket *socket, int code) override;
    virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
    virtual void socketDeleted(inet::TcpSocket *socket) override {}
  public:
    ExtMecAppBase();
    virtual ~ExtMecAppBase();
};

#endif /* APPS_MEC_DYNAMICMECAPPS_MECAPPBASE_EXTMECAPP_H_ */
