//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "apps/mec/WarningAlert/MECWarningAlertApp.h"

#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"
#include "apps/mec/WarningAlert/packets/WarningAlertPacket_Types.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet_m.h"

#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/httpUtils/json.hpp"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"

#include <fstream>

Define_Module(MECWarningAlertApp);

using namespace inet;
using namespace omnetpp;

MECWarningAlertApp::MECWarningAlertApp(): MecAppBaseDyn()
{
    circle = nullptr; // circle danger zone

}
MECWarningAlertApp::~MECWarningAlertApp()
{
    if(circle != nullptr)
    {
        if(getSimulation()->getSystemModule()->getCanvas()->findFigure(circle) != -1)
            getSimulation()->getSystemModule()->getCanvas()->removeFigure(circle);
        delete circle;
    }
}


void MECWarningAlertApp::initialize(int stage)
{
    MecAppBaseDyn::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    //retrieving parameters
    size_ = par("packetSize");

    // set Udp Socket
    ueSocket.setOutputGate(gate("socketOut"));
    stateSocket.setOutputGate(gate("socketOut"));

    localUePort = par("localUePort");
    ueSocket.bind(localUePort);

    ueRegistered=false;

    localAddress = L3AddressResolver().resolve(getParentModule()->getFullPath().c_str());
    isMigrating = par("isMigrating").boolValue();
    isMigrated=false;

    if(isMigrating){
        EV << "MECWarningAlertApp::migrating state: LISTEN for context information on " << localAddress << ":" << localUePort << endl;
        serverSocket_.setOutputGate(gate("socketOut"));
        serverSocket_.setCallback(this);
        serverSocket_.bind(localAddress, localUePort);
        serverSocket_.listenOnce();
    }


    webHook="/amsWebHook_" + std::to_string(getId());

    //testing
    EV << "MECWarningAlertApp::initialize - Mec application "<< getClassName() << " with mecAppId["<< mecAppId << "] has started!" << endl;

    // connect with the service registry
    cMessage *msg = new cMessage("connectMp1");
    scheduleAt(simTime() + 0, msg);

}

void MECWarningAlertApp::handleMessage(cMessage *msg)
{
//        MecAppBase::handleMessage(msg);
    if (!msg->isSelfMessage())
    {
        if(ueSocket.belongsToSocket(msg))
        {
            handleUeMessage(msg);
            delete msg;
            return;
        }
    }
    MecAppBase::handleMessage(msg);

}

void MECWarningAlertApp::finish(){
    MecAppBase::finish();
    EV << "MECWarningAlertApp::finish()" << endl;

    if(gate("socketOut")->isConnected()){

    }
}

void MECWarningAlertApp::handleUeMessage(omnetpp::cMessage *msg)
{

    // determine its source address/port
    auto pk = check_and_cast<Packet *>(msg);
    ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
    ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();

    auto mecPk = pk->peekAtFront<WarningAppPacket>();

    if(registered && subscribed && !ueRegistered){
        ueRegistered = true;

        cMessage *m = new cMessage("updateRegistration");
        scheduleAt(simTime()+0.001, m);

        cMessage *p = new cMessage("updateSubscription");
        scheduleAt(simTime()+0.001, p);
    }

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
            ofstream myfile;
            myfile.open ("example.txt", ios::app);
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
        sendDeleteSubscription();
    }

    else
    {
        throw cRuntimeError("MECWarningAlertApp::handleUeMessage - packet not recognized");
    }
}

void MECWarningAlertApp::modifySubscription()
{
    if(isMigrated)
        return;

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
    std::string host = serviceSocket_.getRemoteAddress().str()+":"+std::to_string(serviceSocket_.getRemotePort());
    Http::sendPutRequest(&serviceSocket_, body.c_str(), host.c_str(), uri.c_str());
}

void MECWarningAlertApp::sendSubscription()
{
    if(isMigrated)
        return;

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
    std::string host = serviceSocket_.getRemoteAddress().str()+":"+std::to_string(serviceSocket_.getRemotePort());

    if(par("logger").boolValue())
    {
        ofstream myfile;
        myfile.open ("example.txt", ios::app);
        if(myfile.is_open())
        {
            myfile <<"["<< NOW << "] MEWarningAlertApp - Sent POST circleNotificationSubscription the Location Service \n";
            myfile.close();
        }
    }

    Http::sendPostRequest(&serviceSocket_, body.c_str(), host.c_str(), uri.c_str());
}

