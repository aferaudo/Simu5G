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

#ifndef __SIMU5G_HttpBrokerApp_H_
#define __SIMU5G_HttpBrokerApp_H_

#include <omnetpp.h>


//#include "apps/mec/ResourceSharingApps/PubSub/Subscriber/SubscriberBase.h"


// HTTP
#include "nodes/mec/MECPlatform/MECServices/packets/HttpMessages_m.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"

// Json
#include "nodes/mec/utils/httpUtils/json.hpp"

// inet
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/common/socket/SocketMap.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/geometry/common/Coord.h"




using namespace omnetpp;

/*
 * HTTP Publish-Subscribe Server
 * JSON Centric Messages
 * */

/**
 * This single-thread server acts as a broker where:
 *  - publishers are ResourceRegisterThread
 *  - subscribers are VIMs {subscriptions happen through http POST request}
 * It handles subscriber connections and dispatches publications based on the client coordinates
 */
struct ClientResourceEntry
{
    int clientId;
    inet::L3Address ipAddress;
//    ResourceDescriptor resources;
    std::string reward;
    // TODO add zone coordinates
};

struct SubscriberEntry
{
    int clientId;
    // this parameters maybe useful in case of connection lost
    inet::L3Address clientAddress;
    int clientPort;
    // -----
    std::string clientWebHook;
    inet::Coord subscriberPos;
    double subscriberRadius;
};

class HttpBrokerApp : public inet::ApplicationBase, public inet::TcpSocket::ICallback
{

  int localPort;

  inet::TcpSocket *serverSocket; // Listen incoming connections
  inet::SocketMap socketMap; //Stores the connections

  inet::L3Address localIPAddress; // same as resourceManager

  std::string baseUri;

  // Map with client resource info
  // key = client id
  // value = ClientResourceEntry;
  std::map <int, ClientResourceEntry> availableResources_;

  // Map subscribers
  // key = socket id
  // value = SubscriberBase
  std::map <int, SubscriberEntry> subscriptions_;

  HttpBaseMessage *currentHttpMessage; // current HttpRequest
  std::string buffer;



  protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

    // ApplicationBase methods
    virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
    virtual void handleStartOperation(inet::LifecycleOperation *operation) override;
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override{};
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override{};
    virtual void finish() override{};
    virtual void refreshDisplay() const override{};

    // internal: TcpSocket::ICallback methods
    virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) override;
    virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override;
    virtual void socketEstablished(inet::TcpSocket *socket) override {}
    virtual void socketPeerClosed(inet::TcpSocket *socket) override {}
    virtual void socketClosed(inet::TcpSocket *socket) override {}
    virtual void socketFailure(inet::TcpSocket *socket, int code) override { }
    virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
    virtual void socketDeleted(inet::TcpSocket *socket) override {}

    //HttpMethods API
    virtual void handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed);
    virtual void handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed) { }//Http::send404Response(sock, "PUT not supported"); };
    virtual void handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed) {EV << "Not implemented yet" << endl;}
    virtual void handleRequest(const HttpRequestMessage *currentRequestMessageServed);

  public:
    HttpBrokerApp();
    ~HttpBrokerApp();
    //virtual void init(int port, inet::L3Address ipAddress) { localPort = port; localIPAddress = ipAddress;}

    virtual void publish(ClientResourceEntry c);
};

#endif
