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

using namespace omnetpp;


class ExtMECWarningAlertApp : public ExtMecAppBase
{
    //UDP socket to communicate with the UeApp
    inet::UdpSocket ueSocket;
    int localUePort;

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


  protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void finish() override;

    virtual void handleProcessedMessage(omnetpp::cMessage *msg) override;

    virtual void handleHttpMessage(int connId) override;
    virtual void handleServiceMessage(int connId, int index) override;
    virtual void handleMp1Message(int connId) override;
    virtual void handleUeMessage(omnetpp::cMessage *msg) override;

    virtual void modifySubscription();
    virtual void sendSubscription();
    virtual void sendDeleteSubscription();

    virtual void handleSelfMessage(cMessage *msg) override;

    virtual void handleLSMessage(inet::TcpSocket *serviceSocket, int index);
    virtual void handleAMSMessage(inet::TcpSocket *serviceSocket, int index);


//        /* TCPSocket::CallbackInterface callback methods */
    virtual void established(int connId) override;

  public:
    ExtMECWarningAlertApp();
    ~ExtMECWarningAlertApp();
};

#endif
