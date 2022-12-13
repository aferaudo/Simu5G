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

#include "ExtMECWarningAlertApp.h"

#include <fstream>

// inet
#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet_m.h"

#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

// simu
#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"
#include "apps/mec/WarningAlert/packets/WarningAlertPacket_Types.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "nodes/mec/utils/httpUtils/json.hpp"

Define_Module(ExtMECWarningAlertApp);

using namespace omnetpp;

ExtMECWarningAlertApp::ExtMECWarningAlertApp()
{
}

ExtMECWarningAlertApp::~ExtMECWarningAlertApp()
{
}

void ExtMECWarningAlertApp::initialize(int stage)
{
    ExtMecAppBase::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    //retrieving parameters
    size_ = par("packetSize");

    // set Udp Socket
    ueSocket.setOutputGate(gate("socketOut"));

    localUePort = par("localUePort");
    ueSocket.bind(localUePort);

    EV << "ExtMECWarningAlertApp::initializing" << endl;
    mp1Socket_ = addNewSocket();

    MecServiceInfo *m1 = new MecServiceInfo();
    m1->name = "LocationService";
    MecServiceInfo *m2 = new MecServiceInfo();
    m2->name = "ApplicationMobilityService";

    // std MEC services needed
    servicesData_.push_back(m1);
    servicesData_.push_back(m2);
    std::cout << "Called initilize extmecappbase 1" <<endl;

    std::cout << "Array 0: " <<  servicesData_[0]->name << endl;
    std::cout << "Array 1: " <<  servicesData_[1]->name << endl;
    // connect with the service registry
    cMessage *msg = new cMessage("connectMp1");
    scheduleAt(simTime() + 0, msg);
    std::cout << "Called initilize extmecappbase 2" <<endl;
}

void ExtMECWarningAlertApp::handleSelfMessage(cMessage *msg)
{
    if(strcmp(msg->getName(), "connectMp1") == 0)
    {
        EV << "ExtMECWarningAlertApp::handleSelfMessage " << msg->getName() << endl;
        connect(mp1Socket_, mp1Address, mp1Port);
    }
    else if(strcmp(msg->getName(), "connectService") == 0)
    {
        // TODO here we should connect with each service
        // Location Service
        // Application Mobility Service
        EV << "ExtMECWarningAlertApp::handleMessage- " << msg->getName() << endl;
        if(!servicesData_[0]->host.addr.isUnspecified())
        {
            inet::TcpSocket *serviceSocket  = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(servicesData_[0]->sockid));
            if(serviceSocket != nullptr && serviceSocket->getState() != inet::TcpSocket::CONNECTED)
            {
                EV << "ExtMECWarningAlertApp::connecting to the service: " << servicesData_[0]->name << endl;
                connect(serviceSocket, servicesData_[0]->host.addr, servicesData_[0]->host.port);
            }
            else
            {
                EV << "ExtMECWarningAlertApp::handleSelfMessage - service socket is already connected or nullptr problem" << endl;
                sendNackToUE();
            }
        }
        else
        {

            EV << "ExtMECWarningAlertApp::handleSelfMessage - service IP address is  unspecified (maybe response from the service registry is arriving)" << endl;
            sendNackToUE();
        }
    }

    delete msg;
}


void ExtMECWarningAlertApp::handleProcessedMessage(cMessage *msg)
{
    if (!msg->isSelfMessage())
    {
        if(ueSocket.belongsToSocket(msg))
       {
           handleUeMessage(msg);
           delete msg;
           return;
       }
    }
    ExtMecAppBase::handleProcessedMessage(msg);
}

void ExtMECWarningAlertApp::handleHttpMessage(int connId)
{
    // we should consider here, that we may have socket which are not related
    // to a communication with a MEC service
    // for example when we need to exchange the state with another app during migration
    if (connId == mp1Socket_->getSocketId())
    {
        handleMp1Message(connId);
        return;
    }
    int index = findServiceFromSocket(connId);
    if(index != -1)
    {
        EV << "ExtMECWarningAlertApp::handling service message" << endl;
        handleServiceMessage(index);
    }
    else
    {
        EV_INFO << "ExtMECWarningAlertApp::message not recognised as service -- This case has not been implemented yet!" << endl;
    }

}

