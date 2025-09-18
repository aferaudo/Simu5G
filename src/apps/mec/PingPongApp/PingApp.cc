#include "inet/common/INETUtils.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "PingApp.h"

#include <random>
#include <chrono>

#include "nodes/mec/Federator/Messages/MEFmessages_m.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/chunk/BytesChunk.h"

Define_Module(PingApp);

PingApp::PingApp(){
    requestAppMsg = nullptr;
}


void PingApp::finish() {
    DMecAppBaseDyn::finish();

    if (socket.getState() == inet::UdpSocket::CONNECTED)
        //socket.close();

    std::cout << ">> [PingApp::finish()] called" << std::endl;


}

PingApp::~PingApp() {
    std::cout << ">> [PingApp::~PingApp()] destructor called" << std::endl;


}


void PingApp::initialize(int stage) {
    DMecAppBaseDyn::initialize(stage);


    EV << "DEBUG APP: " << par("isMigrating") << " - " << par("addressMigration") << " - " << par("portMigration") << endl;
    pendingOps = 0;

    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    localPort = par("localUePort");
    destPort = par("destPort");

    pingSending = 0;


    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort);

    EV_INFO << "PingApp:socketBind at: " << localPort << endl;

    id = par("mecAppId");

    webHook = "/amsWebHook_" + std::to_string(id);


    amsAddress = L3AddressResolver().resolve(par("addressMigration"));
    localAddress = L3AddressResolver().resolve(getParentModule()->getFullPath().c_str());

    stateSocket.setOutputGate(gate("socketOut"));

    if (amsAddress.isUnspecified()) {
        EV_INFO << "PingApp:create id" << endl;
        scheduleAt(simTime(), new cMessage("connectMp1"));
        isMigrated=false;
        amsOld=false;
    }else{
        EV_INFO << "PingApp:import id" << endl;
        isMigrated=true;
        amsOld=true;

        amsPort = par("portMigration").intValue();

        // connect to service
        if(!amsAddress.isUnspecified() && amsSocket_.getState() != inet::TcpSocket::CONNECTED){
                EV_INFO << "PingApp:connect at AMS" << endl;
                connect(&amsSocket_, amsAddress, amsPort);
        }

        startTimeOp = simTime();

        serverSocket_.setOutputGate(gate("socketOut"));
        serverSocket_.setCallback(this);
        serverSocket_.bind(localAddress, localPort);
        serverSocket_.listenOnce();



    }

    //scheduleAt(simTime(), new cMessage("connectMp1"));

    EV_INFO << "PingApp:id per migrazione ams: " << id << endl;

    //cMessage* selfPing = new cMessage("sendPing");
    //scheduleAt(simTime() + 1, selfPing); // primo ping dopo 1s



    requestAppMsg = new cMessage("requestApp");
    //scheduleAt(simTime() + 1, requestAppMsg);



}

