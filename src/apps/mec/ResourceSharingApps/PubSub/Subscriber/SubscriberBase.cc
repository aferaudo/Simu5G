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

#include "SubscriberBase.h"

//Define_Module(SubscriberBase);


SubscriberBase::SubscriberBase()
{
    currentHttpMessageBuffer_ = nullptr;
    currentHttpMessageServed_ = nullptr;
}

SubscriberBase::~SubscriberBase()
{
    std::cout<<"called subscriberBase destructor" << endl;
    if(nextEvent != nullptr)
        cancelAndDelete(nextEvent);
}

void SubscriberBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        EV << "SubscriberBase::initialising parameters" << endl;
    }

    EV << "SubscriberBase::Initialisation" << endl;

    inet::ApplicationBase::initialize(stage);

}

void SubscriberBase::handleStartOperation(inet::LifecycleOperation *operation)
{
    EV <<"SubscriberBase::local binding and position computation"<<endl;
    //inet::IMobility *mod = check_and_cast<inet::IMobility *>(host->getSubmodule("mobility"));
    // center = mod->getCurrentPosition();
    //center = inet::Coord;
//    center.setX(0);
//    center.setY(0);
//    center.setZ(0);

    // Socket local binding
    tcpSocket.setOutputGate(gate("socketOut"));
    tcpSocket.bind(localToBrokerPort);
    tcpSocket.setCallback(this);
    appState = UNSUB;
    nextEvent = new cMessage("nextEvent");
    // drawing a cricle
//    host->getParentModule()->getDisplayString().setTagArg("p", 0, center.getX());
//    host->getParentModule()->getDisplayString().setTagArg("p", 1, center.getY());
//    host->getParentModule()->getDisplayString().setTagArg("r", 0, radius);

//    cMessage* msg = new cMessage("connect");
//    scheduleAt(simTime()+0.01, msg);
}

void SubscriberBase::handleMessageWhenUp(omnetpp::cMessage *msg)
{
    std::cout << "Subscriber base: received message: " << msg->getName() << " " << msg->isSelfMessage() << endl;
    if(msg->isSelfMessage() && strcmp(msg->getName(), "connectToBroker") == 0)
    {
        EV << "SubscriberBase:: connecting to the broker" << endl;
        connectToBroker();

        delete msg;
    }
    else if(msg->isSelfMessage() && strcmp(msg->getName(), "nextEvent") == 0)
    {

        if(httpMessageQueue_.size() != 0)
        {

            currentHttpMessageServed_ = httpMessageQueue_.front();
            httpMessageQueue_.pop();
            manageNotification();
            EV << "SubscriberBase::http message to be processed: " << httpMessageQueue_.size() << endl;
            if(!nextEvent->isScheduled() && httpMessageQueue_.size() > 0)
               scheduleAt(simTime(), nextEvent);
        }

        if(currentHttpMessageServed_ != nullptr)
        {
            currentHttpMessageServed_ = nullptr;
        }


        //delete msg;
    }
    else if (msg->isSelfMessage() && strcmp(msg->getName(), "unsub") == 0)
    {
        EV << "SubscriberBase:: received unsub" << endl;
        unsubscribe();
        delete msg;
    }
    else
    {
        if(msg->arrivedOn("socketIn"))
        {
            if (tcpSocket.belongsToSocket(msg))
                tcpSocket.processMessage(msg);
        }
        else{
            throw cRuntimeError("Unknown message");
            delete msg;
        }
    }

}

void SubscriberBase::connectToBroker()
{
    tcpSocket.renewSocket();
    if (brokerIPAddress.isUnspecified()) {
        EV_ERROR << "SubscriberBase::Connecting to " << brokerIPAddress << " port=" << brokerPort << ": cannot resolve destination address\n";
//        throw cRuntimeError("Broker address is unspecified!");
    }
    else {
        EV << "SubscriberBase::Connecting to " << brokerIPAddress << " port=" << brokerPort << endl;
        tcpSocket.connect(brokerIPAddress, brokerPort);
    }
}


void SubscriberBase::socketEstablished(inet::TcpSocket *socket)
{
    EV << "SubscriberBase::connection established!" << endl;
    serverHost = tcpSocket.getRemoteAddress().str() + ":" + std::to_string(tcpSocket.getRemotePort());

    //EV << "SubscriberBase::subscribing" << endl;
    //sendSubscription();
}

void SubscriberBase::sendSubscription()
{
    EV << "SubscriberBase::subscriber to " << brokerIPAddress.str() << endl;


//    EV << "SubscriptionBody - before request!: \n " << subscriptionBody_.dump().c_str() << endl;
//    EV << "subscribeURI: " << subscribeURI.c_str() << endl;
    Http::sendPostRequest(&tcpSocket, subscriptionBody_.dump().c_str(), serverHost.c_str(), subscribeURI.c_str());
    appState = SUB;
}

