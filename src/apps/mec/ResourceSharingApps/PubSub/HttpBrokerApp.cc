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
    currentResourceReleaseId = -1;
    currentResourceNewId = -1;
    serverSocket = nullptr;
    currentHttpMessage = nullptr;
    totalResources.clear();
    subscribers.clear();

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
    inet::ApplicationBase::initialize(stage);

}

void HttpBrokerApp::handleStartOperation(inet::LifecycleOperation *operation)
{

    EV << "HttpBrokerApp::handleStartOperation - local Address: " << par("localAddress") << " port: " << localPort << endl;
    localIPAddress = inet::L3AddressResolver().resolve(par("localAddress"));
    EV << "HttpBrokerApp::handleStartOperation - local Address resolved: "<< localIPAddress << endl;

    EV << "HttpBrokerApp::broker listening..."<<endl;
    serverSocket = new inet::TcpSocket();
    serverSocket->setOutputGate(gate("socketOut"));
    serverSocket->bind(localIPAddress, localPort);
    serverSocket->setCallback(this); //set this as callback

    EV << "HttpBrokerApp::broker listening..."<<endl;
    serverSocket->listen();

    // Response Test
    //cMessage* msg = new cMessage("mock");
    //scheduleAt(simTime(), msg);
}

void HttpBrokerApp::handleMessageWhenUp(cMessage *msg)
{
    if(msg->isSelfMessage() && strcmp(msg->getName(), "mock") == 0)
    {
        EV << "HttpBrokerApp::mock" << endl;
        mockResources();
        delete msg;
    }
    else if (msg->isSelfMessage() && strcmp(msg->getName(), "sendFakeNot") == 0)
    {
        notifyNewResources();
    }
    else
    {
        if(msg->arrivedOn("socketIn"))
        {
            EV << "HttpBrokerApp::Data Arrived on socket"<< endl;
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
            *   - Subscribe - /broker/subscribe/
            *   - Publish - /broker/publish/
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
    EV << "HttpBrokerApp::handleRequest - request method " << currentRequestMessageServed->getMethod() << endl;

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
    EV << "HttpBrokerApp::handlePOSTRequest"<< endl;
    std::string uri = currentRequestMessageServed->getUri();
    inet::TcpSocket *sock = check_and_cast<inet::TcpSocket *>(socketMap.getSocketById(currentRequestMessageServed->getSockId()));
    nlohmann::json jsonBody = nlohmann::json::parse(currentRequestMessageServed->getBody());
    if(uri.compare(baseUri + "subscribe/") == 0)
    {
        // subscription messages
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

                        // Recording subscriber data
                        subscribers.insert(std::pair<int, SubscriberEntry>(sub.clientId, sub));
                        clientSocketMap.insert(std::pair<int, int>(sub.clientId, sock->getSocketId()));
                        printSubscribers();
                        sendResponse(sock, &sub);

                        // DEBUG
//                        cMessage* msg = new cMessage("sendFakeNot");//only for testing
//                        scheduleAt(simTime()+0.01, msg);
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
            EV << "HttpBrokerApp::subscription - ClientId unspecified" << endl;
            Http::send400Response(sock);
        }
    }
    else if (uri.compare(baseUri + "publish/") == 0)
    {
        if(jsonBody.contains("id"))
        {
           ClientResourceEntry c;
           c.clientId = jsonBody["id"];

           if(jsonBody.contains("ipAddress"))
           {
               std::string ipAddress_str = jsonBody["ipAddress"];
               c.ipAddress = inet::L3Address(ipAddress_str.c_str());

               if(jsonBody.contains("ram"))
               {
                   c.resources.ram = jsonBody["ram"];
                   if(jsonBody.contains("disk"))
                   {
                      c.resources.disk = jsonBody["disk"];
                      if(jsonBody.contains("cpu"))
                      {
                          c.resources.cpu = jsonBody["cpu"];
                          if(jsonBody.contains("viPort"))
                          {
                                c.viPort = jsonBody["viPort"];
                                totalResources.insert(std::pair<int, ClientResourceEntry>(c.clientId, c));
                                currentResourceNewId = c.clientId;
                                printAvailableResources();
                                sendNotification(NEW);
                          }

                      }
                      else
                      {
                          EV << "HttpBrokerApp::publisher - cpu address unspecified" << endl;
                          Http::send400Response(sock);
                      }
                   }
                   else
                   {
                       EV << "HttpBrokerApp::publisher - disk address unspecified" << endl;
                       Http::send400Response(sock);
                   }
               }
               else
               {
                   EV << "HttpBrokerApp::publisher - ram address unspecified" << endl;
                   Http::send400Response(sock);
               }
           }
           else
           {
               EV << "HttpBrokerApp::publisher - ip address unspecified" << endl;
               Http::send400Response(sock);
           }
        }
        else
        {
            EV << "HttpBrokerApp::publisher - id unspecified" << endl;
            Http::send400Response(sock);
        }
    }
    else
    {
        // Bad uri
        Http::send404Response(sock);

    }
}

void HttpBrokerApp::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo)
{
    auto newSocket = new inet::TcpSocket(availableInfo);
    newSocket->setOutputGate(gate("socketOut")); //gate connection
    newSocket->setCallback(this); // All sockets are managed by a single thread

    EV << "HttpBrokerApp::AvailableInfo " << availableInfo << endl;

    EV << "HttpBrokerApp::Socket " << newSocket->getSocketId() << endl;

    EV << "HttpBrokerApp::Storing socket " << endl;
    socketMap.addSocket(newSocket);

    serverSocket->accept(availableInfo->getNewSocketId());
    EV << "HttpBrokerApp::SocketMap size " << socketMap.size() << ", id: " << newSocket->getSocketId() << ", port: " << newSocket->getRemotePort() << endl;

}