void ExtMECWarningAlertApp::handleMp1Message(int connId)
{
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) mp1Socket_->getUserData();
    mp1HttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();

    EV << "ExtMECWarningAlertApp::handleMp1Message - payload: " << mp1HttpMessage->getBody() << endl;
    try
    {
        nlohmann::json jsonBody = nlohmann::json::parse(mp1HttpMessage->getBody()); // get the JSON structure
        if(!jsonBody.empty())
        {
            jsonBody = jsonBody[0];
            std::string serName = jsonBody["serName"];
            if(serName.compare("LocationService") == 0)
            {
                EV << "ExtMECWarningAlertApp::Location service info arrived from service registry" << endl;
                if(jsonBody.contains("transportInfo"))
                {
                    nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                    EV << "address: " << endPoint["host"] << " port: " <<  endPoint["port"] << endl;
                    std::string address = endPoint["host"];
                    int port = endPoint["port"];
                    // Adding service
                    servicesData_[0]->host.addr = inet::L3AddressResolver().resolve(address.c_str());
                    servicesData_[0]->host.port = port;

                    EV << "ExtMECWarningAlertApp::registered addr " << servicesData_[0]->host.addr << ", port " <<  servicesData_[0]->host.port << endl;
                    inet::TcpSocket *serviceSocket = addNewSocket();
                    servicesData_[0]->sockid = serviceSocket->getSocketId();
                }
            }
            else if(serName.compare("ApplicationMobilityService") == 0)
            {
                EV << "ExtMECWarningAlertApp::Location service info arrived from service registry" << endl;
                if(jsonBody.contains("transportInfo"))
                {
                    nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                    EV << "address: " << endPoint["host"] << " port: " <<  endPoint["port"] << endl;
                    std::string address = endPoint["host"];
                    int port = endPoint["port"];
                    servicesData_[1]->host.addr = inet::L3AddressResolver().resolve(address.c_str());
                    servicesData_[1]->host.port = port;
                    inet::TcpSocket *serviceSocket = addNewSocket();
                    servicesData_[1]->sockid = serviceSocket->getSocketId();
                }
            }
            else
            {
                EV << "MECWarningAlertApp::handleMp1Message - LocationService not found"<< endl;
//                serviceAddress = L3Address();
            }
        }

    }
    catch(nlohmann::detail::parse_error e)
    {
        EV <<  e.what() << std::endl;
        // body is not correctly formatted in JSON, manage it
        return;
    }
}


void ExtMECWarningAlertApp::handleServiceMessage(int index)
{
    inet::TcpSocket *serviceSocket_ = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(servicesData_[index]->sockid));


    if(std::strcmp(servicesData_[index]->name.c_str(), "LocationService") == 0)
    {
        handleLSMessage(serviceSocket_, index);
    }
    else
    {
        // case Application mobility service
        handleAMSMessage(serviceSocket_, index);
    }

}