//nlohmann::json SubscriberBase::infoToJson()
//{
//    /*
//     * Return a json object with
//     * clientId: int
//     * subscription: obj
//     *
//     * Subscription obj:
//     *      coordinateX:
//     *      coordinateY:
//     *      coordinateZ: -> typically null
//     *      radius
//     * */
//
//    nlohmann::json jsonObj = nlohmann::json::object();
//
//    jsonObj["clientId"] = getId();
//    jsonObj["clientWebhook"] = webHook;
//    jsonObj["subscription"]["coordinateX"] = center.getX();
//    jsonObj["subscription"]["coordinateY"] = center.getY();
//    jsonObj["subscription"]["coordinateZ"] = center.getZ();
//    jsonObj["subscription"]["radius"] = radius;
//
//    return jsonObj;
//}

void SubscriberBase::socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent)
{
    /*
     * Two types of messages:
     * subscription phase
     *  - Response --> After first subscription
     *  - Request --> notification that something happened
     * unsubscription phase
     *  - Response --> how delete went;
     */

    EV << "SubscriberBase:: message received from the broker..." << endl;

    std::vector<uint8_t> bytes =  packet->peekDataAsBytes()->getBytes();
    delete packet;

    std::string msg(bytes.begin(), bytes.end());
    bool res = Http::parseReceivedMsg(tcpSocket.getSocketId(), msg, completedMessageQueue, &buffer, &currentHttpMessageBuffer_);

    if(res)
    {
        while(completedMessageQueue.getLength() > 0)
        {
            HttpBaseMessage* currentHttpMessage = check_and_cast<HttpBaseMessage*>(completedMessageQueue.pop());
            switch(appState)
            {
                case SUB:
                {

                    currentHttpMessage->setSockId(tcpSocket.getSocketId());
                    if(currentHttpMessage->getType() == RESPONSE || currentHttpMessage->getType() == REQUEST)
                    {
                        EV << "SubscriberBase::Put notification in queue " << endl;
                        // In case of multiple notification a queue may be needed (AMS scenario)
                        httpMessageQueue_.push(currentHttpMessage);

                        if(!nextEvent->isScheduled())
                        {
                           std::cout<<"Scheduling nextEvent " << httpMessageQueue_.size() << endl;

                           // so far no computation time has been added
                           scheduleAt(simTime(), nextEvent);
                        }

                    }
                    else
                    {
                        EV << "SubscriberBase::Not recognised message (state = SUB)..."<<endl;
                    }
                    break;
                }
                case UNSUB:
                {

                    if(currentHttpMessage->getType() == RESPONSE)
                    {
                        HttpResponseMessage *response = dynamic_cast<HttpResponseMessage*> (currentHttpMessage);
                        EV << "SubscriberBase::Results unsubscription: " << std::to_string(response->getCode()) << endl;
                    }
                    else
                    {
                        EV << "SubscriberBase::Not Recognised message (state = UNSUB)..." << endl;
                    }
                    EV << "SubscriberBase::Closing socket with the broker after unsubscription.. bye" << endl;
                    tcpSocket.close();

                }
            }
        }
    }


//    if(currentHttpMessage != nullptr)
//    {
//        currentHttpMessage = nullptr;
//    }

}

void SubscriberBase::handleCrashOperation(inet::LifecycleOperation *operation)
{
    if (operation->getRootModule() != getContainingNode(this))
        tcpSocket.destroy();
}

void SubscriberBase::handleStopOperation(inet::LifecycleOperation *operation)
{
    if (tcpSocket.getState() == inet::TcpSocket::CONNECTED || tcpSocket.getState() == inet::TcpSocket::CONNECTING || tcpSocket.getState() == inet::TcpSocket::PEER_CLOSED)
        tcpSocket.close();
}

void SubscriberBase::unsubscribe()
{
    appState = UNSUB;
    EV << "SubscriberBase::called method unsubscribe - send delete request!" << endl;

    EV << "SubscriberBase::delete URI" << unsubscribeURI << endl;
    //std::string serverHost = tcpSocket.getRemoteAddress().str() + ":" + std::to_string(tcpSocket.getRemotePort());

    Http::sendDeleteRequest(&tcpSocket, serverHost.c_str(), unsubscribeURI.c_str());

}

//
//void SubscriberBase::refreshDisplay() const
//{
//    // Drawing circle
//    cOvalFigure *figure = new cOvalFigure("circle");
//    figure->setBounds(cFigure::Rectangle(100,100, radius, radius));
//    figure->setLineWidth(2);
//    figure->setLineStyle(cFigure::LINE_DOTTED);
//    figure->setPosition(cFigure::Point(center.getX(), center.getY()), cFigure::ANCHOR_CENTER);
//
//}


