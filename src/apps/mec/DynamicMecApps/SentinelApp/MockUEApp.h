//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __SIMU5G_MOCKUEAPP_H_
#define __SIMU5G_MOCKUEAPP_H_

#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include <omnetpp.h>

using namespace omnetpp;

/**
 * TODO - Generated class
 */
class MockUEApp : public cSimpleModule
{
  //communication to device app and mec app
  inet::UdpSocket socket;
  
  int size_;
  simtime_t period_;
  int localPort_;
  int deviceAppPort_;
  inet::L3Address deviceAppAddress_;

  char* sourceSimbolicAddress;            //Ue[x]
  char* deviceSimbolicAppAddress_;              //meHost.virtualisationInfrastructure

  // MEC application endPoint (returned by the device app)
  // FIXME Does Mock application need to communicate with MEC app?
  inet::L3Address mecAppAddress_;
  int mecAppPort_;

  std::string mecAppName;

  // mobility informations
  cModule* ue;

  //scheduling
  cMessage *selfStart_;
  cMessage *selfStop_;
  cMessage *selfMecAppStart_;

  protected:
    virtual int numInitStages() const override{ return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

    void sendStartMECApp();
    void sendMessageToMECApp();
    void sendStopMECApp();


    void handleAckStartMECApp(cMessage* msg);
    void handleInfoMECApp(cMessage* msg);
    void handleAckStopMECApp(cMessage* msg);
    void handleMECAppInfo(cMessage *msg);
  
  public:
    MockUEApp();
    ~MockUEApp();
};

#endif
