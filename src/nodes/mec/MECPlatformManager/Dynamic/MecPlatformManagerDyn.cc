//
//  MecPlatformManagerDyn.cc
//
//  @author Angelo Feraudo
//  @author Alessandro Calvio
//

// Imports
#include <sstream>

#include "nodes/mec/MECPlatformManager/Dynamic/MecPlatformManagerDyn.h"
#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h" //ServiceInfo struct


Define_Module(MecPlatformManagerDyn);

MecPlatformManagerDyn::MecPlatformManagerDyn()
{
    vim = nullptr;
    serviceRegistry = nullptr;
    amsEnabled = false;
}

MecPlatformManagerDyn::~MecPlatformManagerDyn()
{
    // TODO
    cancelAndDelete(registerMessage_);
}

void MecPlatformManagerDyn::initialize(int stage)
{

    if (stage == inet::INITSTAGE_LOCAL) {
        EV << "MecPlatformManagerDyn::initializing..."<<endl;
        // Get other modules
        if(getParentModule()->getParentModule()->getSubmodule("vim")->getSubmodule("vimApp") == nullptr)
            vim = check_and_cast<VirtualisationInfrastructureManagerDyn*>(getParentModule()->getParentModule()->getSubmodule("vim")->getSubmodule("app", 0));
        else
            vim = check_and_cast<VirtualisationInfrastructureManagerDyn*>(getParentModule()->getParentModule()->getSubmodule("vim")->getSubmodule("vimApp"));

        cModule* mecPlatform = getParentModule()->getParentModule()->getSubmodule("mecPlatform");
        if(mecPlatform != nullptr && mecPlatform->findSubmodule("serviceRegistry") != -1)
        {
            serviceRegistry = check_and_cast<ServiceRegistry*>(mecPlatform->getSubmodule("serviceRegistry"));
        }else{
            EV << "MecPlatformManagerDyn::initialize - unable to find mecPlatform or serviceRegistry" << endl;
        }

        meoPort = par("meoPort");
        localPort = par("localPort");
        vimPort = par("vimPort");

        //Broker settings
        brokerPort = par("brokerPort");
        localToBrokerPort = localPort;

        subscribeURI = std::string(par("subscribeURI").stringValue());
        unsubscribeURI = subscribeURI + std::to_string(getId());
        webHook = std::string(par("webHook").stringValue());

        //triggeruri
        triggerURI = std::string(par("triggerURI").stringValue());

    }

    inet::ApplicationBase::initialize(stage);


}

void MecPlatformManagerDyn::handleStartOperation(inet::LifecycleOperation *operation)
{
    // Read parameters
    meoAddress = inet::L3AddressResolver().resolve(par("meoAddress"));
    localAddress = inet::L3AddressResolver().resolve(par("localAddress"));
    vimAddress = inet::L3AddressResolver().resolve(par("vimAddress"));
    // Setup orchestrator communication
    if(localPort != -1){
        EV << "MecPlatformManagerDyn::initialize - binding orchestrator socket to port " << localPort << endl;
        socket.setOutputGate(gate("socketOut"));
        socket.bind(localPort);
    }

    // Broker settings - ams address
    brokerIPAddress = inet::L3AddressResolver().resolve(par("brokerAddress").stringValue());

    registerMessage_ = new cMessage("register");
    scheduleAt(simTime()+0.01, registerMessage_);
    SubscriberBase::handleStartOperation(operation);
}

