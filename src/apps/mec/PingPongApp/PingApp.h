#ifndef __PINGPONGAPP_H_
#define __PINGPONGAPP_H_

#include "apps/mec/DynamicMecApps/MecAppBase/DMecAppBaseDyn.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"

using namespace omnetpp;
using namespace inet;

class PingApp : public DMecAppBaseDyn
{
  protected:
    UdpSocket socket;
    L3Address destAddr;
    int localPort;
    int destPort;

    int pingSending;

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void finish() override;

    virtual void handleSelfMessage(cMessage* msg) override;
    virtual void sendPing();
    virtual void requestApp();

    // metodi richiesti ma non usati
    virtual void handleServiceMessage() override {EV << "handleServiceMessage" << endl;}
    virtual void handleMp1Message() override {EV << "handleMp1Message" << endl;}
    virtual void handleAmsMessage() override {EV << "handleAmsMessage" << endl;}
    virtual void handleStateMessage() override {EV << "handleStateMessage" << endl;}
    virtual void handleUeMessage(cMessage* msg) override {EV << "handleUeMessage" << endl;}
    virtual void established(int connId) override {EV << "established" << endl;}


    virtual void handleGenericMessage(cMessage* msg) override;

  public:
    PingApp();
    virtual ~PingApp();
};

#endif
