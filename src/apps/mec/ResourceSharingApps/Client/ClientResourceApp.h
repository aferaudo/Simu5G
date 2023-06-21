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

#ifndef __SIMU5G_CLIENTRESOURCEAPP_H_
#define __SIMU5G_CLIENTRESOURCEAPP_H_

#include <omnetpp.h>
#include <iostream>
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/mobility/contract/IMobility.h"

#include "nodes/mec/utils/httpUtils/httpUtils.h"

#include "apps/mec/ViApp/VirtualisationInfrastructureApp.h"

// simu5g httputils
#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/httpUtils/json.hpp"

#include "nodes/mec/utils/MecCommon.h"


using namespace omnetpp;

enum State {INIT, RELEASING};
class VirtualisationInfrastructureApp;

class ClientResourceApp : public cSimpleModule, public inet::TcpSocket::ICallback
{

    VirtualisationInfrastructureApp* viApp;
    static simsignal_t parkingReleased_;

  public:
    ClientResourceApp();
    virtual ~ClientResourceApp();


  protected:

      State appState;

      // Network communication
      inet::TcpSocket tcpSocket;
      HttpBaseMessage* currentHttpMessage;
      std::string buffer;

      inet::L3Address localIPAddress;
      int localPort;

      inet::L3Address destIPAddress;
      int destPort;

      int viPort;

      // App execution time
      simtime_t startTime;
      simtime_t parkTime;

      // Other app parameters
      int minReward;
      std::string choosenReward;

      // Available resources
      // Loaded from the host
      cModule* host; // any device host this application needs to provide some resources
      ResourceDescriptor localResources;


      // Mobility data
      inet::IMobility *mobility;

      virtual void initialize(int stage) override;
      virtual void handleMessage(cMessage *msg) override;
      virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
      virtual void finish() override;

      /*Utility methods*/
      virtual void connectToSRR();
      virtual void handleSelfMessage(cMessage *msg);
      virtual void sendRewardRequest(); // GET Request
      virtual void sendRegisterRequest(); // POST Request
      virtual void sendReleaseMessage(); // DELETE Request
      virtual void handleResponse(HttpResponseMessage* response);
      virtual std::string processRewards(nlohmann::json jsonResponseBody);
      virtual void close();


      // Callback methods
      virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override;
      virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override{EV << "ClientResourceApp::SocketAvailable" << endl;};
      virtual void socketEstablished(inet::TcpSocket *socket) override;
      virtual void socketPeerClosed(inet::TcpSocket *socket) override {close();}
      virtual void socketClosed(inet::TcpSocket *socket) override {EV << "ClientResourceApp::connection closed" << endl;}
      virtual void socketFailure(inet::TcpSocket *socket, int code) override {}
      virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
      virtual void socketDeleted(inet::TcpSocket *socket) override {}

  public:
      State getState();
};

#endif