void MecPlatformManagerDyn::handleMessageWhenUp(cMessage *msg)
{
    EV << "MecPlatformManagerDyn::handleMessage - message received - " << msg << endl;


    if(msg->isSelfMessage() && strcmp(msg->getName(), "register") == 0)
    {
        // Register to MECOrchestrator
        std::cout << "MecPlatformManagerDyn::handleMessage - register to MEO " << msg << endl;
        sendMEORegistration();

        //Check Ams availability
        // TODO amsname should be a parameters
        amsEnabled = checkServiceAvailability("ApplicationMobilityService");
        EV << "MecPlatformManagerDyn::ams is enabled? " << amsEnabled << endl;
        if(amsEnabled)
        {
            connectToBroker();
        }
//        delete msg;
    }else if (!msg->isSelfMessage() && socket.belongsToSocket(msg))
    {
        EV << "MecPlatformManagerDyn::handleMessage - TYPE: "<< msg->getName() << endl;
        if(!strcmp(msg->getName(), "ServiceRequest")){
                inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
                handleServiceRequest(pPacket);
//                delete msg;
        }
        else if (!strcmp(msg->getName(), "instantiationApplicationRequest")){
            inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
            handleInstantiationRequest(pPacket);
    //            delete msg;
        }
        else if (!strcmp(msg->getName(), "instantiationApplicationResponse")){
            inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
            handleInstantiationResponse(pPacket);

    //            delete msg;
        }
        else if(!strcmp(msg->getName(), "terminationAppInstRequest"))
        {
            inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
            handleTerminationRequest(packet);
        }
        else if(!strcmp(msg->getName(), "terminationAppInstResponse"))
        {
            inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
            handleTerminationResponse(packet);
        }
        else if(!strcmp(msg->getName(), "ParkMigrationTrigger"))
        {
            inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
            handleParkMigrationTrigger(packet);
//            delete msg;
        }
        else if(!strcmp(msg->getName(), "ServiceMobilityResponse"))
        {
            inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
            handleServiceMobilityResponse(packet);
//            delete msg;
        }
        else{
            EV << "MecPlatformManagerDyn::handleMessage - unknown package" << endl;
        }
        delete msg;
    }
    else
    {
        EV << "MecPlatformManagerDyn::handleMessage - TCP message received!" << endl;
        SubscriberBase::handleMessageWhenUp(msg);
    }
//        std::cout << "IP received " << pPacket->getTag<inet::L3AddressInd>()->getSrcAddress() << endl;
//        std::cout << "NAME received " << msg->getName() << endl;

}


MecAppInstanceInfo* MecPlatformManagerDyn::instantiateMEApp(InstantiationApplicationRequest* msg)
{
    EV << "MecPlatformManagerDyn::instantiateMEApp" << endl;
    MecAppInstanceInfo* res = vim->instantiateMEApp(msg);
    delete msg;
    return res;
}

bool MecPlatformManagerDyn::instantiateEmulatedMEApp(CreateAppMessage* msg)
{
    EV << "MecPlatformManagerDyn::instantiateEmulatedMEApp" << endl;
    bool res = vim->instantiateEmulatedMEApp(msg);
    delete msg;
    return res;
}

bool MecPlatformManagerDyn::terminateEmulatedMEApp(DeleteAppMessage* msg)
{
    EV << "MecPlatformManagerDyn::terminateEmulatedMEApp" << endl;
    bool res = vim->terminateEmulatedMEApp(msg);
    delete msg;
    return res;
}


bool MecPlatformManagerDyn::terminateMEApp(DeleteAppMessage* msg)
{
//    EV << "MecPlatformManagerDyn::terminateMEApp" << endl;
//    bool res = vim->terminateMEApp(msg);
//    delete msg;
//    return res;
    return false;
}

const std::vector<ServiceInfo>* MecPlatformManagerDyn::getAvailableMecServices() const
{
    EV << "MecPlatformManagerDyn::getAvailableMecServices" << endl;
    if(serviceRegistry == nullptr)
        return nullptr;
    else
    {
       return serviceRegistry->getAvailableMecServices();
    }
}


void MecPlatformManagerDyn::registerMecService(ServiceDescriptor& servDescriptor) const
{
    EV << "MecPlatformManagerDyn::registerMecService" << endl;
    serviceRegistry->registerMecService(servDescriptor); //TODO Substitute with MecOrchestrator registring (?)
}



void MecPlatformManagerDyn::sendMEORegistration()
{
    inet::Packet* packet = new inet::Packet("Registration");
    auto registrationPkt = inet::makeShared<RegistrationPkt>();
    registrationPkt->setHostId(getParentModule()->getParentModule()->getId());
    registrationPkt->setType(MM3);
    registrationPkt->setSourcePort(localPort);
    registrationPkt->setChunkLength(inet::B(1000));
    packet->insertAtBack(registrationPkt);

    EV << "MecPlatformManagerDyn::sending custom packet to MEO " << endl;
    std::cout << "MecPlatformManagerDyn::sending custom packet to MEO " << endl;
    std::cout << "MecPlatformManagerDyn::sending custom packet to MEO " << meoAddress.str() << endl;
    std::cout << "MecPlatformManagerDyn::sending custom packet to MEO " << meoPort << endl;
    std::cout << "MecPlatformManagerDyn::sending custom packet to MEO " << packet->getFullName()<< endl;
    inet::PacketPrinter printer;
    printer.printPacket(std::cout, packet);
    socket.sendTo(packet, meoAddress, meoPort);

}