void PingApp::handleMessage(cMessage *msg)
{
    EV << "PingApp::handleMessage - received " << msg << endl;

    if(strcmp(msg->getName(), "startTerminationProcedure") == 0){
        EV << "PingApp::handleMessage - scheduling termination " << endl;

        cMessage *b = new cMessage("deleteRegistration");
        scheduleAt(simTime()+0.001, b);
        delete msg;
        return;
    }

    DMecAppBaseDyn::handleMessage(msg);

}
void PingApp::handleGenericMessage(cMessage* msg) {

    EV << "PingApp:handleGenericMessage" << endl;

    auto packet = dynamic_cast<Packet*>(msg);
    if (packet) {
        const char* name = packet->getName();


        if (strcmp(name, "appResponseVIMtoAPP") == 0) {
            auto data = packet->peekData<AppResponse>();
            destAddr = inet::L3AddressResolver().resolve(data->getAppAddress());
            destPort = data->getAppPort();

            EV << "PingApp:handleMessage - recived appAddress:port " << destAddr << ":" << destPort << endl;

            cMessage* selfPing = new cMessage("sendPing");
            scheduleAt(simTime(), selfPing);
            pendingOps--;

            simtime_t delay = simTime() - startTime;
            emit(requestAppFed, delay);
        }else if (strcmp(name, "appResponseVIMtoAPPINT") == 0) {
            auto data = packet->peekData<AppResponse>();
            destAddr = inet::L3AddressResolver().resolve(data->getAppAddress());
            destPort = data->getAppPort();

            EV << "PingApp:handleMessage - recived appAddress:port " << destAddr << ":" << destPort << endl;

            cMessage* selfPing = new cMessage("sendPing");
            scheduleAt(simTime(), selfPing);
            pendingOps--;

            simtime_t delay = simTime() - startTime;
            emit(requestAppInt, delay);
        }else if (strcmp(name, "Pong") == 0){
            pendingOps--;
            EV << "PONG " << endl;

            simtime_t delay = simTime() - startTimePing;
            emit(pingPong, delay);
        }else if(strcmp(name, "migrateStateInfo") == 0){

            auto data = packet->peekData<MigrateState>();
            pingSending = data->getPingSending();

            EV << "PingApp:handleMessage - recived pingsending to migrate State: " << pingSending << endl;

            simtime_t delay = simTime() - startTimeOpTWO;
            emit(registerSignal("debugTWO"), delay);
            cMessage* updateRegistration = new cMessage("updateRegistration");
            scheduleAt(simTime()+0.1, updateRegistration);



        }else if(strcmp(name, "migrateStateRequest") == 0){
            auto src = packet->getTag<L3AddressInd>()->getSrcAddress();
            EV_INFO << "Received " << name << " from " << src << endl;

            auto data = packet->peekData<MigrateState>();
            EV << "Sent to new app info " << src << ":" << data->getPort() << endl;


            inet::Packet *packetResp = new inet::Packet("migrateStateInfo");

            auto request = inet::makeShared<MigrateState>();

            request->setPingSending(pingSending);

            request->setChunkLength(inet::B(4096));
            packetResp->insertAtBack(request);


            EV << "PingApp::send migrate info" << endl;

            socket.sendTo(packetResp, src, data->getPort());
        }

        delete packet;
    }

}