void ExtMECWarningAlertApp::handleLSMessage(inet::TcpSocket *serviceSocket, int index)
{
    // This method correspond to the method handleServiceMessage of MECWarningAlertApp.cc
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) serviceSocket->getUserData();
    HttpBaseMessage* serviceHttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();
    EV << "ExtMECWarningAlertApp::handling location service message " <<  serviceHttpMessage->getBody() << endl;
    if(serviceHttpMessage->getType() == REQUEST)
    {
        Http::send204Response(serviceSocket); // send back 204 no content
        nlohmann::json jsonBody;
        EV << "ExtMECWarningAlertApp::handleTcpMsg - REQUEST " << serviceHttpMessage->getBody()<< endl;
        try
        {
           jsonBody = nlohmann::json::parse(serviceHttpMessage->getBody()); // get the JSON structure
        }
        catch(nlohmann::detail::parse_error e)
        {
           std::cout  <<  e.what() << std::endl;
           // body is not correctly formatted in JSON, manage it
           return;
        }
        if(jsonBody.contains("subscriptionNotification"))
        {
            if(jsonBody["subscriptionNotification"].contains("enteringLeavingCriteria"))
            {
                nlohmann::json criteria = jsonBody["subscriptionNotification"]["enteringLeavingCriteria"] ;
                auto alert = inet::makeShared<WarningAlertPacket>();
                alert->setType(WARNING_ALERT);

                if(criteria == "Entering")
                {
                    EV << "ExtMECWarningAlertApp::handleTcpMsg - Ue is Entered in the danger zone "<< endl;
                    alert->setDanger(true);

                    if(par("logger").boolValue())
                    {
                        std::ofstream myfile;
                        myfile.open ("example.txt", std::ios::app);
                        if(myfile.is_open())
                        {
                            myfile <<"["<< NOW << "] ExtMECWarningAlertApp - Received circleNotificationSubscription notification from Location Service. UE's entered the red zone! \n";
                            myfile.close();
                        }
                    }

                    // send subscription for leaving..
                    modifySubscription(serviceSocket);

                }
                else if (criteria == "Leaving")
                {
                    EV << "ExtMECWarningAlertApp::handleTcpMsg - Ue left from the danger zone "<< endl;
                    alert->setDanger(false);
                    if(par("logger").boolValue())
                    {
                        std::ofstream myfile;
                        myfile.open ("example.txt", std::ios::app);
                        if(myfile.is_open())
                        {
                            myfile <<"["<< NOW << "] MEWarningAlertApp - Received circleNotificationSubscription notification from Location Service. UE's exited the red zone! \n";
                            myfile.close();
                        }
                    }
                    sendDeleteSubscription(serviceSocket);
                }
                alert->setPositionX(jsonBody["subscriptionNotification"]["terminalLocationList"]["currentLocation"]["x"]);
                alert->setPositionY(jsonBody["subscriptionNotification"]["terminalLocationList"]["currentLocation"]["y"]);
                alert->setChunkLength(inet::B(20));
                alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

                inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
                packet->insertAtBack(alert);
                ueSocket.sendTo(packet, ueAppAddress, ueAppPort);
            }
        }
    }
    else if(serviceHttpMessage->getType() == RESPONSE)
    {
        EV << "ExtMECWarningAlertApp::handling response message" << endl;
        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(serviceHttpMessage);
        if(rspMsg->getCode() == 204) // in response to a DELETE
        {
            EV << "ExtMECWarningAlertApp::handleTcpMsg - response 204, removing circle" << rspMsg->getBody()<< endl;
            serviceSocket->close();
            getSimulation()->getSystemModule()->getCanvas()->removeFigure(circle);

        }
        else if(rspMsg->getCode() == 201) // in response to a POST
        {
            nlohmann::json jsonBody;
            EV << "ExtMECWarningAlertApp::handleTcpMsg - response 201 " << rspMsg->getBody()<< endl;
            try
            {
               jsonBody = nlohmann::json::parse(rspMsg->getBody()); // get the JSON structure
            }
            catch(nlohmann::detail::parse_error e)
            {
               EV <<  e.what() << endl;
               // body is not correctly formatted in JSON, manage it
               return;
            }
            std::string resourceUri = jsonBody["circleNotificationSubscription"]["resourceURL"];
            std::size_t lastPart = resourceUri.find_last_of("/");
            if(lastPart == std::string::npos)
            {
                EV << "1" << endl;
                return;
            }
            // find_last_of does not take in to account if the uri has a last /
            // in this case subscriptionType would be empty and the baseUri == uri
            // by the way the next if statement solve this problem
            std::string baseUri = resourceUri.substr(0,lastPart);
            //save the id
            subId = resourceUri.substr(lastPart+1);
            EV << "subId: " << subId << endl;

            circle = new cOvalFigure("circle");
            circle->setBounds(cFigure::Rectangle(centerPositionX - radius, centerPositionY - radius,radius*2,radius*2));
            circle->setLineWidth(2);
            circle->setLineColor(cFigure::RED);
            getSimulation()->getSystemModule()->getCanvas()->addFigure(circle);

        }
    }

}


