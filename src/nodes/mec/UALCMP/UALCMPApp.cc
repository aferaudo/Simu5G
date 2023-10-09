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


// INET
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "inet/transportlayer/contract/tcp/TcpCommand_m.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"

#include <iostream>
#include <string>
#include <vector>

// Utils
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "common/utils/utils.h"

// MEC system
#include "nodes/mec/UALCMP/UALCMPApp.h"
#include "nodes/mec/MECOrchestrator/MecOrchestrator.h"
#include "nodes/mec/Dynamic/MEO/MecOrchestratorApp.h"
#include "nodes/mec/MECOrchestrator/ApplicationDescriptor/ApplicationDescriptor.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.h"


// Messages
#include "nodes/mec/UALCMP/UALCMPMessages/CreateContextAppMessage.h"
#include "nodes/mec/UALCMP/UALCMPMessages/CreateContextAppAckMessage.h"
#include "nodes/mec/UALCMP/UALCMPMessages/UALCMPMessages_m.h"
#include "nodes/mec/UALCMP/UALCMPMessages/UALCMPMessages_types.h"

// Notifications
#include "resources/AddressChangeNotification.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"


Define_Module(UALCMPApp);

UALCMPApp::UALCMPApp()
{
    baseUriQueries_ = "/example/dev_app/v1";
    baseUriSubscriptions_ = baseUriQueries_;
    supportedQueryParams_.insert("app_list");
    supportedQueryParams_.insert("app_contexts");
    scheduledSubscription = false;
    subscriptionUri_ = "/example/amsi/v1/subscriptions/";
    mecOrchestrator_ = nullptr;
    requestSno = 0;
    subscriptionId_ = 0;
    subscriptions_.clear();
    amsHttpCompleteMessage = nullptr;
    httpmanager_.clear();
}

void UALCMPApp::initialize(int stage)
{
    EV << "UALCMPApp::initialize stage " << stage << endl;
    if (stage == inet::INITSTAGE_LOCAL)
    {
        EV << "MecServiceBase::initialize" << endl;
        serviceName_ = par("serviceName").stringValue();

        requestServiceTime_ = par("requestServiceTime");
        requestService_ = new cMessage("serveRequest");
        requestQueueSize_ = par("requestQueueSize");

        subscriptionServiceTime_ = par("subscriptionServiceTime");
        subscriptionService_ = new cMessage("serveSubscription");
        subscriptionQueueSize_ = par("subscriptionQueueSize");
        currentRequestMessageServed_ = nullptr;
        currentSubscriptionServed_ = nullptr;

        requestQueueSizeSignal_ = registerSignal("requestQueueSize");
        responseTimeSignal_ = registerSignal("responseTime");
        binder_ = getBinder();
        isMEOApp = getAncestorPar("isMEOapp").boolValue();
        processSubscriptionMessage = new cMessage("processSubscription");
    }

    else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        baseSubscriptionLocation_ = host_+ baseUriSubscriptions_ + "/";
        std::string mecOrchestratorHostname = getAncestorPar("mecOrchestratorHostname").stringValue();
        if(!isMEOApp)
            mecOrchestrator_ = check_and_cast<MecOrchestrator*>(getSimulation()->getModuleByPath(mecOrchestratorHostname.c_str()));
        else
            mecOrchestrator_ = check_and_cast<MecOrchestratorApp*>(getSimulation()->getModuleByPath(mecOrchestratorHostname.c_str())->getSubmodule("meoApp"));
    }

    inet::ApplicationBase::initialize(stage);
}