void PingApp::handleSelfMessage(cMessage* msg) {
    EV << "PingApp:handleSelfMessage " << msg->getName() << endl;
    if (strcmp(msg->getName(), "sendPing")==0) {
        sendPing();
    }else if (strcmp(msg->getName(), "requestApp")==0) {
        requestApp();
    }else if(strcmp(msg->getName(), "connectMp1") == 0) {
        EV_INFO << "PingApp:connect at mp1" << endl;
        connect(&mp1Socket_, mp1Address, mp1Port);
    }else if(strcmp(msg->getName(), "subscribeAms") == 0) {
        subscribeAms();
    }else if(strcmp(msg->getName(), "registrationAms") == 0){

        EV << "PingPongApp AMS connected... send reg" << endl;
        nlohmann::ordered_json registrationBody;
        registrationBody["serviceConsumerId"]["appInstanceId"] = std::to_string(id);
        registrationBody["serviceConsumerId"]["mepId"] = std::to_string(id);
        registrationBody["deviceInformation"] = nlohmann::json::array(); // opzionale

        std::string host = amsSocket_.getRemoteAddress().str() + ":" + std::to_string(amsSocket_.getRemotePort());
        Http::sendPostRequest(&amsSocket_, registrationBody.dump().c_str(), host.c_str(), "/example/amsi/v1/app_mobility_services/");

    }else if (strcmp(msg->getName(), "migrateState") == 0){
        /*
        EV << "Sent to new app info " << migrationAddress << ":" << migrationPort<< endl;


        inet::Packet *packetResp = new inet::Packet("migrateStateInfo");

        auto request = inet::makeShared<MigrateState>();

        request->setPingSending(pingSending);

        inet::B packetSize = inet::B(100 * 1024 * 1024);
        request->setChunkLength(packetSize);
        packetResp->insertAtBack(request);


        EV << "PingApp::send migrate info" << endl;

        socket.sendTo(packetResp, migrationAddress, migrationPort);
        */

        connect(stateSocket_, migrationAddress, migrationPort);


    }else if (strcmp(msg->getName(), "updateRegistration") == 0){


            EV << "PingApp::updateRegistration - OK id" << endl;

            // Update registration
            nlohmann::ordered_json registrationBody;
            registrationBody = nlohmann::ordered_json();
            registrationBody["serviceConsumerId"]["appInstanceId"] = std::to_string(id);
            registrationBody["serviceConsumerId"]["mepId"] = std::to_string(id);
            registrationBody["deviceInformation"] = nlohmann::json::array();

            nlohmann::ordered_json deviceInformation;
            nlohmann::ordered_json associateId;

            associateId["type"] = "UE_IPv4_ADDRESS";
            associateId["value"] = std::to_string(id);

            deviceInformation["associateId"] = associateId;
            //deviceInformation["associateId"] = nlohmann::json::array();
            deviceInformation["appMobilityServiceLevel"] = "APP_MOBILITY_NOT_ALLOWED";

            deviceInformation["contextTransferState"] = "USER_CONTEXT_TRANSFER_COMPLETED";

            registrationBody["deviceInformation"].push_back(deviceInformation);

            std::string host = amsSocket_.getRemoteAddress().str()+":"+std::to_string(amsSocket_.getRemotePort());
            std::string uristring = "/example/amsi/v1/app_mobility_services/" + amsRegistrationId;
            const char *uri = uristring.c_str();
            Http::sendPutRequest(&amsSocket_, registrationBody.dump().c_str(), host.c_str(), uri);



           simtime_t delay = simTime() - startTimeOp;
           emit(registerSignal("opTime"), delay);
           emit(registerSignal("stop"), simTime());
           simtime_t delay1 = simTime() - startTimeOpTWO;
           emit(registerSignal("beforMigrateTWO"), delay1);


    }else if (strcmp(msg->getName(), "deleteModule") == 0) {
        std::cout << "PingApp::deleteModule - terminating" << endl;

        cancelEvent(msg);
        //delete msg;

        cGate* gate = this->gate("viAppGate$o");
        cMessage* t = new cMessage("endTerminationProcedure");
        send(t, gate->getName());


    }else if(strcmp(msg->getName(), "deleteRegistration") == 0){
        std::cout << "PingApp::handleTermination - start termination" << endl;

        if (pendingOps > 0) {

            std::cout << "PingApp::handleTermination - Still processing. Rescheduling..." << endl;
            cMessage *retryMsg = new cMessage("deleteRegistration");
            scheduleAt(simTime() + 0.05, retryMsg);
            delete msg;
            return;
        }

       // Chiudi socket TCP se attivo
       if (amsSocket_.getState() == inet::TcpSocket::CONNECTED)
           amsSocket_.close();

       if (mp1Socket_.getState() == inet::TcpSocket::CONNECTED)
           mp1Socket_.close();

       // Eventuale socket di stato
       if (stateSocket_ && stateSocket_->getState() == inet::TcpSocket::CONNECTED)
           stateSocket_->close();


       /*
       if (requestAppMsg != nullptr) {
           if (requestAppMsg->isScheduled()) {
               cancelEvent(requestAppMsg);
           }
           delete requestAppMsg;
           requestAppMsg = nullptr;
       }
    */

       // pianifica deleteModule
       cMessage *deleteMsg = new cMessage("deleteModule");
       scheduleAt(simTime() + 0.05, deleteMsg);
    }

    delete msg;
}

void PingApp::sendPing() {
    startTimePing = simTime();
    pingSending++;
    pendingOps++;
    EV << "Sending ping(" << pingSending << ") to " << destAddr << ":" << destPort << endl;
    auto packet = new Packet("Ping");
    const auto& payload = makeShared<ByteCountChunk>(B(4));
    packet->insertAtBack(payload);
    socket.sendTo(packet, destAddr, destPort);
}

