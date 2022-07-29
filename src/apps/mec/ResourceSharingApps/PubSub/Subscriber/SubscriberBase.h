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

#ifndef __SIMU5G_SUBSCRIBERBASE_H_
#define __SIMU5G_SUBSCRIBERBASE_H_

// inet Application
#include "inet/applications/base/ApplicationBase.h"

// inet NETWORK
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

// inet Mobility
#include "inet/common/geometry/common/Coord.h"
#include "inet/mobility/contract/IMobility.h"

// Utility: from Simu5G
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "nodes/mec/utils/httpUtils/json.hpp"

#include <queue>

#include <omnetpp.h>

using namespace omnetpp;

enum SubscriberState {SUB, UNSUB};

class SubscriberBase: public inet::ApplicationBase, public inet::TcpSocket::ICallback
{

   public:
      SubscriberBase();
      ~SubscriberBase();

  protected:
    std::string subscribeURI;
    std::string unsubscribeURI;

    SubscriberState appState;

    cModule* host;
    cMessage *nextEvent;

    // HttpMessageManagement
    std::queue<HttpBaseMessage*> httpMessageQueue_;
    HttpBaseMessage* currentHttpMessageBuffer_;
    HttpBaseMessage* currentHttpMessageServed_;
    std::string buffer;
    omnetpp::cQueue completedMessageQueue;

    std::string webHook;
    std::string serverHost;
    inet::Coord center; // This is a fixed node so we consider it as a center of a circle
    double radius; // loaded from ned file - for dynamic mechosts

    nlohmann::ordered_json subscriptionBody_; // this depends on the broker accepted requests

    inet::TcpSocket tcpSocket;
    inet::L3Address brokerIPAddress;
    int brokerPort;
    int localToBrokerPort;

    // utility methods
    virtual void connectToBroker();
    virtual void sendSubscription();
    virtual nlohmann::json infoToJson();

    /*
     * This method sends a DELETE request to the broker
     * with its component id
     */
    virtual void unsubscribe();

    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
    virtual void handleStartOperation(inet::LifecycleOperation *operation)override;
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override;
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override;
//    virtual void finish() override {};
//    virtual void refreshDisplay() const override {};

    // TcpSocketCallback methods
    virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override;
    virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override{};
    virtual void socketEstablished(inet::TcpSocket *socket) override;
    virtual void socketPeerClosed(inet::TcpSocket *socket) override {tcpSocket.close();}
    virtual void socketClosed(inet::TcpSocket *socket) override {EV << "SubscriberBase::connection closed" << endl;}
    virtual void socketFailure(inet::TcpSocket *socket, int code) override {}
    virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
    virtual void socketDeleted(inet::TcpSocket *socket) override {}

    // Abstract methods
    virtual void manageNotification() = 0;
};

#endif