void UALCMPApp::handleStartOperation(inet::LifecycleOperation *operation)
{
    EV << "UALCMPApp::handleStartOperation" << endl;
    const char *localAddress = par("localAddress");
    int localPort = par("localPort");
    EV << "Local Address: " << localAddress << " port: " << localPort << endl;
    inet::L3Address localAdd(inet::L3AddressResolver().resolve(localAddress));
    EV << "Local Address resolved: "<< localAdd << endl;

    // e.g. 1.2.3.4:5050
    std::stringstream hostStream;
    hostStream << localAddress<< ":" << localPort;
    host_ = hostStream.str();

    serverSocket.setOutputGate(gate("socketOut"));
    serverSocket.setCallback(this);
    //serverSocket.bind(localAddress[0] ? L3Address(localAddress) : L3Address(), localPort);
    serverSocket.bind(inet::L3Address(), localPort); // bind socket for any address

    serverSocket.listen();
}

void UALCMPApp::handleMessageWhenUp(cMessage *msg)
{
    if(msg->arrivedOn("fromMecOrchestrator"))
    {
        // manage here the messages from mecOrchestrator
        UALCMPMessage * lcmMsg = check_and_cast<UALCMPMessage*>(msg);
        if(strcmp(lcmMsg->getType(), ACK_CREATE_CONTEXT_APP) == 0)
        {
            handleCreateContextAppAckMessage(lcmMsg);
        }
        else if(strcmp(lcmMsg->getType(), ACK_DELETE_CONTEXT_APP) == 0)
        {
            handleDeleteContextAppAckMessage(lcmMsg);
        }

        pendingRequests.erase(lcmMsg->getRequestId());
        delete msg;

        return;
    }
    else
    {
        if(!msg->isSelfMessage())
        {
            inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(amsSocketMap.findSocketFor(msg));
            if (socket)
            {
                socket->processMessage(msg);
                return;
            }

        }
        else if(msg->isSelfMessage() && strcmp(msg->getName(), "processSubscription") == 0)
        {

            AmsSubscription *sub = pendingSubscriptions_.front();
            inet::TcpSocket* amsSock = check_and_cast<inet::TcpSocket*>(amsSocketMap.getSocketById(sub->amsSockId));
            double delay = uniform(0.001, 0.004);
            if(amsSock->getState() == inet::TcpSocket::CONNECTED)
            {
                std::string host = amsSock->getRemoteAddress().str() + ":" +std::to_string(amsSock->getRemotePort());
                std::string webHook ="/amsWebHook_" + std::to_string(sub->ueSockId);
                inet::TcpSocket* ueSock = check_and_cast<inet::TcpSocket*>(socketMap.getSocketById(sub->ueSockId));


                nlohmann::ordered_json subscriptionBody_;
                subscriptionBody_ = nlohmann::ordered_json();
                subscriptionBody_["_links"]["self"]["href"] = "";
                subscriptionBody_["callbackReference"] = amsSock->getLocalAddress().str() + ":" + std::to_string(amsSock->getLocalPort()) + webHook;
                subscriptionBody_["requestTestNotification"] = false;
                subscriptionBody_["websockNotifConfig"]["websocketUri"] = "";
                subscriptionBody_["websockNotifConfig"]["requestWebsocketUri"] = false;
                subscriptionBody_["filterCriteria"]["appInstanceId"] = "";
                subscriptionBody_["filterCriteria"]["associateId"] = nlohmann::json::array();

                nlohmann::ordered_json val_;
                val_["type"] = "UE_IPv4_ADDRESS";
                val_["value"] = ueSock->getRemoteAddress().str();
                subscriptionBody_["filterCriteria"]["associateId"].push_back(val_);

                subscriptionBody_["filterCriteria"]["mobilityStatus"] = "INTERHOST_MOVEOUT_COMPLETED";
                subscriptionBody_["subscriptionType"] = "MobilityProcedureSubscription";

                EV << "UALCMP::subscribing for: " << subscriptionBody_ << endl;
                // sending the subscription
                Http::sendPostRequest(amsSock, subscriptionBody_.dump().c_str(), host.c_str(), subscriptionUri_.c_str());

                // removing from the queue
                pendingSubscriptions_.pop();
                delay = 0;
            }

            if(!processSubscriptionMessage->isScheduled() && !pendingSubscriptions_.empty())
            {
                scheduleAt(simTime()+delay, processSubscriptionMessage);
            }

            return;

        }

        MecServiceBase::handleMessageWhenUp(msg);
    }
}