void MecPlatformManagerDyn::handleServiceRequest(inet::Packet* resourcePacket){

    auto data = resourcePacket->peekData<MeoMepmRequest>();
    const MeoMepmRequest *receivedData = data.get();

    const std::vector<ServiceInfo>* services = serviceRegistry->getAvailableMecServices();
    bool res = true;
    for(int i = 0; i < receivedData->getRequiredServiceNamesArraySize() && res; i++)
    {
        const char* serviceName = receivedData->getRequiredServiceNames(i);
        bool found = checkServiceAvailability(serviceName);
//        for(int j = 0; j < services->size() && !found; j++)
//        {
//            const char* availableServiceName = services->at(j).getName().c_str();
//            if(std::strcmp(availableServiceName, serviceName)==0){
//                found = true;
//            }
//
//        }

        res = res && found;
    }

    inet::Packet* packet = new inet::Packet("AvailabilityResponseMepm");
    auto responsePkt = inet::makeShared<MecHostResponse>();
    responsePkt->setDeviceAppId(receivedData->getDeviceAppId());
    responsePkt->setMecHostId(getParentModule()->getParentModule()->getId());
    responsePkt->setResult(res);
    responsePkt->setChunkLength(inet::B(1000));
    packet->insertAtBack(responsePkt);
    socket.sendTo(packet, meoAddress, meoPort);

    EV << "MecPlatformManagerDyn::handleResourceRequest - request result:  " << res << endl;
}

void MecPlatformManagerDyn::handleInstantiationRequest(inet::Packet* instantiationPacket){

    inet::Packet* pktdup = new inet::Packet("instantiationApplicationRequest");
    auto instAppRequest = inet::makeShared<InstantiationApplicationRequest>();
    auto data = instantiationPacket->peekData<InstantiationApplicationRequest>();
    instAppRequest = data.get()->dup();
    pktdup->insertAtBack(instAppRequest);

    // Choosing interface for communications inside the MEC Host
    //pktdup->addTag<inet::InterfaceReq>()->setInterfaceId(ifacetable->findInterfaceByName("pppIfRouter")->getInterfaceId());

    EV << "MecPlatformManagerDyn::handleResourceRequest - sending:  " << vimAddress << ":" << vimPort << endl;
    socket.sendTo(pktdup, vimAddress, vimPort);
}

void MecPlatformManagerDyn::handleTerminationRequest(inet::Packet *packet)
{

    inet::Packet* pktdup = new inet::Packet("terminationAppInstRequest");
    auto deleteAppRequest = inet::makeShared<TerminationAppInstRequest>();
    auto data = packet->peekData<TerminationAppInstRequest>();
    deleteAppRequest = data.get()->dup();
    pktdup->insertAtBack(deleteAppRequest);

    // Choosing interface for communications inside the MEC Host
    //pktdup->addTag<inet::InterfaceReq>()->setInterfaceId(ifacetable->findInterfaceByName("pppIfRouter")->getInterfaceId());

    EV << "MecPlatformManagerDyn::handleTerminationRequest - sending:  " << vimAddress << ":" << vimPort << endl;
    socket.sendTo(pktdup, vimAddress, vimPort);

}

void MecPlatformManagerDyn::handleTerminationResponse(inet::Packet * packet)
{
    auto data = packet->peekData<TerminationAppInstResponse>().get();
    if(data->isMigrating())
    {
        EV << "MecPlatformManagerDyn::handleTerminationResponse - app of " << data->getDeviceAppId() << " migrated (nothing to do)" << endl;

        auto it = links_.find(std::string(data->getAppInstanceId()));
        unsubscribeURI = it->second->getHref();
        EV << "MecPlatformManagerDyn::handleTerminationResponse unsub uri:  " << unsubscribeURI << endl;

        // unsubscribing for old apps
        unsubscribe();
        links_.erase(it);

        // TODO add multiple unsubscription - COMPLETED

        // Subscribing for new Apps
        handleSubscription(std::string(data->getAppInstanceId()));
        appState = SUB;
        return;
    }
    inet::Packet* pktdup = new inet::Packet("terminationAppInstResponse");
    auto deleteAppResponse = inet::makeShared<TerminationAppInstResponse>();
    deleteAppResponse = data->dup();
    pktdup->insertAtBack(deleteAppResponse);

    // Choosing interface for communications inside the MEC Host

    EV << "MecPlatformManagerDyn::handleTerminationResponse - sending:  " << meoAddress << ":" << meoPort << endl;
    socket.sendTo(pktdup, meoAddress, meoPort);
}

