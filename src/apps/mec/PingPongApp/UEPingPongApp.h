#ifndef __UEPINGAPP_H_
#define __UEPINGAPP_H_

#include "inet/common/INETDefs.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_m.h"

using namespace inet;

class UEPingPongApp : public cSimpleModule
{
  protected:
    L3Address destAddress;
    int destPort;
    int localPort;
    simtime_t startTime;

    std::string mecAppName;

    UdpSocket socket;

  protected:
    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    void initialize(int stage);
    virtual void handleMessage(cMessage *msg) override;
    virtual void sendStart();

};

#endif