void UALCMPApp::socketEstablished(inet::TcpSocket *socket)
{
    EV << "UALCMP::AMS socket established" << endl;
    // Creating an instance of the struct to manage ams related http message
    HttpMessageManager *messageStruct = new HttpMessageManager();
    httpmanager_[socket->getSocketId()] = messageStruct;
}


void UALCMPApp::handleCreateContextAppAckMessage(UALCMPMessage *msg)
{
    CreateContextAppAckMessage* ack = check_and_cast<CreateContextAppAckMessage*>(msg);
    unsigned int reqSno = ack->getRequestId();

    EV << "UALCMPApp::handleCreateContextAppAckMessage - reqSno: " << reqSno << endl;

    auto req = pendingRequests.find(reqSno);
    if(req == pendingRequests.end())
    {
        EV << "UALCMPApp::handleCreateContextAppAckMessage - reqSno: " << reqSno<< " not present in pendingRequests. Discarding... "<<endl;
        return;
    }

    int connId = req->second.connId;

    EV << "UALCMPApp::handleCreateContextAppAckMessage - reqSno: " << reqSno << " related to connid: "<< connId << endl;

    nlohmann::json jsonBody = req->second.appCont;

    inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(socketMap.getSocketById(connId));

    if(socket)
    {
        if(ack->getSuccess())
        {

            jsonBody["contextId"] = std::to_string(ack->getContextId());
            jsonBody["appInfo"]["userAppInstanceInfo"]["appInstanceId"] = ack->getAppInstanceId();
            jsonBody["appInfo"]["userAppInstanceInfo"]["referenceURI"]  = ack->getAppInstanceUri(); // add the end point

//            jsonBody["appInfo"]["userAppInstanceInfo"]["appLocation"]; // TODO not implemented yet
            std::stringstream uri;
            uri << baseUriQueries_<<"/app_contexts/"<< ack->getContextId();
            std::pair<std::string, std::string> locHeader("Location: ", uri.str());
            EV << "UALCMP::jsonbody response: " << jsonBody.dump() << "\nUALCMP dest: " << socket->getRemoteAddress() << endl;
            Http::send201Response(socket, jsonBody.dump().c_str(), locHeader);

            // Checking required services:
            if(std::strcmp(ack->getAmsUri(), "") != 0)
            {
                // Subscribing to the AMS for this application
                EV << "UALCMP::AMS Subscription required: " << ack->getAmsUri() << endl;
                char *uri = strdup(ack->getAmsUri());
                // No error checking
                inet::L3Address amsAddress = inet::L3AddressResolver().resolve(std::strtok(uri, ":"));
                int amsPort = std::atoi(std::strtok(NULL, ":"));

                int amsSockId = checkAmsSocket(amsAddress, amsPort);
                if(amsSocketMap.size() == 0 || amsSockId == -1)
                {
                    std::cout << "Socket not opened with: " << amsAddress << " " << amsPort << endl;
                    amsSockId = openAmsConnection(amsAddress, amsPort);
                }
                AmsSubscription *subscription = new AmsSubscription();
                subscription->ueSockId = socket->getSocketId(); // This is useful to notify the correct UE
                subscription->amsSockId = amsSockId;
//                subscription->postBody = subscriptionBody_;

                pendingSubscriptions_.push(subscription);

                if(!processSubscriptionMessage->isScheduled() &&
                        !pendingSubscriptions_.empty())
                {
                    scheduleAt(simTime(), processSubscriptionMessage);
                }
            }

        }
        else
        {
            Http::ProblemDetailBase pd;
            pd.type = "Request not succesfully completed";
            pd.title = "CreateContext request result";
            pd.detail = "the Mec system was not able to instantiate the mec application";
            pd.status = "500";
            Http::send500Response(socket, pd.toJson().dump().c_str());
        }

    }
    return;
}

