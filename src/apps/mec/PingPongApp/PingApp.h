#ifndef __PINGPONGAPP_H_
#define __PINGPONGAPP_H_

#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"

#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"
#include "nodes/mec/MECPlatform/MECServices/ApplicationMobilityService/resources/TargetAppInfo.h"

//#include "apps/mec/DynamicMecApps/MecAppBase/DMecAppBase.h"
#include "apps/mec/DynamicMecApps/MecAppBase/DMecAppBaseDyn.h"


#include "nodes/mec/MECPlatform/MECServices/ApplicationMobilityService/resources/MobilityProcedureNotification.h"



using namespace omnetpp;
using namespace inet;

class PingApp : public DMecAppBaseDyn
{
  protected:
    UdpSocket socket;
    inet::UdpSocket stateSocket;
    L3Address destAddr;
    int localPort;
    int destPort;

    int pingSending;

    std::string webHook;
    bool isMigrated;
    bool amsOld;
    inet::L3Address migrationAddress;
    int migrationPort;

    cMessage* requestAppMsg;

    int pendingOps;

    int id;

    simtime_t startTime;
    simtime_t startTimeOp;
    simtime_t startTimeOpTWO;
    simsignal_t requestAppInt = registerSignal("requestAppInt");
    simsignal_t requestAppFed = registerSignal("requestAppFed");

    simtime_t startTimePing;
    simsignal_t pingPong = registerSignal("pingPong");

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void finish() override;

    virtual void handleMessage(cMessage* msg) override;
    virtual void handleSelfMessage(cMessage* msg) override;
    virtual void sendPing();
    virtual void requestApp();
    virtual void subscribeAms();
    virtual void unsubscribeAms();

    virtual void getServiceData(const char* uri);

    // metodi richiesti ma non usati
    virtual void handleServiceMessage() override {EV << "handleServiceMessage" << endl;}
    virtual void handleMp1Message() override;
    virtual void handleAmsMessage() override;
    virtual void handleStateMessage() override;
    virtual void handleUeMessage(cMessage* msg) override {EV << "handleUeMessage" << endl;}
    virtual void established(int connId) override;


    virtual void handleGenericMessage(cMessage* msg) override;

  public:
    PingApp();
    virtual ~PingApp();
};

#endif