void MecPlatformManagerDyn::handleInstantiationResponse(
        inet::Packet *instantiationPacket) {
    EV << "MecPlatformManagerDyn::handleInstantiationResponse" << endl;
    inet::Packet* pktdup = new inet::Packet("instantiationApplicationResponse");
    auto responsemsg = inet::makeShared<InstantiationApplicationResponse>();
    auto data = instantiationPacket->peekData<InstantiationApplicationResponse>();
    responsemsg = data.get()->dup();
    pktdup->insertAtBack(responsemsg);

    socket.sendTo(pktdup, meoAddress, meoPort);
    EV << "MecPlatformManagerDyn:: App instance id: " << responsemsg->getInstanceId()<< endl;
    if(amsEnabled && responsemsg->getStatus())
    {
        handleSubscription(responsemsg->getInstanceId());
    }

}

bool MecPlatformManagerDyn::checkServiceAvailability(const char *serviceName)
{
    const std::vector<ServiceInfo>* services = serviceRegistry->getAvailableMecServices();
    bool found = false;
    for(int j = 0; j < services->size() && !found; j++)
    {
        const char* availableServiceName = services->at(j).getName().c_str();
        if(std::strcmp(availableServiceName, serviceName)==0){
            found = true;
        }

    }
    return found;
}