void ExtMECWarningAlertApp::handleAMSMessage(inet::TcpSocket *serviceSocket, int index)
{
    EV << "To be defined" << endl;
}

void ExtMECWarningAlertApp::handleUeMessage(omnetpp::cMessage *msg)
{
    // determine its source address/port
    auto pk = check_and_cast<inet::Packet *>(msg);
    ueAppAddress = pk->getTag<inet::L3AddressInd>()->getSrcAddress();
    ueAppPort = pk->getTag<inet::L4PortInd>()->getSrcPort();

    auto mecPk = pk->peekAtFront<WarningAppPacket>();

    if(strcmp(mecPk->getType(), START_WARNING) == 0)
    {
        /*
         * Read center and radius from message
         */
        EV << "MECWarningAlertApp::handleUeMessage - WarningStartPacket arrived" << endl;
        auto warnPk = dynamicPtrCast<const WarningStartPacket>(mecPk);
        if(warnPk == nullptr)
            throw cRuntimeError("MECWarningAlertApp::handleUeMessage - WarningStartPacket is null");

        centerPositionX = warnPk->getCenterPositionX();
        centerPositionY = warnPk->getCenterPositionY();
        radius = warnPk->getRadius();

        if(par("logger").boolValue())
        {
            std::ofstream myfile;
            myfile.open ("example.txt", std::ios::app);
            if(myfile.is_open())
            {
                myfile <<"["<< NOW << "] MEWarningAlertApp - Received message from UE, connecting to the Location Service\n";
                myfile.close();
            }
        }

        cMessage *m = new cMessage("connectService");
        scheduleAt(simTime()+0.005, m);
    }
    else if (strcmp(mecPk->getType(), STOP_WARNING) == 0)
    {
        // Delete subscription from location service
        // Location service index = 0
        inet::TcpSocket *serviceSocket = check_and_cast<inet::TcpSocket *>(sockets_.getSocketById(servicesData_[0]->sockid));
        sendDeleteSubscription(serviceSocket);
    }

    else
    {
        throw cRuntimeError("MECWarningAlertApp::handleUeMessage - packet not recognized");
    }
}

void ExtMECWarningAlertApp::modifySubscription(inet::TcpSocket *serviceSocket)
{
    std::string body = "{  \"circleNotificationSubscription\": {"
                       "\"callbackReference\" : {"
                        "\"callbackData\":\"1234\","
                        "\"notifyURL\":\"example.com/notification/1234\"},"
                       "\"checkImmediate\": \"false\","
                        "\"address\": \"" + ueAppAddress.str()+ "\","
                        "\"clientCorrelator\": \"null\","
                        "\"enteringLeavingCriteria\": \"Leaving\","
                        "\"frequency\": 5,"
                        "\"radius\": " + std::to_string(radius) + ","
                        "\"trackingAccuracy\": 10,"
                        "\"latitude\": " + std::to_string(centerPositionX) + ","           // as x
                        "\"longitude\": " + std::to_string(centerPositionY) + ""        // as y
                        "}"
                        "}\r\n";
    std::string uri = "/example/location/v2/subscriptions/area/circle/" + subId;
    std::string host = serviceSocket->getRemoteAddress().str()+":"+std::to_string(serviceSocket->getRemotePort());
    Http::sendPutRequest(serviceSocket, body.c_str(), host.c_str(), uri.c_str());
}