void MECWarningAlertApp::sendDeleteSubscription()
{
    std::string uri = "/example/location/v2/subscriptions/area/circle/" + subId;
    std::string host = serviceSocket_.getRemoteAddress().str()+":"+std::to_string(serviceSocket_.getRemotePort());
    Http::sendDeleteRequest(&serviceSocket_, host.c_str(), uri.c_str());
}

void MECWarningAlertApp::established(int connId)
{
    EV << "MECWarningAlertApp::established - "<< connId << endl;
    if(connId == mp1Socket_.getSocketId())
    {
        EV << "MECWarningAlertApp::established - Mp1Socket"<< endl;
        // get endPoint of the required service

        const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=ApplicationMobilityService";
        getServiceData(uri);

        return;
    }
    else if(connId == amsSocket_.getSocketId())
    {
        EV << "MECWarningAlertApp::established - AMSSocket"<< endl;

        // Send registration

        nlohmann::ordered_json registrationBody;
        registrationBody = nlohmann::ordered_json();
        registrationBody["serviceConsumerId"]["appInstanceId"] = std::string(getName());
        registrationBody["serviceConsumerId"]["mepId"] = "1234";
        registrationBody["deviceInformation"] = nlohmann::json::array();
//        if(!ueAppAddress.isUnspecified() && ueAppPort > 0){
//            nlohmann::ordered_json deviceInformation;
//            nlohmann::ordered_json associateId;
//
//            associateId["type"] = "UE_IPv4_ADDRESS";
//            associateId["value"] = ueAppAddress.str();
//            deviceInformation["associateId"] = associateId;
//            deviceInformation["appMobilityServiceLevel"] = "APP_MOBILITY_NOT_ALLOWED";
//            deviceInformation["contextTransferState"] = "NOT_TRANSFERRED";
//
//            registrationBody["deviceInformation"].push_back(deviceInformation);
//        }

        EV << "Registration with body" << registrationBody.dump().c_str() << endl;
        std::string host = amsSocket_.getRemoteAddress().str()+":"+std::to_string(amsSocket_.getRemotePort());
        const char *uri = "/example/amsi/v1/app_mobility_services/";
        Http::sendPostRequest(&amsSocket_, registrationBody.dump().c_str(), host.c_str(), uri);

        return;
    }
    else if (connId == serviceSocket_.getSocketId())
    {
        EV << "MECWarningAlertApp::established - serviceSocket"<< endl;
        // the connectService message is scheduled after a start mec app from the UE app, so I can
        // response to her here, once the socket is established
        auto ack = inet::makeShared<WarningAppPacket>();
        ack->setType(START_ACK);
        ack->setChunkLength(inet::B(2));
        inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
        packet->insertAtBack(ack);
        ueSocket.sendTo(packet, ueAppAddress, ueAppPort);
        sendSubscription();
        return;
    }
    else if (connId == stateSocket_->getSocketId()){
        EV << "MECWarningAlertApp::established - stateSocket"<< endl;

        if(!isMigrating){
            EV << "MECWarningAlertApp::established - old MecApp"<< endl;

            std::string module_name = std::string(getName());
            inet::Packet *packet = new inet::Packet("SyncState");
            auto syncMessage = inet::makeShared<MecWarningAppSyncMessage>();
            syncMessage->setPositionX(centerPositionX);
            syncMessage->setPositionY(centerPositionY);
            syncMessage->setRadius(radius);
            syncMessage->setUeAddress(ueAppAddress);
            syncMessage->setUePort(ueAppPort);
            syncMessage->setContextId(std::stoi(module_name.substr(module_name.find('[') + 1, module_name.find(']') - module_name.find('[') - 1)));
            syncMessage->setChunkLength(inet::B(28));

            packet->insertAtBack(syncMessage);

            stateSocket_->send(packet);

            EV << "MECWarningAlertApp::context message sent closing socket" << endl;
            stateSocket_->close();

            isMigrated = true;

            if(!subId.empty()){
                EV << "MECWarningAlertApp::deleting subscription " << subId << endl;
                sendDeleteSubscription();
            }

            if(!amsRegistrationId.empty()){
               cMessage *m = new cMessage("updateRegistration");
               scheduleAt(simTime()+0.001, m);
            }
        }
    }
    else if (connId == serverSocket_.getSocketId()){
        EV << "MECWarningAlertApp::established - serverSocket: waiting for state transfer"<< endl;

    }
    else
    {
        throw cRuntimeError("MecAppBase::socketEstablished - Socket %d not recognized", connId);
    }
}