void UALCMPApp::handleDeleteContextAppAckMessage(UALCMPMessage *msg)
{
    DeleteContextAppAckMessage* ack = check_and_cast<DeleteContextAppAckMessage*>(msg);

    unsigned int reqSno = ack->getRequestId();

    auto req = pendingRequests.find(reqSno);
    if(req == pendingRequests.end())
    {
        EV << "UALCMPApp::handleDeleteContextAppAckMessage - reqSno: " << reqSno<< " not present in pendingRequests. Discarding... "<<endl;
        return;
    }

    int connId = req->second.connId;
    EV << "UALCMPApp::handleDeleteContextAppAckMessage - reqSno: " << reqSno << " related to connid: "<< connId << endl;

    inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(socketMap.getSocketById(connId));

    if(socket)
    {
        if(ack->getSuccess() == true)
        {
            Http::send204Response(socket);

            auto sub = amsSubscriptions_.find(socket->getRemoteAddress().str());
            if(sub != amsSubscriptions_.end())
            {
                // Delete subscription from AMS
                EV << "UALCMP::sending delete subscription to AMS" << endl;
                inet::TcpSocket *amsSocket = check_and_cast<inet::TcpSocket *>(amsSocketMap.getSocketById(sub->second->amsSockId));
                std::string host = amsSocket->getRemoteAddress().str() + ":" + std::to_string(amsSocket->getRemotePort());

                Http::sendDeleteRequest(amsSocket, host.c_str(), sub->second->amsUri.c_str());
                amsSubscriptions_.erase(sub);
            }

        }
        else
        {
            Http::ProblemDetailBase pd;
            pd.type = "Request not succesfully completed";
            pd.title = "DeleteContext request result";
            pd.detail = "the Mec system was not able to terminate the mec application";
            pd.status = "500";
            Http::send500Response(socket, pd.toJson().dump().c_str());
        }

    }
    return;
}

void UALCMPApp::handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    EV << "UALCMPApp::handleGETRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();

    // check it is a GET for a query or a subscription
    if(uri.compare(baseUriQueries_+"/app_list") == 0 ) //queries
    {
        std::string params = currentRequestMessageServed->getParameters();
       //look for query parameters
        if(!params.empty())
        {
            std::vector<std::string> queryParameters = simu5g::utils::splitString(params, "&");
            /*
            * supported paramater:
            * - appName
            */

            std::vector<std::string> appNames;

            std::vector<std::string>::iterator it  = queryParameters.begin();
            std::vector<std::string>::iterator end = queryParameters.end();
            std::vector<std::string> params;
            std::vector<std::string> splittedParams;

            for(; it != end; ++it){
                if(it->rfind("appName", 0) == 0) // cell_id=par1,par2
                {
                    EV <<"UALCMPApp::handleGETRequest - parameters: " << endl;
                    params = simu5g::utils::splitString(*it, "=");
                    if(params.size()!= 2) //must be param=values
                    {
                        Http::send400Response(socket);
                        return;
                    }
                    splittedParams = simu5g::utils::splitString(params[1], ","); //it can an array, e.g param=v1,v2,v3
                    std::vector<std::string>::iterator pit  = splittedParams.begin();
                    std::vector<std::string>::iterator pend = splittedParams.end();
                    for(; pit != pend; ++pit){
                        EV << "appName: " <<*pit << endl;
                        appNames.push_back(*pit);
                    }
                }
                else // bad parameters
                {
                    Http::send400Response(socket);
                    return;
                }
            }

            nlohmann::ordered_json appList;

            // construct the result based on the appName vector
            for(auto appName : appNames)
            {
                const ApplicationDescriptor* appDesc = mecOrchestrator_->getApplicationDescriptorByAppName(appName);
                if(appDesc != nullptr)
                {
                    appList["appList"].push_back(appDesc->toAppInfo());
                }
            }
            // if the appList is empty, send an empty 200 response
            Http::send200Response(socket, appList.dump().c_str());
        }


        else { //no query params
            nlohmann::ordered_json appList;
            auto appDescs = mecOrchestrator_->getApplicationDescriptors();
            auto it = appDescs->begin();
            for(; it != appDescs->end() ; ++it)
            {
                appList["appList"].push_back(it->second.toAppInfo());
            }

            Http::send200Response(socket, appList.dump().c_str());
        }
    }

    else //bad uri
    {
        Http::send404Response(socket);

    }

}

