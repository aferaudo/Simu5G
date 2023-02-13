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

// AMS Messages
#include "nodes/mec/MECPlatform/MECServices/ApplicationMobilityService/resources/MobilityProcedureNotification.h"
#include "apps/mec/WarningAlert/packets/MecWarningAppSyncMessage_m.h"


Define_Module(ExtMECWarningAlertApp);

using namespace omnetpp;

simsignal_t ExtMECWarningAlertApp::migrationTime_ = registerSignal("migrationTime");


ExtMECWarningAlertApp::ExtMECWarningAlertApp()
{
}

ExtMECWarningAlertApp::~ExtMECWarningAlertApp()
{
    if(circle != nullptr)
    {
        if(getSimulation()->getSystemModule()->getCanvas()->findFigure(circle) != -1)
            getSimulation()->getSystemModule()->getCanvas()->removeFigure(circle);
        delete circle;
    }
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


    // migration port
    localPort = par("localPort");

    // ue port
    localUePort = par("localUePort");
    ueSocket.bind(localUePort);

    EV << "ExtMECWarningAlertApp::initializing" << endl;
    mp1Socket_ = addNewSocket();

    status = "Entering";

    MecServiceInfo *m1 = new MecServiceInfo();
    m1->name = "LocationService";
    MecServiceInfo *m2 = new MecServiceInfo();
    m2->name = "ApplicationMobilityService";

    // std MEC services needed
    servicesData_.push_back(m1);
    servicesData_.push_back(m2);

    // AMS
    webHook = "/amsWebHook_" + std::to_string(getId());
    subscribed = false;
    ueRegistered = false;

    isMigrating = par("isMigrating").boolValue();

    // waiting for state
    amsStateSocket_ = nullptr;
    if(isMigrating)
    {
        amsStateSocket_ = addNewSocket();
        amsStateSocket_->bind(localAddress, localPort);
        amsStateSocket_->listen();
    }


    // connect with the service registry
    cMessage *msg = new cMessage("connectMp1");
    scheduleAt(simTime() + 0, msg);
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
       // Connecting to each required service
       // Location Service - array index 0
       // Application Mobility Service - array index 1
       for(auto service : servicesData_)
       {
           EV << "ExtMECWarningAlertApp::handleMessage- " << msg->getName() << endl;
           if(!service->host.addr.isUnspecified())
           {
               inet::TcpSocket *serviceSocket  = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(service->sockid));
               if(serviceSocket != nullptr && serviceSocket->getState() != inet::TcpSocket::CONNECTED
                       && serviceSocket->getState() != inet::TcpSocket::CONNECTING)
               {
                   EV << "ExtMECWarningAlertApp::connecting to the service: " << service->name << endl;
                   connect(serviceSocket, service->host.addr, service->host.port);

               }
               else
               {
                   EV << "ExtMECWarningAlertApp::handleSelfMessage - service socket is already connected or nullptr problem" << endl;
                   sendNackToUE();
               }
           }
           else
           {

               std::cout << "ExtMECWarningAlertApp::handleSelfMessage - service IP address is  unspecified (maybe response from the service registry is arriving)" << endl;
               if(!connectService_->isScheduled())
                   scheduleAt(simTime()+0.005, connectService_);

           }
       }
       return;
    }
    else if(strcmp(msg->getName(), "connectAMS") == 0)
    {
       // Special case for new app started
       inet::TcpSocket *serviceSocket  = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(servicesData_[AMS]->sockid));
       EV << "ExtMECWarningAlertApp::connecting with AMS during migration" << endl;
       if(serviceSocket != nullptr)
       {
           connect(serviceSocket, servicesData_[AMS]->host.addr, servicesData_[AMS]->host.port);
       }
    }
    else if(strcmp(msg->getName(), "migrateState") == 0)
    {
       EV << "ExtMECWarningAlertApp::connecting with new allocated app for state transfer, "
               << "address: " << migrationAddress.str() << ", port: " << migrationPort << endl;

       // Connecting to the new mec app
       connect(amsStateSocket_, migrationAddress, migrationPort);
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

void ExtMECWarningAlertApp::handleReceivedMessage(int sockId, inet::Packet *msg)
{
    auto data = msg->peekData<MecWarningAppSyncMessage>();
    // managing state messages -- no http

    if(!data->isAck())
    {
        EV << "ExtMECWarningAlertApp::handleStateMessage - received message " << endl;

        EV << "ExtMECWarningAlertApp::setting new state: " << endl;
        EV << "ExtMECWarningAlertApp::position x: " << data->getPositionX() << endl;
        EV << "ExtMECWarningAlertApp::position y: " << data->getPositionY() << endl;
        EV << "ExtMECWarningAlertApp::radius: " << data->getRadius() << endl;
        EV << "ExtMECWarningAlertApp::ue addr: " << data->getUeAddress() << endl;
        EV << "ExtMECWarningAlertApp::ue port: " << data->getUePort() << endl;
        EV << "ExtMECWarningAlertApp::contextId: " << data->getContextId() << endl;
        EV << "ExtMECWarningAlertApp::state: " << data->getState() << endl;

        centerPositionX = data->getPositionX();
        centerPositionY = data->getPositionY();
        radius = data->getRadius();

        // Remote UE information
        ueAppAddress = data->getUeAddress();
        ueAppPort = data->getUePort();

        // Local UE information
//        localUePort = data->getLocalUePort();
        std::cout << "Port where " << getSimulation()->getModule(getId())->getName() << " is running  "<< localUePort << endl;
//        ueSocket.bind(localUePort);


        status = std::string(data->getState());
        EV << "ExtMECWarningAlertApp::handleStateMessage - new state injected!" << endl;

        if(subscribed) // means registred and subscribed to the AMS
        {
            // registering the mecapp with the status received
    //        isMigrating = false;
            inet::TcpSocket *serviceSocketLS  = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(servicesData_[LS]->sockid));
            connect(serviceSocketLS, servicesData_[LS]->host.addr, servicesData_[LS]->host.port);
            inet::TcpSocket *serviceSocketAMS = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(servicesData_[AMS]->sockid));
            inet::TcpSocket *stateSocket = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(sockId));


            updateRegistrationAMS(serviceSocketAMS, APP_MOBILITY_NOT_ALLOWED, NOT_TRANSFERRED);
            updateSubscriptionAMS(serviceSocketAMS);
            auto ack = inet::makeShared<MecWarningAppSyncMessage>();
            inet::Packet *pkt = new inet::Packet("MecWarningAppSyncMessage");
            ack->setIsAck(true);
            ack->setResult(true);

            ack->setUeAddress(ueAppAddress);
            ack->setUePort(ueAppPort);
            ack->setChunkLength(inet::B(16));

            pkt->insertAtBack(ack);

            // sending ack back
//            std::cout << "sending ack " << simTime() <<endl;
//            std::cout << "socket id : " << stateSocket->getSocketId() << endl;
            stateSocket->setUserData(nullptr);
            stateSocket->send(pkt);
        }
        else
        {
            //connect to the services
            throw cRuntimeError("Attempting to update information without being subscribed to the AMS");

        }
    }
    else
    {
       std::cout << "RECEIVED ACK" << endl;
        // Update AMS registration to trigger ip addresses updates
       inet::TcpSocket *serviceSocketAMS = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(servicesData_[AMS]->sockid));
       updateRegistrationAMS(serviceSocketAMS, APP_MOBILITY_NOT_ALLOWED, USER_CONTEXT_TRANSFER_COMPLETED);
    }

}

