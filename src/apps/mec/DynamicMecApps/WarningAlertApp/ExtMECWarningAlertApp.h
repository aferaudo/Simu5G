// Authors:
// Angelo Feraudo, Alessandro Calvio
// University of Bologna

#ifndef __SIMU5G_EXTMECWARNINGALERTAPP_H_
#define __SIMU5G_EXTMECWARNINGALERTAPP_H_

#include <omnetpp.h>

// inet
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/mec/DynamicMecApps/MecAppBase/ExtMecAppBase.h"
#include "apps/mec/WarningAlert/packets/WarningAlertPacket_m.h"

#include "nodes/mec/MECPlatform/MECServices/ApplicationMobilityService/resources/DeviceInformation.h"



using namespace omnetpp;


class ExtMECWarningAlertApp : public ExtMecAppBase
{

    enum SERVICE {
               LS = 0, // Location Service
               AMS, // Application Mobility Service
    };


    //UDP socket to communicate with the UeApp
    inet::UdpSocket ueSocket;
    int localUePort;

    // local binding for migration
    int localPort;

    inet::L3Address ueAppAddress;
    int ueAppPort;

    int size_;
    std::string subId;

    std::vector<std::string> serNames;
    // circle danger zone
    cOvalFigure * circle;
    double centerPositionX;
    double centerPositionY;
    double radius;

    // keep trace of ue status ("Entering" or "Leaving")
    std::string status;

    // ##### AMS parameters #####
    bool isMigrating; // TODO change name
    std::string webHook;
    // Address new location
    inet::L3Address migrationAddress;
    int migrationPort;
    inet::TcpSocket* amsStateSocket_; // this socket depends on the app - old (in migration) vs new (migrated)
    std::string amsRegistrationId;
    std::string amsSubscriptionId;
    bool subscribed;
    bool ueRegistered;
    // ##### -------------- #####


  protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void finish() override;

    virtual void handleProcessedMessage(omnetpp::cMessage *msg) override;

    virtual void handleHttpMessage(int connId) override;
    virtual void handleServiceMessage(int index) override;
    virtual void handleMp1Message(int connId) override;
    virtual void handleUeMessage(omnetpp::cMessage *msg) override;
    virtual void handleReceivedMessage(cMessage *msg) override;
    virtual void handleTermination() override;

    // Location Service API
    virtual void modifySubscriptionLS(inet::TcpSocket *serviceSocket, std::string criteria);
    virtual void sendSubscriptionLS(inet::TcpSocket *socket, std::string criteria);
    virtual void sendDeleteSubscriptionLS(inet::TcpSocket *serviceSocket);


    // Application Mobility Service API
    virtual void sendRegistrationAMS(inet::TcpSocket *socket, ContextTransferState transferState=NOT_TRANSFERRED);
    virtual void sendDeleteRegistrationAMS(inet::TcpSocket *socket);
    virtual void sendSubscriptionAMS(inet::TcpSocket *socket);
    virtual void sendDeleteSubscriptionAMS(inet::TcpSocket *socket);
    virtual void updateRegistrationAMS(inet::TcpSocket *socket,
            AppMobilityServiceLevel level=APP_MOBILITY_NOT_ALLOWED, ContextTransferState transferState=NOT_TRANSFERRED);
    virtual void updateSubscriptionAMS(inet::TcpSocket *socket);

    virtual void handleSelfMessage(cMessage *msg) override;

    virtual void handleLSMessage(inet::TcpSocket *serviceSocket);
    virtual void handleAMSMessage(inet::TcpSocket *serviceSocket);


    /* TCPSocket::CallbackInterface callback methods */
    virtual void established(int connId) override;

    // utility methods
    virtual void sendNackToUE();
    virtual void sendState();

  public:
    ExtMECWarningAlertApp();
    ~ExtMECWarningAlertApp();

  private:
    nlohmann::ordered_json getSubsciptionAMSBody();
};

#endif
