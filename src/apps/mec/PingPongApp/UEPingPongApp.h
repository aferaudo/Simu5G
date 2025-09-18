#ifndef __UEPINGAPP_H_
#define __UEPINGAPP_H_

#include "inet/common/INETDefs.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_m.h"


using namespace inet;

class UEPingPongApp : public cSimpleModule
{

    L3Address destAddress;
    int destPort;
    int localPort;
    simtime_t startTime;

    std::string mecAppName;

    UdpSocket socket;

    int lastMasterId = -1;



  protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void sendStart();
    virtual void finish() override;

    void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details);
    int getCurrentCellId();

  public:
    UEPingPongApp();
    virtual ~UEPingPongApp();

};

#endif