void HttpBrokerApp::sendNotification(Notification type)
{
    switch(type)
    {
        case(NEW):
        {
            notifyNewResources();
            break;
        }
        case(RELEASE):
        {
            notifyResourceRelease();
            break;
        }
        default:
        {
            EV << "HttpBrokerApp::UNRECOGNISED notification type" << endl;
        }
    }
}

void HttpBrokerApp::sendResponse(inet::TcpSocket *sock, SubscriberEntry *s)
{
    EV << "HttpBrokerApp::sending response" << endl;
    nlohmann::json jsonObj = filterInitialResourcesToSend(s->clientId, s->subscriberPos, s->subscriberRadius);
    Http::send200Response(sock, jsonObj.dump().c_str());

}


void HttpBrokerApp::notifyNewResources()
{
    EV << "HttpBrokerApp::notifyNewResources"<<endl;

    if(subscribers.empty())
    {
        EV << "HttpBrokerApp::Not subscribers yet!" << endl;
        return;
    }
    auto resource = totalResources.find(currentResourceNewId);
    if(resource == totalResources.end())
    {
        EV << "HttpBrokerApp::Vehicle ID - " << currentResourceNewId << " - not found!" << endl;
        return;
    }
    EV << "HttpBrokerApp::computing socket id" << endl;

    SubscriberEntry s = filterSubscribers(&resource->second);

    inet::TcpSocket *sock = check_and_cast<inet::TcpSocket *> (socketMap.getSocketById(clientSocketMap.find(s.clientId)->second));

    std::string webhookHost = s.clientAddress.str() + ":" + std::to_string(s.clientPort);
    if(resource->second.vimId == -1)
    {
        resource->second.vimId = s.clientId;
        nlohmann::json jsonObj = nlohmann::json::object();
        jsonObj[std::to_string(resource->first)]["ipAddress"] = resource->second.ipAddress.str();
        jsonObj[std::to_string(resource->first)]["viPort"] = resource->second.viPort;
        jsonObj[std::to_string(resource->first)]["ram"] = resource->second.resources.ram;
        jsonObj[std::to_string(resource->first)]["disk"] = resource->second.resources.disk;
        jsonObj[std::to_string(resource->first)]["cpu"] = resource->second.resources.cpu;

        EV << "HttpBrokerApp::Send Notification Post to " << webhookHost << endl;
        EV << "HttpBrokerApp::Json- " << jsonObj.dump() << endl;
        Http::sendPostRequest(sock, jsonObj.dump().c_str(), webhookHost.c_str(), s.clientWebHook.c_str());
        printAvailableResources();
    }
    else
    {
        EV << "HttpBrokerApp::notification system - Client already allocated to " << resource->second.vimId << endl;
    }



}