void ExtMECWarningAlertApp::handleMp1Message(int connId)
{
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) mp1Socket_->getUserData();
    mp1HttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();
    responseCounter_ --;
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
                EV << "ExtMECWarningAlertApp::Location Service info arrived from service registry" << endl;
                if(jsonBody.contains("transportInfo"))
                {
                    nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                    EV << "address: " << endPoint["host"] << " port: " <<  endPoint["port"] << endl;
                    std::string address = endPoint["host"];
                    int port = endPoint["port"];
                    // Adding service
                    servicesData_[LS]->host.addr = inet::L3AddressResolver().resolve(address.c_str());
                    servicesData_[LS]->host.port = port;

                    EV << "ExtMECWarningAlertApp::registered addr " << servicesData_[LS]->host.addr << ", port " <<  servicesData_[LS]->host.port << endl;
                    inet::TcpSocket *serviceSocket = addNewSocket();
                    servicesData_[LS]->sockid = serviceSocket->getSocketId();
                }
            }
            else if(serName.compare("ApplicationMobilityService") == 0)
            {
                EV << "ExtMECWarningAlertApp::Application Mobility Service info arrived from service registry" << endl;
                if(jsonBody.contains("transportInfo"))
                {
                    nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                    EV << "address: " << endPoint["host"] << " port: " <<  endPoint["port"] << endl;
                    std::string address = endPoint["host"];
                    int port = endPoint["port"];
                    servicesData_[AMS]->host.addr = inet::L3AddressResolver().resolve(address.c_str());
                    servicesData_[AMS]->host.port = port;
                    inet::TcpSocket *serviceSocket = addNewSocket();
                    servicesData_[AMS]->sockid = serviceSocket->getSocketId();

                    if(isMigrating)
                    {
                        // connect to service
                        // The new App should be able to communicate
                        // with the AMS as soon as it starts
                        cMessage *m = new cMessage("connectAMS");
                        scheduleAt(simTime(), m);
                    }

                }
            }
            else
            {
                EV << "ExtMECWarningAlertApp::handleMp1Message - Service" << serName << " not found"<< endl;
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
    std::cout << "Received Service Message at " << simTime()<< endl;

    if(std::strcmp(servicesData_[index]->name.c_str(), "LocationService") == 0)
    {
        handleLSMessage(serviceSocket_);
    }
    else
    {
        // case Application mobility service
        handleAMSMessage(serviceSocket_);
    }

}

