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

#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/MecCommon.h"
#include "apps/mec/MecApps/MecAppBase.h" // we need this to import their structure HttpMessageStatus, thus avoiding to define a new one here.



/*
 * This class enables the apps/mec/MecApps/MecAppBase.* models provided in Simu5G
 * to support our system.
 * Thus, end-user can easily deploy app compliant with the Standard ETSI MEC
 * and deploy them in our scenario.
 */

struct MecServiceInfo
{
    SockAddr host;
    std::string name;
//    HttpBaseMessage *currentHttpMessage;
    int sockid;
};


class ExtMecAppBase : public omnetpp::cSimpleModule, public inet::TcpSocket::ICallback
{
  protected:

    // Scheduling message processing
    omnetpp::cMessage* processMessage_;

    // Scheduling MEC App termination
    omnetpp::cMessage* terminationMessage_;

    // Scheduling MEC App termination
    omnetpp::cMessage* connectService_;

    // Message Queue received at MECApp
    omnetpp::cQueue packetQueue_;

    /*
     * To manage MEC App sockets
     */
    inet::SocketMap sockets_;

    // Although a MecApp may have or not a connection with
    // the MEC platform, it is a std component commont to multiple
    // apps.
    inet::TcpSocket* mp1Socket_;

    /*
     * It contains the sockets id corresponding to a service
     * This enables a MEC app to communicate with multiple MEC Service
     */
//    std::map<int, std::string> serviceNamesSocketMap_;

    std::vector<MecServiceInfo*> servicesData_;

    HttpBaseMessage* mp1HttpMessage;
//    std::map<std::string, HttpBaseMessage*> serviceHttpMessages;

    // endpoint for contacting the Service Registry
    inet::L3Address mp1Address;
    int mp1Port;

    // Local information
    int mecAppId;
    int mecAppIndex_;
    double requiredRam;
    double requiredDisk;
    double requiredCpu;

    inet::L3Address localAddress;

    // used to avoid nullpointer exception when module is suddenly deleted
    // This may occur for MEC app deployed on far-edge nodes, where nodes
    // can leave the resource pool and thus triggering app termination
    int responseCounter_;


    // simple module methods
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(omnetpp::cMessage *msg) override;
    virtual void finish() override;

    virtual void handleProcessedMessage(omnetpp::cMessage *msg);
    virtual inet::TcpSocket* addNewSocket();
    virtual inet::TcpSocket* addNewSocketNoHttp();
    virtual void removeSocket(inet::TcpSocket* tcpSock);

    // to be implemented by subclasses
    virtual void handleSelfMessage(omnetpp::cMessage *msg) = 0;
    virtual void handleHttpMessage(int connId) = 0;
    virtual void handleReceivedMessage(int sockId, inet::Packet *msg) = 0; // this method menage message received on a socket in listening mode
    virtual void handleServiceMessage(int index) = 0;
    virtual void handleMp1Message(int connId) = 0;
    virtual void handleUeMessage(omnetpp::cMessage *msg) = 0;
    virtual void established(int connId) = 0;
    virtual void handleTermination() = 0; //delete registrations


    virtual void connect(inet::TcpSocket* socket, const inet::L3Address& address, const int port);

    // Usually standard for each MECApp
    virtual bool getInfoFromServiceRegistry();

    // INET socket management methods
    virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) override;
    virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override;
    virtual void socketEstablished(inet::TcpSocket *socket) override;
    virtual void socketPeerClosed(inet::TcpSocket *socket) override;
    virtual void socketClosed(inet::TcpSocket *socket) override;
    virtual void socketFailure(inet::TcpSocket *socket, int code) override;
    virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
    virtual void socketDeleted(inet::TcpSocket *socket) override {}

    // utility methods
    virtual int findServiceFromSocket(int connId);
//    inet::TcpSocket * findSocketFromService(MecServiceInfo *service);

  public:
    ExtMecAppBase();
    virtual ~ExtMecAppBase();
};

#endif /* APPS_MEC_DYNAMICMECAPPS_MECAPPBASE_EXTMECAPP_H_ */