void MECWarningAlertApp::handleMp1Message()
{
    EV << "MECWarningAlertApp::handleMp1Message - payload: " << mp1HttpMessage->getBody() << endl;
    try
    {
        nlohmann::json jsonBody = nlohmann::json::parse(mp1HttpMessage->getBody()); // get the JSON structure
        if(!jsonBody.empty())
        {
            jsonBody = jsonBody[0];
            std::string serName = jsonBody["serName"];
            if(serName.compare("LocationService") == 0)
            {
                if(jsonBody.contains("transportInfo"))
                {
                    nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                    EV << "address: " << endPoint["host"] << " port: " <<  endPoint["port"] << endl;
                    std::string address = endPoint["host"];
                    serviceAddress = L3AddressResolver().resolve(address.c_str());;
                    servicePort = endPoint["port"];
                }
            }
            else if(serName.compare("ApplicationMobilityService") == 0){
                if(jsonBody.contains("transportInfo"))
                {
                    nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                    EV << "address: " << endPoint["host"] << " port: " <<  endPoint["port"] << endl;
                    std::string address = endPoint["host"];
                    amsAddress = L3AddressResolver().resolve(address.c_str());;
                    amsPort = endPoint["port"];

                    // connect to service
                    cMessage *m = new cMessage("connectAMS");
                    scheduleAt(simTime()+0.005, m);
                }
            }
            else
            {
                EV << "MECWarningAlertApp::handleMp1Message - Service not found"<< endl;
                serviceAddress = L3Address();
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

void MECWarningAlertApp::handleAmsMessage()
{
    try
    {

        if(amsHttpMessage->getType() == REQUEST){
            EV << "MECWarningAlertApp::handleAmsMessage - Received request - payload: " << " " << amsHttpMessage->getBody() << endl;
            HttpRequestMessage* amsRequest = check_and_cast<HttpRequestMessage*>(amsHttpMessage);
            nlohmann::json jsonBody = nlohmann::json::parse(amsRequest->getBody());


            if(std::string(amsRequest->getUri()).compare(webHook) == 0 && !jsonBody.empty()){
                MobilityProcedureNotification *notification = new MobilityProcedureNotification();
                notification->fromJson(jsonBody);
                std::string type = notification->getMobilityStatusString();
                if(type.empty()){
                    throw cRuntimeError("mobility status not specified in the notification");
                }

                EV << "MECWarningAlertApp::handleAmsMessage - Analyzing notification - payload: " << " " << amsHttpMessage->getBody() << endl;
                if(type.compare("INTERHOST_MOVEOUT_TRIGGERED") == 0 && jsonBody.contains("targetAppInfo")){
                   TargetAppInfo* targetAppInfo = new TargetAppInfo();
                   targetAppInfo->fromJson(jsonBody["targetAppInfo"]);

                   if(targetAppInfo->getCommInterface().size() != 0 && targetAppInfo->getCommInterface()[0].addr != localAddress){
                       EV << "MECWarningAlertApp::handleAmsMessage - Analyzing notification - TargetAppInfo found: " << " " << amsHttpMessage->getBody() << endl;
                       migrationAddress = targetAppInfo->getCommInterface()[0].addr;
                       migrationPort = targetAppInfo->getCommInterface()[0].port;
                       cMessage *m = new cMessage("migrateState");
                       scheduleAt(simTime()+0.005, m);
                   }
                }
                else if(type.compare("INTERHOST_MOVEOUT_COMPLETED") == 0 && jsonBody.contains("targetAppInfo")){

//                    cMessage *m = new cMessage("deleteRegistration");
//                    scheduleAt(simTime()+0.005, m);
//
//                    cMessage *d = new cMessage("deleteModule");
//                    scheduleAt(simTime()+0.7, d);

                    EV << "MECWarningAlertApp::handleAmsMessage - Deletion has been scheduled" << endl;
                }
            }

        }
        else if(amsHttpMessage->getType() == RESPONSE){
            EV << "MECWarningAlertApp::handleAmsMessage - Received response - payload: " << " " << amsHttpMessage->getBody() << endl;
            HttpResponseMessage* amsResponse = check_and_cast<HttpResponseMessage*>(amsHttpMessage);
            nlohmann::json jsonBody = nlohmann::json::parse(amsResponse->getBody());
            if(!jsonBody.empty()){
                if(jsonBody.contains("appMobilityServiceId"))
                {
                    amsRegistrationId = jsonBody["appMobilityServiceId"];
                    registered = true;
                    EV << "MECWarningAlertApp::handleAmsMessage - registration ID: " << amsRegistrationId << endl;

                    cMessage *m = new cMessage("subscribeAms");
                    scheduleAt(simTime()+0.001, m);
                }
                else if(jsonBody.contains("callbackReference")){

                    std::string type = jsonBody["filterCriteria"]["mobilityStatus"];

                    if(type.compare("INTERHOST_MOVEOUT_TRIGGERED") == 0){
                        std::stringstream stream;
                        stream << "sub" << jsonBody["subscriptionId"];
                        amsSubscriptionId = stream.str();
                        EV << "MECWarningAlertApp::handleAmsMessage - subscription ID triggered: " << amsSubscriptionId << endl;

//                        cMessage *b = new cMessage("subscribeAmsCompleted");
//                        scheduleAt(simTime()+0.003, b);
                    }
                    else if(type.compare("INTERHOST_MOVEOUT_COMPLETED") == 0){
                        std::stringstream stream;
                        stream << "sub" << jsonBody["subscriptionId"];
                        amsSubscriptionId_completed = stream.str();
                        EV << "MECWarningAlertApp::handleAmsMessage - subscription ID completed: " << amsSubscriptionId_completed << endl;
                    }

                    if(!amsSubscriptionId.empty())
                    {//&& !amsSubscriptionId_completed.empty()){
                        subscribed = true;
                    }

                }
            }
        }
        else{
            EV << "MECWarningAlertApp::handleAmsMessage - Message type not recognized " << endl;

        }
    }
    catch(nlohmann::detail::parse_error e)
    {
        EV <<  e.what() << std::endl;
        // body is not correctly formatted in JSON, manage it
        return;
    }
}

void MECWarningAlertApp::handleServiceMessage()
{
    if(serviceHttpMessage->getType() == REQUEST)
    {
        Http::send204Response(&serviceSocket_); // send back 204 no content
        nlohmann::json jsonBody;
        EV << "MEClusterizeService::handleTcpMsg - REQUEST " << serviceHttpMessage->getBody()<< endl;
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
                    EV << "MEClusterizeService::handleTcpMsg - Ue is Entered in the danger zone "<< endl;
                    alert->setDanger(true);

                    if(par("logger").boolValue())
                    {
                        ofstream myfile;
                        myfile.open ("example.txt", ios::app);
                        if(myfile.is_open())
                        {
                            myfile <<"["<< NOW << "] MEWarningAlertApp - Received circleNotificationSubscription notification from Location Service. UE's entered the red zone! \n";
                            myfile.close();
                        }
                    }

                    // send subscription for leaving..
                    modifySubscription();

                }
                else if (criteria == "Leaving")
                {
                    EV << "MEClusterizeService::handleTcpMsg - Ue left from the danger zone "<< endl;
                    alert->setDanger(false);
                    if(par("logger").boolValue())
                    {
                        ofstream myfile;
                        myfile.open ("example.txt", ios::app);
                        if(myfile.is_open())
                        {
                            myfile <<"["<< NOW << "] MEWarningAlertApp - Received circleNotificationSubscription notification from Location Service. UE's exited the red zone! \n";
                            myfile.close();
                        }
                    }
                    sendDeleteSubscription();
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
        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(serviceHttpMessage);
        EV <<  "MEClusterizeService::handleTcpMsg - received response" << endl;

        if(rspMsg->getCode() == 204) // in response to a DELETE
        {
            EV << "MEClusterizeService::handleTcpMsg - response 204, removing circle" << rspMsg->getBody()<< endl;
             serviceSocket_.close();
             getSimulation()->getSystemModule()->getCanvas()->removeFigure(circle);

        }
        else if(rspMsg->getCode() == 201) // in response to a POST
        {
            nlohmann::json jsonBody;
            EV << "MEClusterizeService::handleTcpMsg - response 201 " << rspMsg->getBody()<< endl;
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


void MECWarningAlertApp::handleStateMessage(){
    EV << "MECWarningAlertApp::handleStateMessage - received message " << endl;

    auto data = stateMessage->peekData<MecWarningAppSyncMessage>();
    EV << "MECWarningAlertApp::setting new state: " << endl;
    EV << "MECWarningAlertApp::position x: " << data->getPositionX() << endl;
    EV << "MECWarningAlertApp::position y: " << data->getPositionY() << endl;
    EV << "MECWarningAlertApp::radius: " << data->getRadius() << endl;
    EV << "MECWarningAlertApp::ue addr: " << data->getUeAddress() << endl;
    EV << "MECWarningAlertApp::ue port: " << data->getUePort() << endl;
    EV << "MECWarningAlertApp::contextId: " << data->getContextId() << endl;

    centerPositionX = data->getPositionX();
    centerPositionY = data->getPositionY();
    radius = data->getRadius();
    ueAppAddress = data->getUeAddress();
    ueAppPort = data->getUePort();

    EV << "MECWarningAlertApp::handleStateMessage - new state injected!" << endl;

    cMessage *b = new cMessage("waitForInitialization");
    scheduleAt(simTime()+0.001, b);

}

void MECWarningAlertApp::handleSelfMessage(cMessage *msg)
{
    if(strcmp(msg->getName(), "connectMp1") == 0)
    {
        EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
        connect(&mp1Socket_, mp1Address, mp1Port);
    }
    else if(strcmp(msg->getName(), "waitForInitialization") == 0){
        if(registered && subscribed){
            cMessage *a = new cMessage("updateRegistration");
            scheduleAt(simTime()+0.001, a);

            cMessage *m = new cMessage("updateSubscription");
            scheduleAt(simTime()+0.001, m);

            cMessage *p = new cMessage("connectServiceStandalone");
            scheduleAt(simTime()+0.2, p);
        }else{
            EV << "not registered/subscribed yet" << endl;
            cMessage *b = new cMessage("waitForInitialization");
            scheduleAt(simTime()+0.005, b);
        }
    }
    else if(strcmp(msg->getName(), "connectService") == 0)
    {
        EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
        if(!serviceAddress.isUnspecified() && serviceSocket_.getState() != inet::TcpSocket::CONNECTED)
        {
            connect(&serviceSocket_, serviceAddress, servicePort);
        }
        else
        {
            if(serviceAddress.isUnspecified())
                EV << "MECWarningAlertApp::handleSelfMessage - service IP address is  unspecified (maybe response from the service registry is arriving)" << endl;
            else if(serviceSocket_.getState() == inet::TcpSocket::CONNECTED)
                EV << "MECWarningAlertApp::handleSelfMessage - service socket is already connected" << endl;
            auto nack = inet::makeShared<WarningAppPacket>();
            // the connectService message is scheduled after a start mec app from the UE app, so I can
            // response to her here
            nack->setType(START_NACK);
            nack->setChunkLength(inet::B(2));
            inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
            packet->insertAtBack(nack);
            ueSocket.sendTo(packet, ueAppAddress, ueAppPort);

//            throw cRuntimeError("service socket already connected, or service IP address is unspecified");
        }
    }else if(strcmp(msg->getName(), "connectServiceStandalone") == 0)
    {
        EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
        if(!serviceAddress.isUnspecified() && serviceSocket_.getState() != inet::TcpSocket::CONNECTED)
        {
            connect(&serviceSocket_, serviceAddress, servicePort);
        }
        else
        {
            if(serviceAddress.isUnspecified()){
                EV << "MECWarningAlertApp::handleSelfMessage - service IP address is  unspecified (maybe response from the service registry is arriving)" << endl;
                cMessage *m = new cMessage("connectServiceStandalone");
                scheduleAt(simTime()+0.01, m);
            }
            else if(serviceSocket_.getState() == inet::TcpSocket::CONNECTED)
                EV << "MECWarningAlertApp::handleSelfMessage - service socket is already connected" << endl;
        }
    }else if(strcmp(msg->getName(), "connectAMS") == 0){
        EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
        if(!amsAddress.isUnspecified() && amsSocket_.getState() != inet::TcpSocket::CONNECTED)

            connect(&amsSocket_, amsAddress, amsPort);
    }else if (strcmp(msg->getName(), "getServiceData") == 0){
        EV << "MECWarningAlertApp::handleMessage get location data" << endl;
        const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=LocationService";
        getServiceData(uri);
    }
    else if (strcmp(msg->getName(), "subscribeAms") == 0){
        EV << "MECWarningAlertApp::handleMessage sending subscription" << endl;
        EV << getParentModule()->getFullPath() << endl;
        nlohmann::ordered_json subscriptionBody_;
        subscriptionBody_ = nlohmann::ordered_json();
        subscriptionBody_["_links"]["self"]["href"] = "";
        subscriptionBody_["callbackReference"] = localAddress.str() + ":" + std::to_string(par("localUePort").intValue()) + webHook;
        subscriptionBody_["requestTestNotification"] = false;
        subscriptionBody_["websockNotifConfig"]["websocketUri"] = "";
        subscriptionBody_["websockNotifConfig"]["requestWebsocketUri"] = false;
        subscriptionBody_["filterCriteria"]["appInstanceId"] = getName();
        subscriptionBody_["filterCriteria"]["associateId"] = nlohmann::json::array();
        subscriptionBody_["filterCriteria"]["mobilityStatus"] = "INTERHOST_MOVEOUT_TRIGGERED";
        subscriptionBody_["subscriptionType"] = "MobilityProcedureSubscription";
        EV << subscriptionBody_;

        std::string host = amsSocket_.getRemoteAddress().str()+":"+std::to_string(amsSocket_.getRemotePort());
        std::string uristring = "/example/amsi/v1/subscriptions/";
        Http::sendPostRequest(&amsSocket_, subscriptionBody_.dump().c_str(), host.c_str(), uristring.c_str());

    }
    else if (strcmp(msg->getName(), "subscribeAmsCompleted") == 0){
        EV << "MECWarningAlertApp::handleMessage sending subscription" << endl;
        EV << getParentModule()->getFullPath() << endl;
        nlohmann::ordered_json subscriptionBody_;
        subscriptionBody_ = nlohmann::ordered_json();
        subscriptionBody_["_links"]["self"]["href"] = "";
        subscriptionBody_["callbackReference"] = localAddress.str() + ":" + std::to_string(par("localUePort").intValue()) + webHook;
        subscriptionBody_["requestTestNotification"] = false;
        subscriptionBody_["websockNotifConfig"]["websocketUri"] = "";
        subscriptionBody_["websockNotifConfig"]["requestWebsocketUri"] = false;
        subscriptionBody_["filterCriteria"]["appInstanceId"] = getName();
        subscriptionBody_["filterCriteria"]["associateId"] = nlohmann::json::array();
        subscriptionBody_["filterCriteria"]["mobilityStatus"] = "INTERHOST_MOVEOUT_COMPLETED";
        subscriptionBody_["subscriptionType"] = "MobilityProcedureSubscription";
        EV << subscriptionBody_;

        std::string host = amsSocket_.getRemoteAddress().str()+":"+std::to_string(amsSocket_.getRemotePort());
        std::string uristring = "/example/amsi/v1/subscriptions/";
        Http::sendPostRequest(&amsSocket_, subscriptionBody_.dump().c_str(), host.c_str(), uristring.c_str());
    }
    else if (strcmp(msg->getName(), "updateSubscription") == 0){
        // Update registration
        EV << getParentModule()->getFullPath() << endl;
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
        subscriptionBody_["filterCriteria"]["mobilityStatus"] = "INTERHOST_MOVEOUT_TRIGGERED";
        subscriptionBody_["subscriptionType"] = "MobilityProcedureSubscription";
        EV << subscriptionBody_;

        std::string host = amsSocket_.getRemoteAddress().str()+":"+std::to_string(amsSocket_.getRemotePort());
        std::string uristring = "/example/amsi/v1/subscriptions/" + amsSubscriptionId;
        Http::sendPutRequest(&amsSocket_, subscriptionBody_.dump().c_str(), host.c_str(), uristring.c_str());

        cMessage *m = new cMessage("getServiceData");
        scheduleAt(simTime()+0.001, m);

//        cMessage *b = new cMessage("updateSubscriptionCompleted");
//        scheduleAt(simTime()+0.02, b);
    }
    else if (strcmp(msg->getName(), "updateSubscriptionCompleted") == 0){
        // Update registration
        EV << getParentModule()->getFullPath() << endl;
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
        subscriptionBody_["filterCriteria"]["mobilityStatus"] = "INTERHOST_MOVEOUT_COMPLETED";
        subscriptionBody_["subscriptionType"] = "MobilityProcedureSubscription";
        EV << subscriptionBody_;

        std::string host = amsSocket_.getRemoteAddress().str()+":"+std::to_string(amsSocket_.getRemotePort());
        std::string uristring = "/example/amsi/v1/subscriptions/" + amsSubscriptionId_completed;
        Http::sendPutRequest(&amsSocket_, subscriptionBody_.dump().c_str(), host.c_str(), uristring.c_str());

    }
    else if (strcmp(msg->getName(), "updateRegistration") == 0){

        // Update registration
        nlohmann::ordered_json registrationBody;
        registrationBody = nlohmann::ordered_json();
        registrationBody["serviceConsumerId"]["appInstanceId"] = std::string(getName());
        registrationBody["serviceConsumerId"]["mepId"] = "1234";
        registrationBody["deviceInformation"] = nlohmann::json::array();
        nlohmann::ordered_json deviceInformation;
        nlohmann::ordered_json associateId;

        associateId["type"] = "UE_IPv4_ADDRESS";
        associateId["value"] = ueAppAddress.str();
        deviceInformation["associateId"] = associateId;
        deviceInformation["appMobilityServiceLevel"] = "APP_MOBILITY_NOT_ALLOWED";
        if(isMigrated)
            deviceInformation["contextTransferState"] = "USER_CONTEXT_TRANSFER_COMPLETED";
        else
            deviceInformation["contextTransferState"] = "NOT_TRANSFERRED";

        registrationBody["deviceInformation"].push_back(deviceInformation);

        std::string host = amsSocket_.getRemoteAddress().str()+":"+std::to_string(amsSocket_.getRemotePort());
        std::string uristring = "/example/amsi/v1/app_mobility_services/" + amsRegistrationId;
        const char *uri = uristring.c_str();
        Http::sendPutRequest(&amsSocket_, registrationBody.dump().c_str(), host.c_str(), uri);
    }
    else if (strcmp(msg->getName(), "migrateState") == 0){
        EV << "Connecting to new app " << migrationAddress << ":" << migrationPort << endl;
        connect(stateSocket_, migrationAddress, migrationPort);
    }
    else if (strcmp(msg->getName(), "deleteRegistration") == 0){
        EV << "Deleting registration"<< endl;

        std::string host = amsSocket_.getRemoteAddress().str()+":"+std::to_string(amsSocket_.getRemotePort());
        std::string uristring = "/example/amsi/v1/app_mobility_services/" + amsRegistrationId;
        const char *uri = uristring.c_str();
        Http::sendDeleteRequest(&amsSocket_, host.c_str(), uri);

        cMessage *b = new cMessage("deleteSubscriptionTriggered");
        scheduleAt(simTime()+0.001, b);

    }
    else if (strcmp(msg->getName(), "deleteSubscriptionTriggered") == 0){
        EV << "Deleting subscription triggered"<< endl;

        std::string host = amsSocket_.getRemoteAddress().str()+":"+std::to_string(amsSocket_.getRemotePort());
        std::string uristring = "/example/amsi/v1/subscriptions/" + amsSubscriptionId;
        const char *uri = uristring.c_str();
        Http::sendDeleteRequest(&amsSocket_, host.c_str(), uri);

        cMessage *b = new cMessage("deleteSubscriptionCompleted");
        scheduleAt(simTime()+0.02, b);

    }
    else if (strcmp(msg->getName(), "deleteSubscriptionCompleted") == 0){
        EV << "Deleting subscription completed"<< endl;

        std::string host = amsSocket_.getRemoteAddress().str()+":"+std::to_string(amsSocket_.getRemotePort());
        std::string uristring = "/example/amsi/v1/subscriptions/" + amsSubscriptionId_completed;
        const char *uri = uristring.c_str();
        Http::sendDeleteRequest(&amsSocket_, host.c_str(), uri);

    }
    else if (strcmp(msg->getName(), "deleteModule") == 0){
        EV << "Deleting module - nothing to do"<< endl;

//        callFinish();
//        deleteModule();
    }

    delete msg;
}

void MECWarningAlertApp::getServiceData(const char* uri){
    std::string host = mp1Socket_.getRemoteAddress().str()+":"+std::to_string(mp1Socket_.getRemotePort());

    Http::sendGetRequest(&mp1Socket_, host.c_str(), uri);
}