void ExtMECWarningAlertApp::sendSubscription(inet::TcpSocket *socket)
{
    std::string body = "{  \"circleNotificationSubscription\": {"
                           "\"callbackReference\" : {"
                            "\"callbackData\":\"1234\","
                            "\"notifyURL\":\"example.com/notification/1234\"},"
                           "\"checkImmediate\": \"false\","
                            "\"address\": \"" + ueAppAddress.str()+ "\","
                            "\"clientCorrelator\": \"null\","
                            "\"enteringLeavingCriteria\": \"Entering\","
                            "\"frequency\": 5,"
                            "\"radius\": " + std::to_string(radius) + ","
                            "\"trackingAccuracy\": 10,"
                            "\"latitude\": " + std::to_string(centerPositionX) + ","           // as x
                            "\"longitude\": " + std::to_string(centerPositionY) + ""        // as y
                            "}"
                            "}\r\n";
    std::string uri = "/example/location/v2/subscriptions/area/circle";
    std::string host = socket->getRemoteAddress().str()+":"+std::to_string(socket->getRemotePort());

    if(par("logger").boolValue())
    {
        std::ofstream myfile;
        myfile.open ("example.txt", std::ios::app);
        if(myfile.is_open())
        {
            myfile <<"["<< NOW << "] ExtMEWarningAlertApp - Sent POST circleNotificationSubscription the Location Service \n";
            myfile.close();
        }
    }

    Http::sendPostRequest(socket, body.c_str(), host.c_str(), uri.c_str());
}

void ExtMECWarningAlertApp::sendDeleteSubscription(inet::TcpSocket *serviceSocket)
{
    std::string uri = "/example/location/v2/subscriptions/area/circle/" + subId;
    std::string host = serviceSocket->getRemoteAddress().str()+":"+std::to_string(serviceSocket->getRemotePort());
    Http::sendDeleteRequest(serviceSocket, host.c_str(), uri.c_str());
}

void ExtMECWarningAlertApp::established(int connId)
{
    if(connId == mp1Socket_->getSocketId())
    {
        EV << "ExtMECWarningAlertApp::established connection with Service Registry" << endl;
        bool res = ExtMecAppBase::getInfoFromServiceRegistry();
        if(!res)
        {
            EV << "ExtMECWarningAlertApp::service not found" << endl;
        }
        return;
    }
    int index = findServiceFromSocket(connId);
    inet::TcpSocket *serviceSocket_ = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(connId));
    if(index != -1)
    {
        if(std::strcmp(servicesData_[index]->name.c_str(), "LocationService") == 0)
        {
            // handling connection established with Location Service
            EV << "ExtMECWarningAlertApp::established - serviceSocket"<< endl;
            // the connectService message is scheduled after a start mec app from the UE app, so I can
            // response to her here, once the socket is established
            auto ack = inet::makeShared<WarningAppPacket>();
            ack->setType(START_ACK);
            ack->setChunkLength(inet::B(2));
            inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
            packet->insertAtBack(ack);
            ueSocket.sendTo(packet, ueAppAddress, ueAppPort);
            sendSubscription(serviceSocket_);
            return;
        }
        else
        {
            //handling connection established with Application Mobility Service
            EV << "ExtMECWarningAlertApp::not implemented yet" << endl;

        }
    }
    else
    {
        // connection established between two mecapps for
        // status exchange
        // TODO
    }
}


void ExtMECWarningAlertApp::finish(){
    ExtMecAppBase::finish();
    EV << "ExtMECWarningAlertApp::finish()" << endl;

    if(gate("socketOut")->isConnected()){

    }
}

void ExtMECWarningAlertApp::sendNackToUE()
{
    auto nack = inet::makeShared<WarningAppPacket>();
    // the connectService message is scheduled after a start mec app from the UE app, so I can
    // response to her here
    nack->setType(START_NACK);
    nack->setChunkLength(inet::B(2));
    inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
    packet->insertAtBack(nack);
    ueSocket.sendTo(packet, ueAppAddress, ueAppPort);
}