void PingApp::subscribeAms(){

    nlohmann::ordered_json subscriptionBody;
    subscriptionBody["_links"]["self"]["href"] = "";
    subscriptionBody["callbackReference"] = localAddress.str() + ":" + std::to_string(par("localUePort").intValue()) + webHook;
    subscriptionBody["requestTestNotification"] = false;
    subscriptionBody["websockNotifConfig"]["websocketUri"] = "";
    subscriptionBody["websockNotifConfig"]["requestWebsocketUri"] = false;
    //subscriptionBody["filterCriteria"]["appInstanceId"] = getName();
    subscriptionBody["filterCriteria"]["appInstanceId"] = std::to_string(id);
    subscriptionBody["filterCriteria"]["associateId"] = nlohmann::json::array();
    subscriptionBody["filterCriteria"]["mobilityStatus"] = nlohmann::json::array();
    subscriptionBody["filterCriteria"]["mobilityStatus"].push_back("INTERHOST_MOVEOUT_TRIGGERED");
    subscriptionBody["filterCriteria"]["mobilityStatus"].push_back("INTERHOST_MOVEOUT_COMPLETED");

    subscriptionBody["subscriptionType"] = "MobilityProcedureSubscription";

    EV << "JSON: " << subscriptionBody << endl;
    std::string host = amsSocket_.getRemoteAddress().str() + ":" + std::to_string(amsSocket_.getRemotePort());
    std::string uri = "/example/amsi/v1/subscriptions/";

    Http::sendPostRequest(&amsSocket_, subscriptionBody.dump().c_str(), host.c_str(), uri.c_str());
}

void PingApp::unsubscribeAms() {
    if (amsRegistrationId != "" && amsSubscriptionId != "") {
        std::string host = amsSocket_.getRemoteAddress().str() + ":" + std::to_string(amsSocket_.getRemotePort());

        std::string uri = "/example/amsi/v1/subscriptions/" + amsSubscriptionId;
        EV << "PingApp::unsubscribeAms subscription delete " << uri << endl;
        Http::sendDeleteRequest(&amsSocket_, host.c_str(), uri.c_str());

        uri = "/example/amsi/v1/app_mobility_services/" + amsRegistrationId;
        EV << "PingApp::unsubscribeAms registration delete " << uri << endl;
        Http::sendDeleteRequest(&amsSocket_, host.c_str(), uri.c_str());

        amsRegistrationId = "";
        amsSubscriptionId = "";
    }
}


void PingApp::requestApp(){
    pendingOps++;

    startTime = simTime();

    std::string dest = getParentModule()->getFullPath();

    inet::Packet *packetResp = new inet::Packet("appRequestAPPtoVIM");

    auto request = inet::makeShared<AppRequest>();

    request->setAppName("PongApp");
    L3Address localAddr = L3AddressResolver().resolve(dest.c_str());

    request->setIpRequest(localAddr.str().c_str());
    request->setPortRequest(localPort);

    request->setChunkLength(inet::B(64));
    packetResp->insertAtBack(request);


    EV << "PingApp::requestApp - request: PongApp " << endl;

    //socket.sendTo(packetResp, inet::L3AddressResolver().resolve(par("meoAddr")), 2000);
    socket.sendTo(packetResp, localAddr, par("vimPort"));
}