void MecPlatformManagerDyn::manageNotification()
{
    /*
     * This methods manage both RESPONSE and REQUESTS
     * - responses are not actually notification, but notify the correct or failed registration to the broker (FIXME?)
     */
    if(currentHttpMessageServed_->getType() == RESPONSE)
    {
        EV << "MecPlatformManagerDyn::manageNotification - subscription" << endl;
        HttpResponseMessage *response = check_and_cast<HttpResponseMessage*>(currentHttpMessageServed_);
//        nlohmann::ordered_json jsonBody = nlohmann::json::parse(currentHttpMessageServed_->getBody());
        if(response->getCode() == 201)
        {
            nlohmann::ordered_json jsonBody = nlohmann::json::parse(currentHttpMessageServed_->getBody());
            MobilityProcedureSubscription subscription;
            subscription.fromJson(jsonBody);
            LinkType *link = new LinkType();
            link->setHref(subscription.getLinks());
            links_[subscription.getFilterCriteria()->getAppInstanceId()] = link;
            EV << "MecPlatformManagerDyn::registered for " << subscription.getFilterCriteria()->getAppInstanceId() << endl;
            // test that subscription is really ok
            //Http::sendGetRequest(&tcpSocket, serverHost.c_str(), subscription.getLinks().c_str());
        }
        else if(response->getCode() == 204)
        {
            EV << "MecPlatformManagerDyn::received reponse with code 204 (probably unsub)" << endl;
        }

    }
    else
    {

        EV << "MecPlatformManagerDyn::manageNotification - notification" << endl;
        // This is a request, so notification
        HttpRequestMessage *request = check_and_cast<HttpRequestMessage*>(currentHttpMessageServed_);
        nlohmann::ordered_json jsonBody = nlohmann::json::parse(request->getBody());

        EV << "MecPlatformManagerDyn::notificationType: " << jsonBody["notificationType"] << endl;
        if(jsonBody.contains("notificationType") && (jsonBody["notificationType"] == "MobilityProcedureNotification"
                || jsonBody["notificationType"] == "AdjacentAppNotification"))
        {
            // TODO we can ignore packets that contains targetappinfo
            if(jsonBody["notificationType"] == "MobilityProcedureNotification")
            {
                EV << "MecPlatformManagerDyn::MobilityProcedureNotification - " << jsonBody["mobilityStatus"] << endl;
                if(jsonBody["mobilityStatus"] == "INTERHOST_MOVEOUT_TRIGGERED")
                {
                    EV << "MecPlatformManagerDyn::notification with mobilityStatus:  INTERHOST_MOVEOUT_TRIGGERED" << endl;
                    // MobilityProcedureNotification
                    MobilityProcedureNotification *notification = new MobilityProcedureNotification();
                    EV << "MecPlatformManagerDyn::notification received: " << jsonBody.dump(2) << endl;
                    bool res = notification->fromJson(jsonBody);
                    bool condition = res && notification->getTargetAppInfo().getCommInterface().size() == 0; // event with targetAppInfo are ignored
                    if(condition)
                    {

                        inet::L3Address destinationAddr;
                        int destPort;
                        int packetLength = 0;

                        // Preparing ServiceMobilityRequest
                        inet::Packet* packet = new inet::Packet("ServiceMobilityRequest");

                        auto toSend = inet::makeShared<ServiceMobilityRequest>();
                        toSend->setAssociateIdArraySize(notification->getAssociateId().size());

                        // the index is needed to populate the other packet array
                        // so we use the old fashioned way
                        for(int i = 0; i < notification->getAssociateId().size(); i++)
                        {
                            toSend->setAssociateId(i, notification->getAssociateId()[i]);
                            // Computing packetLength
                            packetLength = packetLength + notification->getAssociateId()[i].getType().size();
                            packetLength = packetLength + notification->getAssociateId()[i].getType().size();
                        }

                        // Selecting migration type
                        if(jsonBody.contains("appInstanceId")) // -- dynamic resource migration (from dynamic resources to local resources)
                        {
                            EV << "MecPlatformManagerDyn::moving app " << notification->getAppInstanceId() << " from a dynamic to local resources" << endl;
                            // Sending Application Mobility Request to VIM
                            destinationAddr = vimAddress;
                            destPort = vimPort;
                            std::string appInstanceId = jsonBody["appInstanceId"];
                            toSend->setAppInstanceId(appInstanceId.c_str());
                            std::cout << "MecPlatformManagerDyn:: appInstanceId " << appInstanceId << endl;
                            packetLength = packetLength + appInstanceId.size();
                        }
                        else // -- global migration (from mechost to mechost)
                        {
                            EV << "MecPlatformManagerDyn::request app migration to MEC Orchestrator" << endl;
                            // Do we need app related information?
                            // in such a case we should request to the ams our request by using
                            // the _links field in the notification
                            // Sending Application Mobility Request to MEO
                            destinationAddr = meoAddress;
                            destPort = meoPort;
                        }

                        EV << "MecPlatformManagerDyn::ServiceMobilityRequest built! total packet length: " << packetLength << endl;

                        toSend->setChunkLength(inet::B(packetLength));
                        packet->insertAtBack(toSend);
                        EV << "MecPlatformManagerDyn::Sending serviceMobilityRequest to " << destinationAddr.str() << ":"<<std::to_string(destPort)<<endl;
                        socket.sendTo(packet, destinationAddr, destPort);
                    }
                    else
                    {
                        EV << "MecPlatformManagerDyn::notification not recognised" << endl;
                        }
                }
                else if(jsonBody["mobilityStatus"] == "INTERHOST_MOVEOUT_COMPLETED")
                {
                    EV << "MecPlatformManagerDyn::notification with mobilityStatus:  INTERHOST_MOVEOUT_COMPLETED" << endl;
                    // Generate Termination message
                    inet::Packet* pkt = new inet::Packet("terminationAppInstRequest");
                    auto deleteAppRequest = inet::makeShared<TerminationAppInstRequest>();


                    // where can i get ueappid?
                    // you can take it from the address
                    MobilityProcedureNotification *notification = new MobilityProcedureNotification();
                    EV << "MecPlatformManagerDyn::notification received: " << jsonBody.dump(2) << endl;
                    notification->fromJson(jsonBody);
                    std::string appInstanceId = notification->getTargetAppInfo().getAppInstanceId();
                    deleteAppRequest->setAppInstanceId(appInstanceId.c_str());
                    deleteAppRequest->setDeviceAppId("-1");

//                    inet::L3Address ipAddress = inet::L3AddressResolver().resolve(notification->getAssociateId()[0].getValue().c_str());
//                    cModule *ue = inet::L3AddressResolver().findHostWithAddress(ipAddress);
//                    EV << "MecPlatformManagerDyn::UE id: " << ue->getId() << " node id : "<< endl;

                    deleteAppRequest->setChunkLength(inet::B(appInstanceId.size() + 8));

                    pkt->insertAtBack(deleteAppRequest);
                    socket.sendTo(pkt, vimAddress, vimPort);
                }

            }
            else
            {
                // AdjacentAppNotification
                EV << "TO be defined" << endl;
            }
        }
        else
        {
            EV << "MecPlatformManagerDyn::request not recognised" << endl;
        }


    }
}