void HttpBrokerApp::notifyResourceRelease()
{
    EV << "HttpBrokerApp::notifyResourceRelease" << endl;

    // Get associated subscriber
    auto resource = totalResources.find(currentResourceReleaseId);

    int subId = resource->second.vimId;
    inet::TcpSocket *sock = check_and_cast<inet::TcpSocket*>(socketMap.getSocketById(clientSocketMap.find(subId)->second));

    SubscriberEntry s = subscribers.find(subId)->second;

    EV << "HttpBrokerApp::notifying " << s.clientId << " subscriber, type: RESOURCE RELEASE" << endl;

   // Preparing uri
    std::string uri(s.clientWebHook + std::to_string(currentResourceReleaseId));

    std::string subscriberHost = s.clientAddress.str() + ":" + std::to_string(s.clientPort);

    EV << "HttpBrokerApp::host: " << subscriberHost << endl;

    // notify only the vim where resources are allocated
    Http::sendDeleteRequest(sock, subscriberHost.c_str(), uri.c_str());

    // Delete resource from availables
    totalResources.erase(currentResourceReleaseId);
}


nlohmann::json HttpBrokerApp::filterInitialResourcesToSend(int vimId, inet::Coord, double radius)
{
    // TODO add a filtering method
    EV <<"HttpBrokerApp::filterInitialResourcesToSend - no filterning so far" << endl;
    nlohmann::json jsonObj = nlohmann::json::object();

    std::map<int, ClientResourceEntry>::iterator it = totalResources.begin();

//    std::stringstream client;
//    hostStream << localAddress<< ":" << localPort;
    while(it != totalResources.end())
    {
        it->second.vimId = vimId;
        jsonObj[std::to_string(it->second.clientId)]["ipAddress"] = it->second.ipAddress.str();
        jsonObj[std::to_string(it->second.clientId)]["viPort"] = it->second.viPort;
        jsonObj[std::to_string(it->second.clientId)]["ram"] = it->second.resources.ram;
        jsonObj[std::to_string(it->second.clientId)]["disk"] = it->second.resources.disk;
        jsonObj[std::to_string(it->second.clientId)]["cpu"] = it->second.resources.cpu;
        ++it;
    }
    EV << "jsonobj:: " << jsonObj.dump() << endl;

    return jsonObj;
}

void HttpBrokerApp::printSubscribers()
{
    std::map<int, SubscriberEntry>::iterator it = subscribers.begin();
    EV << "#######################################" << endl;
    EV << "HttpBrokerApp::subscribers updated" << endl;
    while(it != subscribers.end())
    {
        EV<<"HttpBrokerApp::ClientId: " << it->first << endl;
        EV<<"HttpBrokerApp::ClientWebhook: " << it->second.clientWebHook << endl;
        EV<<"HttpBrokerApp::ClientPort: " << it->second.clientPort << endl;
        ++it;
        EV << "--------------------------------------" << endl;
    }

    EV << "#######################################" << endl;
}

void HttpBrokerApp::printAvailableResources()
{
    std::map<int, ClientResourceEntry>::iterator it = totalResources.begin();
    EV << "#######################################" << endl;
    EV << "HttpBrokerApp::resources updated" << endl;
    while(it != totalResources.end())
    {
        EV<<"HttpBrokerApp::ClientId: " << it->first << endl;
        EV<<"HttpBrokerApp::Manager: " << it->second.vimId << endl;
        ++it;
        EV << "--------------------------------------" << endl;
    }

    EV << "#######################################" << endl;
}

SubscriberEntry HttpBrokerApp::filterSubscribers(ClientResourceEntry *c)
{
    /*
     * TODO
     * Check in each subscriber its coordinates:
     * - if a device is found, return that device and stop
     * - if a device is not found, return null
     */

    // For testing it will return the begin of the set
    std::map<int, SubscriberEntry>::iterator someElementIterator = subscribers.begin();
    return someElementIterator->second;

//    std::set<SubscriberEntry*>::iterator it = subscribers.begin();
//
//    while(it != subscribers.end())
//    {
//
//        ++it;
//    }

}

void HttpBrokerApp::socketPeerClosed(inet::TcpSocket *socket)
{
    socket->setState(inet::TcpSocket::PEER_CLOSED);
    EV << "HttpBrokerApp::socket " << socket->getSocketId() << " peer-closed" << endl;

    socket->close();
}