void PingApp::handleMp1Message(){
    EV << "PingPing::handleMp1Message - payload: " << mp1HttpMessage->getBody() << endl;
        if(mp1HttpMessage->getType() == RESPONSE){
            HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(mp1HttpMessage);
            //responsecounter--;
        }

        try
        {
            nlohmann::json jsonBody = nlohmann::json::parse(mp1HttpMessage->getBody()); // get the JSON structure
            if(!jsonBody.empty())
            {
                jsonBody = jsonBody[0];
                std::string serName = jsonBody["serName"];
                if(serName.compare("ApplicationMobilityService") == 0){
                    if(jsonBody.contains("transportInfo"))
                    {
                        nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                        EV << "address: " << endPoint["host"] << " port: " <<  endPoint["port"] << endl;
                        std::string address = endPoint["host"];
                        amsAddress = L3AddressResolver().resolve(address.c_str());;
                        amsPort = endPoint["port"];

                        // connect to service
                        if(!amsAddress.isUnspecified())
                                connect(&amsSocket_, amsAddress, amsPort);
                    }
                }
                else
                {
                    EV << "PingPong::handleMp1Message - Service not found"<< endl;
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

void PingApp::established(int connId) {
    EV << "PingApp::established" << endl;

    if(connId == mp1Socket_.getSocketId())
    {
        EV << "PingApp::established - Mp1Socket: " << mp1Address << ":" << mp1Port << endl;
        // get endPoint of the required service

        const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=ApplicationMobilityService";
        getServiceData(uri);

        return;
    }else if(connId == amsSocket_.getSocketId()) {

        //cMessage *m = new cMessage("subscribeAms");
        //scheduleAt(simTime()+0.001, m);
        cMessage *n = new cMessage("registrationAms");
        scheduleAt(simTime()+0.001, n);
    }else if(connId == stateSocket_->getSocketId()) {
        EV << "PingApp::established stateSocket" << endl;

        EV << "Sent to new app info " << migrationAddress << ":" << migrationPort<< endl;


        inet::Packet *packetResp = new inet::Packet("migrateStateInfoState");

        auto request = inet::makeShared<MigrateState>();

        request->setPingSending(pingSending);

        inet::B packetSize = inet::B(64);
        request->setChunkLength(inet::B(packetSize));
        request->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());


        packetResp->insertAtBack(request);


        EV << "PingApp::send migrate info" << endl;

        stateSocket_->send(packetResp);



    }else if(connId == serverSocket_.getSocketId()) {
        EV << "PingApp::established serverSocket" << endl;


    }


}

void PingApp::handleStateMessage(){
    //auto data = stateMessage->peekData<MigrateState>();
    //auto data = stateMessage->peekData<MigrateState>(inet::Chunk::PF_ALLOW_REINTERPRETATION);

   // pingSending = data->getPingSending();

    EV << stateMessage->printToString() << endl;

    EV << "PingApp:handleStateMessage - recived pingsending to migrate State: " << pingSending << endl;

    simtime_t delay = simTime() - startTimeOpTWO;
    emit(registerSignal("debugTWO"), delay);
    cMessage* updateRegistration = new cMessage("updateRegistration");
    scheduleAt(simTime()+0.1, updateRegistration);

}

void PingApp::getServiceData(const char* uri){
    std::string host = mp1Socket_.getRemoteAddress().str()+":"+std::to_string(mp1Socket_.getRemotePort());

    Http::sendGetRequest(&mp1Socket_, host.c_str(), uri);
    //responsecounter++;
}


void PingApp::handleAmsMessage(){
    try
    {

        if(amsHttpMessage->getType() == REQUEST){
            EV << "DMECWarningAlertApp::handleAmsMessage - Received request - payload: " << " " << amsHttpMessage->getBody() << endl;
            HttpRequestMessage* amsRequest = check_and_cast<HttpRequestMessage*>(amsHttpMessage);
            nlohmann::json jsonBody = nlohmann::json::parse(amsRequest->getBody());


            if(std::string(amsRequest->getUri()).compare(webHook) == 0 && !jsonBody.empty()){
                MobilityProcedureNotification *notification = new MobilityProcedureNotification();
                notification->fromJson(jsonBody);
                std::string type = notification->getMobilityStatusString();
                if(type.empty()){
                    throw cRuntimeError("mobility status not specified in the notification");
                }

                EV << "DMECWarningAlertApp::handleAmsMessage - Analyzing notification - payload: " << " " << amsHttpMessage->getBody() << endl;

                if(type.compare("INTERHOST_MOVEOUT_TRIGGERED") == 0 && jsonBody.contains("targetAppInfo")){
                   EV << "yes0" << endl;

                   TargetAppInfo* targetAppInfo = new TargetAppInfo();
                   targetAppInfo->fromJson(jsonBody["targetAppInfo"]);

                   if(targetAppInfo->getCommInterface().size() != 0 && targetAppInfo->getCommInterface()[0].addr != localAddress){
                       EV << "DMECWarningAlertApp::handleAmsMessage - Analyzing notification - TargetAppInfo found: " << " " << amsHttpMessage->getBody() << endl;
                       migrationAddress = targetAppInfo->getCommInterface()[0].addr;
                       migrationPort = targetAppInfo->getCommInterface()[0].port;
                       cMessage *m = new cMessage("migrateState");
                       scheduleAt(simTime()+0.005, m);

                   }
                   EV << "yes4" << endl;


                }
                else if(type.compare("INTERHOST_MOVEOUT_COMPLETED") == 0 && jsonBody.contains("targetAppInfo")){



                   TargetAppInfo* targetAppInfo = new TargetAppInfo();
                   targetAppInfo->fromJson(jsonBody["targetAppInfo"]);

                   if(targetAppInfo->getCommInterface().size() != 0 && targetAppInfo->getCommInterface()[0].addr != localAddress){
                       EV << "DMECWarningAlertApp::handleAmsMessage - deleteRegistration" << endl;
                       unsubscribeAms();
                       cMessage *m = new cMessage("deleteRegistration");
                       scheduleAt(simTime()+0.1, m);
                   }else if(targetAppInfo->getCommInterface().size() != 0 && targetAppInfo->getCommInterface()[0].addr == localAddress){
                       EV << "DMECWarningAlertApp::handleAmsMessage - Complete change AMS" << endl;
                       unsubscribeAms();
                       isMigrated=false;
                       cMessage *m = new cMessage("connectMp1");
                       scheduleAt(simTime()+0.001, m);
                       /*
                       simtime_t delay = simTime() - startTimeOp;
                       emit(registerSignal("opTime"), delay);
                       emit(registerSignal("stop"), simTime());
                       simtime_t delay1 = simTime() - startTimeOpTWO;
                       emit(registerSignal("beforMigrateTWO"), delay1);
                       */
                   }
//
//                    cMessage *d = new cMessage("deleteModule");
//                    scheduleAt(simTime()+0.7, d);

                    EV << "DMECWarningAlertApp::handleAmsMessage - Deletion has been scheduled" << endl;
                }
            }

        }
        else if(amsHttpMessage->getType() == RESPONSE){
            //responsecounter--;
            EV << "DMECWarningAlertApp::handleAmsMessage - Received response - payload: " << " " << amsHttpMessage->getBody() << endl;
            HttpResponseMessage* amsResponse = check_and_cast<HttpResponseMessage*>(amsHttpMessage);

            nlohmann::json jsonBody = nlohmann::json::parse(amsResponse->getBody());
            if(!jsonBody.empty()){
                if(jsonBody.contains("appMobilityServiceId"))
                {



                    amsRegistrationId = jsonBody["appMobilityServiceId"];
                    registered = true;
                    EV << "DMECWarningAlertApp::handleAmsMessage - registration ID: " << amsRegistrationId << endl;

                    cMessage *m = new cMessage("subscribeAms");
                    scheduleAt(simTime()+0.001, m);


                }
                else if(jsonBody.contains("callbackReference")){

                    std::stringstream stream;
                    stream << "sub" << jsonBody["subscriptionId"];
                    amsSubscriptionId = stream.str();
                    EV << "DMECWarningAlertApp::handleAmsMessage - subscription ID triggered: " << amsSubscriptionId << endl;



                    if(!amsSubscriptionId.empty())
                    {
                        subscribed = true;
                    }
                    if(!amsOld){
                        //seconda

                    }else{
                        //prima
                        simtime_t delay = simTime() - startTimeOp;
                        emit(registerSignal("beforMigrateONE"), delay);
                        amsOld = false;
                        startTimeOpTWO = simTime();
                    }

                    if(isMigrated){
                        nlohmann::ordered_json request;
                        request["notificationType"] = "MobilityProcedureNotification";
                        request["mobilityStatus"] = "INTERHOST_MOVEOUT_TRIGGERED";
                        request["_links"]["href"] = "";
                        request["associateId"] = nlohmann::json::array();
                        request["appInstanceId"] = std::to_string(id);
                        request["targetAppInfo"]["appInstanceId"] = std::to_string(id);
                        request["targetAppInfo"]["commInterface"]["ipAddresses"] = nlohmann::json::array();
                        nlohmann::ordered_json arrayVal;
                        arrayVal["host"] = localAddress.str();
                        arrayVal["port"] = localPort;
                        request["targetAppInfo"]["commInterface"]["ipAddresses"].push_back(arrayVal);
                        EV << "MecPlatformManagerDyn::Trigger ready: " << request.dump() << endl;
                        std::cout << "SENDING NOTIFICATION - MY HOST (handleFederationMigrationTrigger) " << endl;

                        std::string host = amsSocket_.getRemoteAddress().str() + ":" + std::to_string(amsSocket_.getRemotePort());

                        Http::sendPostRequest(&amsSocket_, request.dump().c_str(), host.c_str(), "/example/amsi/v1/eventNotification/");
                    }
                }
            }
        }
        else{
            EV << "DMECWarningAlertApp::handleAmsMessage - Message type not recognized " << endl;

        }
    }
    catch(nlohmann::detail::parse_error e)
    {
        EV <<  e.what() << std::endl;
        // body is not correctly formatted in JSON, manage it
        return;
    }
}