void UALCMPApp::handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    std::string uri = currentRequestMessageServed->getUri();
    std::string body = currentRequestMessageServed->getBody();
    EV << "UALCMPApp::handlePOSTRequest - uri: "<< uri << endl;

    // it has to be managed the case when the sub is /area/circle (it has two slashes)
    if(uri.compare(baseUriSubscriptions_+"/app_contexts") == 0)
    {
        nlohmann::json jsonBody;
        try
        {
            jsonBody = nlohmann::json::parse(body); // get the JSON structure
        }
        catch(nlohmann::detail::parse_error e)
        {
            throw cRuntimeError("UALCMPApp::handlePOSTRequest - %s", e.what());
            // body is not correctly formatted in JSON, manage it
            Http::send400Response(socket); // bad body JSON
            return;
        }

        // Parse the JSON  body to organize the App instantion
        CreateContextAppMessage * createContext = parseContextCreateRequest(jsonBody);
        if(createContext != nullptr)
        {
            createContext->setType(CREATE_CONTEXT_APP);
            createContext->setRequestId(requestSno);
            createContext->setConnectionId(socket->getSocketId());
            createContext->setAppContext(jsonBody);

            createContext->setUeIpAddress(socket->getRemoteAddress().str().c_str());
            pendingRequests[requestSno] = {socket->getSocketId(), requestSno, jsonBody};

            EV << "POST request number: " << requestSno << " related to connId: " << socket->getSocketId() << endl;

            requestSno++;

            send(createContext, "toMecOrchestrator");
        }
        else
        {
            Http::send400Response(socket); // bad body JSON
        }
    }
    else
    {
        Http::send404Response(socket); //
    }
}

void UALCMPApp::handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket){
    Http::send404Response(socket, "PUT not implemented, yet");
}

void UALCMPApp::handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    //    DELETE /exampleAPI/location/v1/subscriptions/area/circle/sub123 HTTP/1.1
    //    Accept: application/xml
    //    Host: example.com

    EV << "UALCMP::handleDELETERequest" << endl;
    // uri must be in form /example/dev_app/v1/app_context/contextId
    std::string uri = currentRequestMessageServed->getUri();

//    // it has to be managed the case when the sub is /area/circle (it has two slashes)

    std::size_t lastPart = uri.find_last_of("/"); // split at contextId
    std::string baseUri = uri.substr(0,lastPart); // uri
    std::string contextId =  uri.substr(lastPart+1); // contextId

    if(baseUri.compare(baseUriSubscriptions_+"/app_contexts") == 0)
    {
        DeleteContextAppMessage * deleteContext = new DeleteContextAppMessage();
        deleteContext->setRequestId(requestSno);
        deleteContext->setType(DELETE_CONTEXT_APP);
        deleteContext->setContextId(stoi(contextId));

        pendingRequests[requestSno].connId = socket->getSocketId();
        pendingRequests[requestSno].requestId = requestSno;
        EV << "DELETE request number: " << requestSno << " related to connId: " << socket->getSocketId() << endl;

        requestSno++;

        send(deleteContext, "toMecOrchestrator");
    }
    else
    {
        Http::send404Response(socket);
    }

}