void MecPlatformManagerDyn::handleServiceMobilityResponse(
        inet::Packet *packet) {

    //generate a notification with targetappinfo

    auto data = packet->peekData<ServiceMobilityResponse>().get();

    // target data
    TargetAppInfo *targetInfo = new TargetAppInfo();
    targetInfo->setAppInstanceId(std::string(data->getAppInstanceId()));
    SockAddr targetAddress;
    targetAddress.addr = data->getTargetAddress();
    targetAddress.port = data->getTargetPort();

    std::vector<SockAddr> target;
    target.push_back({data->getTargetAddress(), data->getTargetPort()});
    target.push_back({data->getTargetAddress(), data->getTargetUePort()});

//    targetInfo->setCommInterface(std::vector<SockAddr>(1,targetAddress));
    targetInfo->setCommInterface(target);

    // AssociateId
    std::vector<AssociateId> associateId;
    for(int i = 0; i < data->getAssociateIdArraySize(); i++)
    {
        associateId.push_back(data->getAssociateId(i));
    }

    // Creating a MobilityProcedureNotification with targetAppInfo information
    MobilityProcedureNotification *notification = new MobilityProcedureNotification();
    notification->setMobilityStatus(INTERHOST_MOVEOUT_TRIGGERED);
    notification->setTargetAppInfo(*targetInfo);
    notification->setAssociateId(associateId);

    EV << "MecPlatformManagerDyn::Received service mobility response json object: " << notification->toJson().dump()<< endl;

    // Exploiting socket used for subscribing phase
    std::cout << "SENDING NOTIFICATION - MY HOST (Service mobilityResponse) " << serverHost.c_str() << " for  " << data->getAppInstanceId() << " at " << simTime() << endl;
    Http::sendPostRequest(&tcpSocket, notification->toJson().dump().c_str(), serverHost.c_str(), triggerURI.c_str());

}

void MecPlatformManagerDyn::handleParkMigrationTrigger(inet::Packet* packet)
{

    auto data = packet->peekData<ParkMigrationTrigger>().get();
    nlohmann::ordered_json request;
    request["notificationType"] = "MobilityProcedureNotification";
    request["associateId"] = nlohmann::json::array();

    request["mobilityStatus"] = "INTERHOST_MOVEOUT_TRIGGERED";
    request["_links"]["href"] = "";

    request["appInstanceId"] = data->getAppInstanceId();

    EV << "MecPlatformManagerDyn::Trigger ready: " << request.dump() << endl;
    std::cout << "SENDING NOTIFICATION - MY HOST (handleParkMigrationTrigger) " << serverHost.c_str() << " for app " << data->getAppInstanceId() << endl;

    Http::sendPostRequest(&tcpSocket, request.dump().c_str(),  serverHost.c_str(), triggerURI.c_str());
}

void MecPlatformManagerDyn::handleSubscription(
       std::string appInstanceId) {


    subscriptionBody_ = nlohmann::ordered_json();
    subscriptionBody_["_links"]["self"]["href"] = "";
    subscriptionBody_["callbackReference"] = localAddress.str() + ":" + std::to_string(localToBrokerPort)  + webHook;
    subscriptionBody_["requestTestNotification"] = false;
    subscriptionBody_["websockNotifConfig"]["websocketUri"] = "";
    subscriptionBody_["websockNotifConfig"]["requestWebsocketUri"] = false;
    subscriptionBody_["filterCriteria"]["appInstanceId"] = appInstanceId;
    subscriptionBody_["filterCriteria"]["associateId"] = nlohmann::json::array();
    subscriptionBody_["filterCriteria"]["mobilityStatus"] = nlohmann::json::array();
    subscriptionBody_["filterCriteria"]["mobilityStatus"].push_back("INTERHOST_MOVEOUT_TRIGGERED");
    subscriptionBody_["filterCriteria"]["mobilityStatus"].push_back("INTERHOST_MOVEOUT_COMPLETED");
    subscriptionBody_["subscriptionType"] = "MobilityProcedureSubscription";
    EV << subscriptionBody_;
    sendSubscription();

}

