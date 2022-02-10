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
    cMessage* msg = new cMessage("mock");//only for testing
    scheduleAt(simTime(), msg);
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
        sendNotification();
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

void HttpBrokerApp::publish(ClientResourceEntry *c)
{
    totalResources.insert(c);
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

                        // Recording subscriber data
                        subscribers.insert(sub);
                        clientSocketMap.insert(std::pair<int, int>(sub.clientId, sock->getSocketId()));
                        printSubscribers();
                        sendResponse(sock, &sub);

                        cMessage* msg = new cMessage("sendFakeNot");//only for testing
                        scheduleAt(simTime()+0.01, msg);

                        //Sending current resources list (this depends on its position)
//                        sendNotification();
                        //Http::send200Response(sock, "{Done}");
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
        // publish messages
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


void HttpBrokerApp::sendNotification()
{
    EV << "HttpBrokerApp::sendNotification"<<endl;
    // TODO -> USE THE WEBHOOK
    printSubscribers();

    // MOCK CLIENT - This should be send by the subscriber
    ClientResourceEntry *c = new ClientResourceEntry();
    c->allocated = true;
    c->clientId = 46;
    c->viPort = 3333;
    c->ipAddress = inet::L3Address("192.168.1.2");
    c->resources.ram = 80000;
    c->resources.cpu = 3000;
    c->resources.disk = 1200000;
    EV << "HttpBrokerApp::computing socket id" << endl;
    SubscriberEntry s = filterSubscribers(c);
    //EV << socketMap << endl;
    inet::TcpSocket *sock = check_and_cast<inet::TcpSocket *> (socketMap.getSocketById(clientSocketMap.find(s.clientId)->second));

    std::string webhookHost = s.clientAddress.str() + ":" + std::to_string(s.clientPort);

    nlohmann::json jsonObj = nlohmann::json::object();
    jsonObj[std::to_string(c->clientId)]["ipAddress"] = c->ipAddress.str();
    jsonObj[std::to_string(c->clientId)]["viPort"] = c->viPort;
    jsonObj[std::to_string(c->clientId)]["ram"] = c->resources.ram;
    jsonObj[std::to_string(c->clientId)]["disk"] = c->resources.disk;
    jsonObj[std::to_string(c->clientId)]["cpu"] = c->resources.cpu;

    EV << "HttpBrokerApp::Send Notification Post to " << webhookHost << endl;
    EV << "HttpBrokerApp::Json- " << jsonObj.dump() << endl;
    Http::sendPostRequest(sock, jsonObj.dump().c_str(), webhookHost.c_str(), s.clientWebHook.c_str());


}


void HttpBrokerApp::sendResponse(inet::TcpSocket *sock, SubscriberEntry *s)
{
    EV << "HttpBrokerApp::sending response" << endl;
    nlohmann::json jsonObj = filterInitialResourcesToSend(s->subscriberPos, s->subscriberRadius);
    Http::send200Response(sock, jsonObj.dump().c_str());

}


nlohmann::json HttpBrokerApp::filterInitialResourcesToSend(inet::Coord, double radius)
{
    // TODO add a filtering method
    EV <<"HttpBrokerApp::filterInitialResourcesToSend - no filterning so far" << endl;
    nlohmann::json jsonObj = nlohmann::json::object();

    std::set<ClientResourceEntry*>::iterator it = totalResources.begin();

//    std::stringstream client;
//    hostStream << localAddress<< ":" << localPort;
    while(it != totalResources.end())
    {
        (*it)->allocated = true;
        jsonObj[std::to_string((*it)->clientId)]["ipAddress"] = (*it)->ipAddress.str();
        jsonObj[std::to_string((*it)->clientId)]["viPort"] = (*it)->viPort;
        jsonObj[std::to_string((*it)->clientId)]["ram"] = (*it)->resources.ram;
        jsonObj[std::to_string((*it)->clientId)]["disk"] = (*it)->resources.disk;
        jsonObj[std::to_string((*it)->clientId)]["cpu"] = (*it)->resources.cpu;
        ++it;
    }
    EV << "jsonobj:: " << jsonObj.dump() << endl;

    return jsonObj;
}

void HttpBrokerApp::printSubscribers()
{
    std::set<SubscriberEntry>::iterator it = subscribers.begin();
    EV << "#######################################" << endl;
    EV << "HttpBrokerApp::subscribers updated" << endl;
    while(it != subscribers.end())
    {
        EV<<"HttpBrokerApp::ClientId: " << (*it).clientId << endl;
        EV<<"HttpBrokerApp::ClientWebhook: " << (*it).clientWebHook << endl;
        EV<<"HttpBrokerApp::ClientPort: " << (*it).clientPort << endl;
        ++it;
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
    // For testing it will return the first device in the list
    std::set<SubscriberEntry>::iterator someElementIterator = subscribers.begin();
    EV << "ClientID qua: " << (*someElementIterator).clientId << endl;
    return (*someElementIterator);
//    std::set<SubscriberEntry*>::iterator it = subscribers.begin();
//
//    while(it != subscribers.end())
//    {
//
//        ++it;
//    }

}
void HttpBrokerApp::mockResources()
{

    ClientResourceEntry *c = new ClientResourceEntry();
    c->allocated = false;
    c->clientId = 45;
    c->viPort = 3333;
    c->ipAddress = inet::L3Address("192.168.1.1");
    c->resources.ram = 80000;
    c->resources.cpu = 3000;
    c->resources.disk = 1200000;
    totalResources.insert(c);

}
