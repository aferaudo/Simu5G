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

#ifndef __SIMU5G_PUBLISHERBASE_H_
#define __SIMU5G_PUBLISHERBASE_H_

// Utils HTTP
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"

// Inet
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "nodes/mec/utils/httpUtils/json.hpp"

#include <omnetpp.h>

using namespace omnetpp;

/**
 * This class allows to publish content on a HTTP broker
 * - POST requests on serverhost:serverport/publish/
 * - open TCP socket with a Broker (TCP client)
 */
class PublisherBase : public inet::ApplicationBase, public inet::TcpSocket::ICallback
{
  HttpBaseMessage* currentHttpMessage;
  std::string buffer;

  public:
    PublisherBase();
    ~PublisherBase() {};
  protected:

    std::string publishURI;

    inet::TcpSocket brokerSocket;

    // broker parameters
    int brokerPort; // initialised in subclass
    int localToBrokerPort; //initilised in subclass
    inet::L3Address brokerIpAddress; //initialised in subclass

    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    // utility methods
    virtual void connectToBroker();

    // ApplicationBase Methods (AppliacationBase extends cSimpleModule and ILyfecycle (used to support application lyfecycle))
    virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
    virtual void handleStartOperation(inet::LifecycleOperation *operation) override;
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override {};
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override {};
    virtual void finish() override {};
    virtual void refreshDisplay() const override {};


    // Callback methods
    virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override;
    virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override { EV << "PublisherBase::SocketAvailable - nothing to do" << endl;};
    virtual void socketEstablished(inet::TcpSocket *socket) override {EV << "PublisherBase::broker socket established" << endl;}
    virtual void socketPeerClosed(inet::TcpSocket *socket) override {brokerSocket.close();}
    virtual void socketClosed(inet::TcpSocket *socket) override {EV << "PublisherBase::Cannot publish message anymore socket closed" << endl;}
    virtual void socketFailure(inet::TcpSocket *socket, int code) override {}
    virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
    virtual void socketDeleted(inet::TcpSocket *socket) override {}

    // Abstract methods
    /*
     * Publish through HTTP Requests:
     * - POST posting new content
     * - DELETE delete posted content
     * - PUT update posted content
     */
    virtual void publish(const char* type, const char *body, int id=-1) = 0;
};

#endif