CreateContextAppMessage* UALCMPApp::parseContextCreateRequest(const nlohmann::json& jsonBody)
{
    if(!jsonBody.contains("appInfo"))
    {
        EV << "UALCMPApp::parseContextCreateRequest - appInfo attribute not found" << endl;
        return nullptr;
    }

    nlohmann::json appInfo = jsonBody["appInfo"];

    if(!jsonBody.contains("associateDevAppId"))
    {
       EV << "UALCMPApp::parseContextCreateRequest - associateDevAppId attribute not found" << endl;
       return nullptr;
    }

    std::string devAppId = jsonBody["associateDevAppId"];

    // the mec app package is already onboarded (from the device application pov)
    if(appInfo.contains("appDId"))
    {
        CreateContextAppMessage* createContext = new CreateContextAppMessage();
        createContext->setOnboarded(true);
        createContext->setDevAppId(devAppId.c_str());
        std::string appDId = appInfo["appDId"];
        createContext->setAppDId(appDId.c_str());
        return  createContext;
    }

    // the mec app package is not onboarded, but the uri of the app package is provided
    else if (appInfo.contains("appPackageSource"))
    {
        CreateContextAppMessage* createContext = new CreateContextAppMessage();
        createContext->setOnboarded(false);
        createContext->setDevAppId(devAppId.c_str());
        std::string appPackageSource = appInfo["appPackageSource"];
        createContext->setAppPackagePath(appPackageSource.c_str());
        return  createContext;
    }
    else
    {
    // neither the two is present and this is not allowed
        return nullptr;
    }
}

void UALCMPApp::finish()
{
    return;
}

UALCMPApp::~UALCMPApp(){
    cancelAndDelete(processSubscriptionMessage);
    while(!pendingSubscriptions_.empty()) {pendingSubscriptions_.pop();}

    for(auto &it : httpmanager_)
    {
        while(!it.second->completedMessageQueue.isEmpty())
            it.second->completedMessageQueue.pop();
    }

    httpmanager_.clear();
}

int UALCMPApp::openAmsConnection(inet::L3Address amsAddress, int amsPort)
{
    EV << "UALCMP::MEC App requires migration support - connecting to the AMS " << amsAddress.str() << " " << amsPort  << endl;
    inet::TcpSocket *sock = new inet::TcpSocket();
    sock->setOutputGate(gate("socketOut"));
    sock->setCallback(this);
//    sock->bind(localPort);

    // we need a new connId if this is not the first connection
    sock->renewSocket();

    if (sock->getState() == inet::TcpSocket::CONNECTED) {
        EV_ERROR << "Already connected to " << amsAddress.str() << " port=" << amsPort << endl;
    }
    else {
        sock->connect(amsAddress, amsPort);
        amsSocketMap.addSocket(sock);
        return sock->getSocketId();
    }

    return -1;
}

void UALCMPApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent)
{
    EV << "UALCMP::socket data arrived" << endl;

    std::vector<uint8_t> bytes =  packet->peekDataAsBytes()->getBytes();
    std::string msg(bytes.begin(), bytes.end());

    if(checkAmsSocket(socket->getRemoteAddress(), socket->getRemotePort()) != -1)
    {
        EV << "UALCMP::AMS valid socket" << endl;
        auto messageManager = httpmanager_.find(socket->getSocketId());
        if(messageManager == httpmanager_.end())
        {
            throw cRuntimeError("UALCMP::socketDataArrived - httpmessagemanager not found!");
        }

        bool res = Http::parseReceivedMsg(socket->getSocketId(), msg, messageManager->second->completedMessageQueue, &messageManager->second->bufferedData, &messageManager->second->httpMessage);
        if(res)
        {
            while(!messageManager->second->completedMessageQueue.isEmpty())
            {
                amsHttpCompleteMessage = check_and_cast<HttpBaseMessage*>(messageManager->second->completedMessageQueue.pop());

                if(amsHttpCompleteMessage->getType() == REQUEST)
                {
                    HttpRequestMessage *request = check_and_cast<HttpRequestMessage*>(amsHttpCompleteMessage);
                    EV << "UALCMP::socketDataArrived - Received notification - payload: " << " " << amsHttpCompleteMessage->getBody() << endl;
                    char * uri = strdup(request->getUri());

                    EV << "UALCMP::received notification for: " << uri << endl;

                    nlohmann::json jsonBody = nlohmann::json::parse(request->getBody());
                    std::strtok(uri, "_");
                    int ueSockId = std::atoi(std::strtok(NULL, "_"));

                    inet::TcpSocket *ueSock = check_and_cast<inet::TcpSocket*>(socketMap.getSocketById(ueSockId));
                    std::string host = ueSock->getRemoteAddress().str()+":"+std::to_string(ueSock->getRemotePort());
                    std::string uristring = "/example/deviceApp/v1/notification/";

                    if(jsonBody["notificationType"]=="MobilityProcedureNotification")
                    {
                        AddressChangeNotification *notification = new AddressChangeNotification();
                        notification->setAppInstanceId(jsonBody["targetAppInfo"]["appInstanceId"]);

                        // TODO How do we get the context id?
                        notification->setContextId("");
//
//                        // so far only one interface is supported
                        nlohmann::json interfaces = nlohmann::json::array();
                        interfaces = jsonBody["targetAppInfo"]["commInterface"]["ipAddresses"];
                        int port = interfaces.at(1)["port"];
                        std::string referenceUri = std::string(interfaces.at(1)["host"]) + ":" + std::to_string(port);
                        notification->setReferenceUri(referenceUri);
//
                        // sending notification to DeviceApp
                        Http::sendPostRequest(ueSock, notification->toJson().dump().c_str(), host.c_str(), uristring.c_str());

                    }


                }else if(amsHttpCompleteMessage->getType() == RESPONSE){
                   EV << "UALCMP::socketDataArrived - Received response - payload: " << " " << amsHttpCompleteMessage->getBody() << endl;
                   HttpResponseMessage* amsResponse = check_and_cast<HttpResponseMessage*>(amsHttpCompleteMessage);
                   // TODO CODE CHECKING
                   if(amsResponse->getCode() == 201)
                   {
                        nlohmann::json jsonBody = nlohmann::json::parse(amsHttpCompleteMessage->getBody());
                        std::string ueAddress = jsonBody["filterCriteria"]["associateId"][0]["value"];
                        std::string subUri = jsonBody["_links"]["self"]["href"];
                        AmsSubscription *sub = new AmsSubscription();
                        sub->amsSockId = socket->getSocketId();
                        sub->amsUri = subUri;
                        EV << "UALCMP::AMS subscription for MEC App requested by " << ueAddress << " completed - " << subUri << " -";

                        // keeping track of the subscriptions
                        amsSubscriptions_.insert({ueAddress, sub});
                   }
                   else if(amsResponse->getCode() == 204)
                   {
                       // Delete response
                       EV << "UALCMP::Ams - ue subscription deleted" << endl;

                   }
                   else
                   {
                       EV_ERROR << "UALCMP::Unexpected Error Code: " << amsResponse->getCode() << endl;
                   }

                }

            }

        }
    }
    else
    {
        delete packet;
        throw cRuntimeError("UALCMP::socketDataArrived - Socket %d not recognized", socket->getSocketId());
    }

    delete packet;
    return;

}


int UALCMPApp::checkAmsSocket(inet::L3Address amsAddress, int amsPort)
{
    for(auto it : amsSocketMap.getMap())
    {

        inet::TcpSocket *sock = check_and_cast<inet::TcpSocket *>(it.second);
        if(sock->getRemoteAddress() == amsAddress && sock->getRemotePort() == amsPort)
        {
            return sock->getSocketId();
        }
    }
    return -1;
}