void ExtMECWarningAlertApp::handleLSMessage(inet::TcpSocket *serviceSocket)
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

                    status = "Leaving";

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
                    modifySubscriptionLS(serviceSocket, status);
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
                    sendDeleteSubscriptionLS(serviceSocket);
                }
                alert->setUeOmnetID(getId()); // TODO remove this line -> it is used only for statistics
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
        responseCounter_ --;
        EV << "ExtMECWarningAlertApp::handling response message" << endl;
        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(serviceHttpMessage);
        if(rspMsg->getCode() == 204) // in response to a DELETE
        {
            EV << "ExtMECWarningAlertApp::handleTcpMsg - response 204, removing circle" << rspMsg->getBody()<< endl;
            serviceSocket->close();
            getSimulation()->getSystemModule()->getCanvas()->removeFigure(circle);

            if(!isMigrating)
            {
                emit(migrationTime_, simTime());
            }
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


void ExtMECWarningAlertApp::handleAMSMessage(inet::TcpSocket *serviceSocket)
{
    EV << "ExtMECWarningAlertApp::handleAMSMessage" << endl;
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) serviceSocket->getUserData();
    HttpBaseMessage* serviceHttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();
    try
    {
        if(serviceHttpMessage->getType() == REQUEST)
        {
            // Notification
            EV << "ExtMECWarningAlertApp::handleAmsMessage - Received request - payload: " << " " << serviceHttpMessage->getBody() << endl;
            HttpRequestMessage* amsRequest = check_and_cast<HttpRequestMessage*>(serviceHttpMessage);
            nlohmann::json jsonBody = nlohmann::json::parse(amsRequest->getBody());


            if(std::string(amsRequest->getUri()).compare(webHook) == 0 && !jsonBody.empty()){
                MobilityProcedureNotification *notification = new MobilityProcedureNotification();
                notification->fromJson(jsonBody);
                std::string type = notification->getMobilityStatusString();
                if(type.empty()){
                    throw cRuntimeError("mobility status not specified in the notification");
                }
                EV << "ExtMECWarningAlertApp::handleAmsMessage - Analyzing notification - payload: " << " " << serviceHttpMessage->getBody() << endl;
                if(type.compare("INTERHOST_MOVEOUT_TRIGGERED") == 0 && jsonBody.contains("targetAppInfo"))
                {
                   TargetAppInfo* targetAppInfo = new TargetAppInfo();
                   targetAppInfo->fromJson(jsonBody["targetAppInfo"]);

                   // Distinguishing between old app and new app
                   if(targetAppInfo->getCommInterface().size() != 0 &&
                           targetAppInfo->getCommInterface()[0].addr != localAddress)
                   {
                       EV << "ExtMECWarningAlertApp::handleAmsMessage - Analyzing notification - TargetAppInfo found: " << " " << serviceHttpMessage->getBody() << endl;
                       migrationAddress = targetAppInfo->getCommInterface()[0].addr; // 0 migration port
                       migrationPort = targetAppInfo->getCommInterface()[0].port; // 1 ue port
                       // Adding socket to socket map
                       amsStateSocket_ = addNewSocketNoHttp();

                       // triggaring status change procedure
                       cMessage *msg = new cMessage("migrateState");
                       scheduleAt(simTime()+0.005, msg);
                   }
                   else
                   {
                       EV << "ExtMECWarningAlertApp::not interested in this notification" << endl;
                   }
                }
                else if(type.compare("INTERHOST_MOVEOUT_COMPLETED") == 0 && jsonBody.contains("targetAppInfo")){
                    // TODO deltion procedure?
//                    cMessage *m = new cMessage("deleteRegistration");
//                    scheduleAt(simTime()+0.005, m);
//
//                    cMessage *d = new cMessage("deleteModule");
//                    scheduleAt(simTime()+0.7, d);

                    EV << "ExtMECWarningAlertApp::handleAmsMessage - Deletion has been scheduled" << endl;
                }
            }
        }
        else if(serviceHttpMessage->getType() == RESPONSE)
        {
            responseCounter_ --;
            EV << "ExtMECWarningAlertApp::handleAmsMessage - Received a response - payload: " << " " << serviceHttpMessage->getBody() << endl;
            HttpResponseMessage* amsResponse = check_and_cast<HttpResponseMessage*>(serviceHttpMessage);

            nlohmann::json jsonBody = nlohmann::json::parse(amsResponse->getBody());
            if(!jsonBody.empty())
            {
                if(jsonBody.contains("appMobilityServiceId"))
                {
                    // Registration
                    amsRegistrationId = jsonBody["appMobilityServiceId"];
                    EV << "ExtMECWarningAlertApp::handleAmsMessage - registration ID: " << amsRegistrationId << endl;

                    // sending subscription after registration
                    sendSubscriptionAMS(serviceSocket);
                }
                else if(jsonBody.contains("callbackReference")){
                    // Subscription
                    std::stringstream stream;
                    stream << "sub" << jsonBody["subscriptionId"];
                    amsSubscriptionId = stream.str();
                    EV << "ExtMECWarningAlertApp::handleAmsMessage - subscription ID " << amsSubscriptionId << endl;



                    if(!amsSubscriptionId.empty())
                    {
                        subscribed = true;
                    }

                }
            }
            else
            {
                EV_INFO << "ExtMECWarningAlertApp::json body ams empty!!!!" << endl;
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

        if(!connectService_->isScheduled())
        {
            scheduleAt(simTime()+0.005, connectService_);
        }
    }
    else if (strcmp(mecPk->getType(), STOP_WARNING) == 0)
    {
        // Delete subscription from location service
        // Location service index = 0
        inet::TcpSocket *serviceSocket = check_and_cast<inet::TcpSocket *>(sockets_.getSocketById(servicesData_[LS]->sockid));
        sendDeleteSubscriptionLS(serviceSocket);

    }

    else
    {
        throw cRuntimeError("MECWarningAlertApp::handleUeMessage - packet not recognized");
    }
}

void ExtMECWarningAlertApp::modifySubscriptionLS(inet::TcpSocket *serviceSocket, std::string criteria)
{
    std::string body = "{  \"circleNotificationSubscription\": {"
                           "\"callbackReference\" : {"
                            "\"callbackData\":\"1234\","
                            "\"notifyURL\":\"example.com/notification/1234\"},"
                           "\"checkImmediate\": \"false\","
                            "\"address\": \"" + ueAppAddress.str()+ "\","
                            "\"clientCorrelator\": \"null\","
                            "\"enteringLeavingCriteria\": \"" + criteria + "\","
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
    responseCounter_ ++;
}

void ExtMECWarningAlertApp::sendSubscriptionLS(inet::TcpSocket *socket, std::string criteria)
{
    std::string body = "{  \"circleNotificationSubscription\": {"
                           "\"callbackReference\" : {"
                            "\"callbackData\":\"1234\","
                            "\"notifyURL\":\"example.com/notification/1234\"},"
                           "\"checkImmediate\": \"false\","
                            "\"address\": \"" + ueAppAddress.str()+ "\","
                            "\"clientCorrelator\": \"null\","
                            "\"enteringLeavingCriteria\": \""+ criteria +"\","
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
    responseCounter_ ++;
}

void ExtMECWarningAlertApp::sendDeleteSubscriptionLS(inet::TcpSocket *serviceSocket)
{
    std::string uri = "/example/location/v2/subscriptions/area/circle/" + subId;
    std::string host = serviceSocket->getRemoteAddress().str()+":"+std::to_string(serviceSocket->getRemotePort());
    Http::sendDeleteRequest(serviceSocket, host.c_str(), uri.c_str());
    responseCounter_ ++;
}


void ExtMECWarningAlertApp::sendRegistrationAMS(inet::TcpSocket *socket, ContextTransferState transferState)
{

    EV << "ExtMECWarningAlertApp::registration to AMS" << endl;
    nlohmann::ordered_json registrationBody;
    registrationBody = nlohmann::ordered_json();
    registrationBody["serviceConsumerId"]["appInstanceId"] = std::string(getName());
    registrationBody["serviceConsumerId"]["mepId"] = "1234"; //TODO make it a parameter
    registrationBody["deviceInformation"] = nlohmann::json::array();
    nlohmann::ordered_json deviceInformation;
    nlohmann::ordered_json associateId;

    associateId["type"] = "UE_IPv4_ADDRESS";
    associateId["value"] = ueAppAddress.str();
    deviceInformation["associateId"] = associateId;
    deviceInformation["appMobilityServiceLevel"] = "APP_MOBILITY_NOT_ALLOWED";
    // TODO fix for migrated case
    deviceInformation["contextTransferState"] = ContextTransferStateStrings[transferState];

    registrationBody["deviceInformation"].push_back(deviceInformation);

    std::cout << "ExtMECWarningAlertApp::registration to AMS with body" << registrationBody.dump().c_str() << endl;
    std::string host = socket->getRemoteAddress().str()+":"+std::to_string(socket->getRemotePort());
    const char *uri = "/example/amsi/v1/app_mobility_services/";
    Http::sendPostRequest(socket, registrationBody.dump().c_str(), host.c_str(), uri);
    responseCounter_ ++;
}

void ExtMECWarningAlertApp::sendDeleteRegistrationAMS(inet::TcpSocket *socket)
{
    EV << "ExtMECWarningAlertApp::delete registration from AMS" << endl;
    std::string host = socket->getRemoteAddress().str()+":"+std::to_string(socket->getRemotePort());
    std::string uristring = "/example/amsi/v1/app_mobility_services/" + amsRegistrationId;
    const char *uri = uristring.c_str();
    Http::sendDeleteRequest(socket, host.c_str(), uri);
    responseCounter_ ++;
}

void ExtMECWarningAlertApp::sendSubscriptionAMS(inet::TcpSocket *socket)
{

    EV << "ExtMECWarningAlertApp::send subscription to AMS" << endl;
    nlohmann::ordered_json subscriptionBody_ = getSubsciptionAMSBody();


    std::string host = socket->getRemoteAddress().str()+":"+std::to_string(socket->getRemotePort());
    std::string uristring = "/example/amsi/v1/subscriptions/";
    Http::sendPostRequest(socket, subscriptionBody_.dump().c_str(), host.c_str(), uristring.c_str());
    responseCounter_ ++;

}

void ExtMECWarningAlertApp::sendDeleteSubscriptionAMS(inet::TcpSocket *socket)
{

    EV << "ExtMECWarningAlertApp::send subscription to AMS" << endl;
    std::string host = socket->getRemoteAddress().str()+":"+std::to_string(socket->getRemotePort());
    std::string uristring = "/example/amsi/v1/subscriptions/" + amsSubscriptionId;
    const char *uri = uristring.c_str();
    Http::sendDeleteRequest(socket, host.c_str(), uri);
    responseCounter_ ++;
}

void ExtMECWarningAlertApp::updateRegistrationAMS(inet::TcpSocket *socket, AppMobilityServiceLevel level,
        ContextTransferState transferState)
{
    EV << "ExtMECWarningAlertApp::Updating registration" << endl;
    nlohmann::ordered_json registrationBody;
    registrationBody = nlohmann::ordered_json();
    registrationBody["serviceConsumerId"]["appInstanceId"] = std::string(getName());
    registrationBody["serviceConsumerId"]["mepId"] = "1234"; //TODO make it a parameter
    registrationBody["deviceInformation"] = nlohmann::json::array();
    nlohmann::ordered_json deviceInformation;
    nlohmann::ordered_json associateId;

    associateId["type"] = "UE_IPv4_ADDRESS";
    associateId["value"] = ueAppAddress.str();
    deviceInformation["associateId"] = associateId;
    deviceInformation["appMobilityServiceLevel"] = AppMobilityServiceLevelStrings[level];
    deviceInformation["contextTransferState"] = ContextTransferStateStrings[transferState];

    registrationBody["deviceInformation"].push_back(deviceInformation);

    std::string host = socket->getRemoteAddress().str()+":"+std::to_string(socket->getRemotePort());
    std::string uristring = "/example/amsi/v1/app_mobility_services/" + amsRegistrationId;
    const char *uri = uristring.c_str();
    Http::sendPutRequest(socket, registrationBody.dump().c_str(), host.c_str(), uri);
    responseCounter_ ++;
}

void ExtMECWarningAlertApp::updateSubscriptionAMS(inet::TcpSocket *socket)
{
    EV << "ExtMECWarningAlertApp::send subscription to AMS" << endl;
    nlohmann::ordered_json subscriptionBody_ = getSubsciptionAMSBody();

    std::string host = socket->getRemoteAddress().str()+":"+std::to_string(socket->getRemotePort());
    std::string uristring = "/example/amsi/v1/subscriptions/" + amsSubscriptionId;
    Http::sendPutRequest(socket, subscriptionBody_.dump().c_str(), host.c_str(), uristring.c_str());
    responseCounter_ ++;
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
    inet::TcpSocket *socket_ = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(connId));
    if(index != -1)
    {
        if(std::strcmp(servicesData_[index]->name.c_str(), "LocationService") == 0)
        {
            // handling connection established with Location Service
            EV << "ExtMECWarningAlertApp::established - LocationService socket"<< endl;
            // the connectService message is scheduled after a start mec app from the UE app, so I can
            // response to her here, once the socket is established
            auto ack = inet::makeShared<WarningAppPacket>();
            ack->setUeOmnetID(getId()); // TODO remove this: used for statistics
            ack->setType(START_ACK);
            ack->setChunkLength(inet::B(2));
            inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
            packet->insertAtBack(ack);
            ueSocket.sendTo(packet, ueAppAddress, ueAppPort);
            sendSubscriptionLS(socket_, status);
            return;
        }
        else
        {
            //handling connection established with Application Mobility Service
            EV << "ExtMECWarningAlertApp::established - ApplicationMobilityService socket" << endl;
            sendRegistrationAMS(socket_);

        }
    }
    else if(socket_->getSocketId() == amsStateSocket_->getSocketId())
    {
        // connection established between two mecapps for
        // status exchange
        EV << "ExtMECWarningAlertApp::socket state exchange established -- old mec app" << endl;
        sendState();

    }
    else if (socket_ != nullptr)
    {
        EV << "ExtMECWarningAlertApp::established a connection in listening mode -- migration" << endl;
    }
    else
    {
        throw cRuntimeError("ExtMecAppBase::socketEstablished - Socket %d not recognized", connId);
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

void ExtMECWarningAlertApp::sendState()
{

    std::string module_name = std::string(getName());
    inet::Packet *packet = new inet::Packet("SyncState");
    auto syncMessage = inet::makeShared<MecWarningAppSyncMessage>();
    syncMessage->setPositionX(centerPositionX);
    syncMessage->setPositionY(centerPositionY);
    syncMessage->setRadius(radius);
    syncMessage->setUeAddress(ueAppAddress); // remote us address
    syncMessage->setUePort(ueAppPort); // remote port
    syncMessage->setState(status.c_str());
    syncMessage->setContextId(std::stoi(module_name.substr(module_name.find('[') + 1, module_name.find(']') - module_name.find('[') - 1)));
    syncMessage->setChunkLength(inet::B(32));

    packet->insertAtBack(syncMessage);



    // Deleting state related subscriptions (subscription from location service)
    inet::TcpSocket *serviceSocketLS  = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(servicesData_[LS]->sockid));
    sendDeleteSubscriptionLS(serviceSocketLS);


    // sending MEC app's state
    amsStateSocket_->send(packet);


}

void ExtMECWarningAlertApp::handleTermination()
{
    inet::TcpSocket *amsSocket = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(servicesData_[AMS]->sockid));

    // deleting registration and subscription from AMS
    sendDeleteRegistrationAMS(amsSocket);
    sendDeleteSubscriptionAMS(amsSocket);

    // closing state socket (the only one managed by this class)
    std::cout << "handle termination inside the app 0.1" << endl;
    // There is the possibility that the app did not exchange a state
    // in such a scenario the socket is nullptr
    if(amsStateSocket_ != nullptr)
    {
        amsStateSocket_->close();
    }
    std::cout << "handle termination inside the app 0.2" << endl;
    // scheduling termination message (waiting for replies)
    scheduleAt(simTime()+0.01, terminationMessage_);

}

nlohmann::ordered_json ExtMECWarningAlertApp::getSubsciptionAMSBody()
{

    nlohmann::ordered_json subscriptionBody_;
    subscriptionBody_ = nlohmann::ordered_json();
    subscriptionBody_["_links"]["self"]["href"] = "";
    subscriptionBody_["callbackReference"] = localAddress.str() + ":" + std::to_string(par("localUePort").intValue()) + webHook;
    subscriptionBody_["requestTestNotification"] = false;
    subscriptionBody_["websockNotifConfig"]["websocketUri"] = "";
    subscriptionBody_["websockNotifConfig"]["requestWebsocketUri"] = false;
    subscriptionBody_["filterCriteria"]["appInstanceId"] = getName();
    subscriptionBody_["filterCriteria"]["associateId"] = nlohmann::json::array();

    nlohmann::ordered_json val_;
    val_["type"] = "UE_IPv4_ADDRESS";
    val_["value"] = ueAppAddress.str();
    subscriptionBody_["filterCriteria"]["associateId"].push_back(val_);
    subscriptionBody_["filterCriteria"]["mobilityStatus"] = nlohmann::json::array();
    subscriptionBody_["filterCriteria"]["mobilityStatus"].push_back("INTERHOST_MOVEOUT_TRIGGERED");
    subscriptionBody_["subscriptionType"] = "MobilityProcedureSubscription";
    return subscriptionBody_;
}