void HttpBrokerApp::socketClosed(inet::TcpSocket *socket)
{
    EV << "HttpBrokerApp::socket " << socket->getSocketId() << " closed" << endl;
    socketMap.removeSocket(socket);
    // if subscriber delete

    bool res = unregisterSubscriber(socket->getSocketId());
    if(res)
    {
        EV << "HttpBrokerApp::Subscriber unregistred!"<< endl;
    }

}

void HttpBrokerApp::socketFailure(inet::TcpSocket *socket, int code)
{
    EV << "HttpBrokerApp::socket failure with code: " << code << ", remoteADD: " << socket->getRemoteAddress() << endl;
    socketMap.removeSocket(socket);
    //if subscriber delete
    bool res = unregisterSubscriber(socket->getSocketId());
    if(res)
    {
        EV << "HttpBrokerApp::Subscriber unregistred!"<< endl;
    }
}

bool HttpBrokerApp::unregisterSubscriber(int sockId, int subId)
{
    if(subId == -1)
    {
        for(auto& it : clientSocketMap)
        {
            if(it.second == sockId){
                subId = it.first;
                // subscriber exists
                break;
            }
        }
    }

    if(subId != -1) //it's not an else because this needs to be checked after the first if in any case
    {
        // Subscriber exists
        // Deleting subscriber
        //std::set<SubscriberEntry>::iterator it = subscribers.begin();
        EV << "HttpBrokerApp::Unregister subscriber " << std::to_string(subId) << endl;
        subscribers.erase(subId);

        //deleting socket association
        clientSocketMap.erase(subId);
        // Make resources available for next subscribers
        std::map<int, ClientResourceEntry>::iterator itres = totalResources.begin();
        while(itres != totalResources.end())
        {
            if(itres->second.vimId == subId)
            {
                itres->second.vimId = -1;
            }
            ++ itres;
        }

        return true;
    }

    return false;

}

void HttpBrokerApp::handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed)
{
    EV << "HttpBrokerApp::Received DELETE request - " << currentRequestMessageServed->getUri() << endl;
    std::string uri = currentRequestMessageServed->getUri();

    std::string delimetersub("subscribe");
    std::string delimeterpub("publish");

    // valid uri: /resourceRegisterApp/v1/availableResources
    std::string urisub = uri.substr(0, uri.find(delimetersub) + delimetersub.length());
    std::string uripub = uri.substr(0, uri.find(delimeterpub) + delimeterpub.length());

    inet::TcpSocket *sock = check_and_cast<inet::TcpSocket *>(socketMap.getSocketById(currentRequestMessageServed->getSockId()));
    if(urisub.compare(baseUri + "subscribe") == 0)
    {
        uri.erase(0, uri.find(delimetersub) + delimetersub.length()+1);
        int subId = std::atoi(uri.c_str());
        EV << "HttpBrokerApp:: DELETE - subscribe - URI correct, ID extracted: " << subId << endl;
        bool res = unregisterSubscriber(sock->getSocketId(), subId);
        if(res)
        {
            EV << "HttpBrokerApp:: DELETE - unregister subscribe OK!" << endl;
            // DEBUG
            printSubscribers();
            printAvailableResources();

            // Send reply..
            Http::send204Response(sock);
        }
        else
        {
            EV << "HttpBrokerApp:: DELETE - no subscriber found" << endl;
            Http::send404Response(sock);
        }
    }
    else if(uripub.compare(baseUri + "publish") == 0)
    {
        uri.erase(0, uri.find(delimeterpub) + delimeterpub.length()+1);
        currentResourceReleaseId = std::atoi(uri.c_str());
        EV << "HttpBrokerApp:: DELETE - publish - URI correct, ID extracted: " << currentResourceReleaseId << endl;
        sendNotification(RELEASE);
        printAvailableResources();

    }
    else
    {
        EV << "HttpBrokerApp::DELETE-Invalid URI: " << uri<< endl;
        Http::send400Response(sock);
    }

}

void HttpBrokerApp::mockResources()
{

    ClientResourceEntry c;
    c.clientId = 50000;
    c.viPort = 3333;
    c.ipAddress = inet::L3Address("192.168.1.1");
    c.resources.ram = 80000;
    c.resources.cpu = 3000;
    c.resources.disk = 1200000;
    totalResources.insert(std::pair<int, ClientResourceEntry>(c.clientId, c));

}
