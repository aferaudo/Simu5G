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

#include "HttpBrokerApp.h"

Define_Module(HttpBrokerApp);

HttpBrokerApp::HttpBrokerApp()
{
    baseUri = "/broker/";
    serverSocket = nullptr;
    availableResources_.clear();
    //subscriptions_.clear();
}

HttpBrokerApp::~HttpBrokerApp()
{
    delete serverSocket;
    socketMap.deleteSockets();
}


void HttpBrokerApp::initialize(int stage)
{
    EV << "HttpBrokerApp::initialize stage " << stage << endl;
    if (stage == inet::INITSTAGE_LOCAL)
    {
        // Initialise lists and other parameters
        EV << "HttpBrokerApp::initialize" << endl;
        localPort = par("localPort");

        // Do we need other parameters?
    }
    ApplicationBase::initialize(stage);

}

void HttpBrokerApp::handleStartOperation(inet::LifecycleOperation *operation)
{
    serverSocket = new inet::TcpSocket();
    serverSocket->setOutputGate(gate("socketOut"));
    serverSocket->bind(localIPAddress, localPort);
    serverSocket->setCallback(this); //set this as callback

    EV << "HttpBrokerApp::broker listening..."<<endl;
    serverSocket->listen();
}

void HttpBrokerApp::handleMessageWhenUp(cMessage *msg)
{
    if(msg->isSelfMessage())
    {
        EV << "HttpBrokerApp::no behaviour defined for self msgs" << endl;
        delete msg;
    }
    else
    {
        if(msg->arrivedOn("socketIn"))
        {
            inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(socketMap.findSocketFor(msg));
            if (socket)
                socket->processMessage(msg);
            else if (serverSocket->belongsToSocket(msg))
                serverSocket->processMessage(msg);
        }
        else{
            throw cRuntimeError("Unknown message");
            delete msg;
        }
    }
}

void HttpBrokerApp::publish(ClientResourceEntry c)
{
    availableResources_.insert(std::pair<int, ClientResourceEntry>(c.clientId, c));
    // send notify
}

void HttpBrokerApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent)
{

    EV << "HttpBrokerApp::Received packet on socket: " << packet->peekData() << endl;
    packet->removeControlInfo();

    std::vector<uint8_t> bytes =  packet->peekDataAsBytes()->getBytes();
    delete packet;

    std::string msg(bytes.begin(), bytes.end());

    EV << "HttpBrokerApp::Packet bytes: " << msg;

    bool res = Http::parseReceivedMsg(msg, &buffer, &currentHttpMessage);
    if(res)
    {
        currentHttpMessage->setSockId(socket->getSocketId());
        if(currentHttpMessage->getType() == REQUEST)
        {
           /*
            * Post Requests:
            *   - Subscribe
            *   - Publish
            * Put:
            *   - Update subscriberInformation
            * Delete:
            *   - Remove subscription
            * */
            HttpRequestMessage *request = check_and_cast<HttpRequestMessage*>(currentHttpMessage);
            handleRequest(request);
        }
        else
        {
            delete currentHttpMessage;
        }

        // After request has been processed it can be deleted
        if(currentHttpMessage != nullptr)
        {
          currentHttpMessage = nullptr;
        }
    }


}

void HttpBrokerApp::handleRequest(const HttpRequestMessage *currentRequestMessageServed)
{
    EV << "ResourceRegisterThread::handleRequest - request method " << currentRequestMessageServed->getMethod() << endl;

    if(std::strcmp(currentRequestMessageServed->getMethod(),"GET") == 0)
    {
        Http::send404Response(check_and_cast<inet::TcpSocket *>(socketMap.getSocketById(currentRequestMessageServed->getSockId())), "GET not supported");
    }
    else if(std::strcmp(currentRequestMessageServed->getMethod(),"POST") == 0)
    {
        handlePOSTRequest(currentRequestMessageServed);
    }
    else if(std::strcmp(currentRequestMessageServed->getMethod(),"DELETE") == 0)
    {
        handleDELETERequest(currentRequestMessageServed);
    }
    else
    {
        handlePUTRequest(currentRequestMessageServed);
    }
}

void HttpBrokerApp::handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed)
{
    std::string uri = currentRequestMessageServed->getUri();
    inet::TcpSocket *sock = check_and_cast<inet::TcpSocket *>(socketMap.getSocketById(currentRequestMessageServed->getSockId()));
    if(uri.compare(baseUri + "subscribe/") == 0)
    {
        // subscription messages
        nlohmann::json jsonBody = nlohmann::json::parse(currentRequestMessageServed->getBody());
        nlohmann::json jsonSub = nlohmann::json::object();
        if(jsonBody.contains("clientId"))
        {
            SubscriberEntry sub;
            sub.clientId = jsonBody["clientId"];
            sub.clientAddress = sock->getRemoteAddress();
            sub.clientPort = sock->getRemotePort();

            if(jsonBody.contains("clientWebhook"))
            {
                sub.clientWebHook = jsonBody["clientWebhook"];

                if(jsonBody.contains("subscription"))
                {
                    jsonSub = jsonBody["subscription"];
                    if(jsonSub.contains("coordinateX") && jsonSub.contains("coordinateY")
                            && jsonSub.contains("coordinateZ") && jsonSub.contains("radius"))
                    {
                        sub.subscriberPos.setX(jsonSub["coordinateX"]);
                        sub.subscriberPos.setY(jsonSub["coordinateY"]);
                        sub.subscriberPos.setZ(jsonSub["coordinateZ"]);
                        sub.subscriberRadius = jsonSub["radius"];

                        EV << "New subscriber Added!" << endl;
                        EV << "Hook: " << sub.clientWebHook << endl;
                        EV << "Position - x: " << std::to_string(sub.subscriberPos.getX()) << ", y: " << std::to_string(sub.subscriberPos.getY())
                                << ", z: " << std::to_string(sub.subscriberPos.getZ()) << endl;
                        EV << "Radius: " << std::to_string(sub.subscriberRadius) << endl;
                        // Send list based on its position

                    }
                    else
                    {
                        EV << "HttpBrokerApp::subscription - subcription coordinate unspecified" << endl;
                        Http::send400Response(sock);
                    }

                }
                else
                {
                    EV << "HttpBrokerApp::subscription - subcription details unspecified" << endl;
                    Http::send400Response(sock);
                }
            }
            else
            {
                EV << "HttpBrokerApp::subscription - clientWebHook unspecified" << endl;
                Http::send400Response(sock);
            }

        }
        else
        {
            EV << "HttpBrokerApp::subscription - Clientid unspecified" << endl;
            Http::send400Response(sock);
        }
    }
    else if (uri.compare(baseUri + "publish/") == 0)
    {
        // publish messages
    }
    else
    {

    }
}

void HttpBrokerApp::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo)
{
    auto newSocket = new inet::TcpSocket(availableInfo);
    newSocket->setOutputGate(gate("socketOut")); //gate connection

    EV << "HttpBrokerApp::AvailableInfo " << availableInfo << endl;

    EV << "HttpBrokerApp::Socket " << newSocket->getSocketId() << endl;

    EV << "ResourceRegisterApp::Storing socket " << endl;
    socketMap.addSocket(newSocket);

    serverSocket->accept(availableInfo->getNewSocketId());
    EV << "HttpBrokerApp::SocketMap size " << socketMap.size() << ", id: " << newSocket->getSocketId() << ", port: " << newSocket->getRemotePort() << endl;

}
