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

/**
 * Authors:
 * Alessandro Calvio
 * Angelo Feraudo
 */

#include "../MEO/MecOrchestratorApp.h"

#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h" // TODO add this messages to our list
#include "inet/networklayer/common/L3AddressTag_m.h"

Define_Module(MecOrchestratorApp);

MecOrchestratorApp::MecOrchestratorApp()
{
    mecHosts.clear();
    responseMap.clear();
    mecApplicationDescriptors_.clear();
    contextIdCounter = 0;
}

MecOrchestratorApp::~MecOrchestratorApp()
{
//    mecHosts.clear();
//    mecApplicationDescriptors_.clear();
}

void MecOrchestratorApp::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        EV << "MEOApp::initialising parameters" << endl;
        localPort = par("localPort");
    }
    inet::ApplicationBase::initialize(stage);
}


void MecOrchestratorApp::handleStartOperation(inet::LifecycleOperation *operation)
{
    EV << "MEOApp::start!" << endl;
    onboardApplicationPackages();
    socket.setOutputGate(gate("socketOut"));
    const char *localAddress = par("localAddress");
    localIPAddress = *localAddress ? inet::L3AddressResolver().resolve(localAddress) : inet::L3Address();
    socket.bind(localIPAddress, par("localPort"));
    socket.setCallback(this);

    // Testing
//    cMessage *tester = new cMessage("Test");
//    scheduleAt(simTime()+0.8, tester);
}

void MecOrchestratorApp::handleMessageWhenUp(omnetpp::cMessage *msg)
{
    if (msg->isSelfMessage() && std::strcmp(msg->getName(), "Test") == 0)
    {
        EV << "MEOApp::ReceivedSelfMessage - TEST!!";

//        // TESTING METHOD FIND BEST MEC HOST
//        if(mecHosts.size() > 0)
//        {
//            const ApplicationDescriptor& appDesc = mecApplicationDescriptors_["WAMECAPP"];
//            findBestMecHost(appDesc);
//        }

    }
    // handle message from the LCM proxy
    else if(msg->arrivedOn("fromUALCMP"))
    {
        EV << "MecOrchestrator::handleMessage - "  << msg->getName() << endl;
        handleUALCMPMessage(msg);
    }
    else if (socket.belongsToSocket(msg))
    {
        socket.processMessage(msg);
    }
    else
    {
        EV << "MEOApp::Not recognised message!";
    }

    delete msg;
    return;
}

void MecOrchestratorApp::socketDataArrived(inet::UdpSocket *socket, inet::Packet *packet)
{
    EV << "MEOApp::Arrived packet on UDP socket " << socket->getSocketId() << ", packet name: " << packet->getName()<< endl;
//    printAvailableAppDescs(); // Debugging
    if(std::strcmp(packet->getName(), "Registration") == 0)
    {
        handleRegistration(packet);
    }
    else if(std::strstr(packet->getName(), "AvailabilityResponse") != NULL)
    {
        EV << "MEOApp::Received availability response from a MECHost - Name of reply: " << packet->getName() << endl;
        handleResourceReply(packet);
    }
    else if(std::strcmp(packet->getName(), "instantiationApplicationResponse") == 0)
    {
        handleInstantiationResponse(packet);
    }
    else if(std::strcmp(packet->getName(), "terminationAppInstResponse") == 0)
    {
        handleTerminationResponse(packet);
    }
    else
    {
        EV << "MEOApp::Not recognized packet!"<< endl;
    }

}

void MecOrchestratorApp::handleRegistration(inet::Packet *packet)
{
    EV << "MEOApp::Received MEC Host registration " << endl;

    auto data = packet->peekData<RegistrationPkt>();
    int hostId = data->getHostId();
    MECHostDescriptor *mecHost = nullptr;

    for(auto &entry : mecHosts)
    {
        if(entry->mecHostId == hostId)
        {
            mecHost = entry;
            break;
        }
    }


    if(mecHost == nullptr)
    {
        // Create a new MECHost
        //registrationEntry = new MECHostRegistrationEntry;
        mecHost = new MECHostDescriptor;
        mecHost->mecHostId = hostId;
        mecHosts.push_back(mecHost);

    }


    if(data->getType() == MM3)
    {
        EV << "MEOApp::Received MEPM registration" << endl;
        mecHost->mepmPort = data->getSourcePort();
        mecHost->mepmHostIp = packet->getTag<inet::L3AddressInd>()->getSrcAddress();
    }
    else if(data->getType() == MM4)
    {
        EV << "MEOApp::Received VIM registration" << endl;
        mecHost->vimPort = data->getSourcePort();
        mecHost->vimHostIp = packet->getTag<inet::L3AddressInd>()->getSrcAddress();
    }
    else
    {
        cRuntimeError("MEOApp::handleRegistration - Errore Registration packet type = NONE");
    }

    printAvailableMECHosts();
}

void MecOrchestratorApp::handleUALCMPMessage(cMessage* msg)
{
    UALCMPMessage* lcmMsg = check_and_cast<UALCMPMessage*>(msg);
    /* Handling CREATE_CONTEXT_APP */
    if(!strcmp(lcmMsg->getType(), CREATE_CONTEXT_APP))
    {
        CreateContextAppMessage* contAppMsg = check_and_cast<CreateContextAppMessage*>(msg);
        handleCreateContextMessage(contAppMsg);
    }

    /* Handling DELETE_CONTEXT_APP */
    else if(!strcmp(lcmMsg->getType(), DELETE_CONTEXT_APP))
        stopMECApp(lcmMsg);
}

void MecOrchestratorApp::handleCreateContextMessage(CreateContextAppMessage* contAppMsg)
{
    if(mecHosts.size() == 0)
    {
       EV << "MEOApp::no hosts available!"<< endl;

       // Sending nack
       sendCreateAppContextAck(false, contAppMsg->getRequestId());
       return;
    }

    std::string appDid;
    if(contAppMsg->getOnboarded() == false)
    {
       // onboard app descriptor
       // source: nodes/mec/MECOrchestrator/MecOrchestrator.h
       EV << "MecOrchestrator::startMECApp - onboarding appDescriptor from: " << contAppMsg->getAppPackagePath() << endl;
       const ApplicationDescriptor& appDesc = onboardApplicationPackage(contAppMsg->getAppPackagePath());
       appDid = appDesc.getAppDId();
    }
    else
    {
       appDid = contAppMsg->getAppDId();
    }

    auto it = mecApplicationDescriptors_.find(appDid);
    if(it == mecApplicationDescriptors_.end())
    {
       EV << "MecOrchestrator::startMECApp - Application package with AppDId["<< contAppMsg->getAppDId() << "] not onboarded." << endl;
       sendCreateAppContextAck(false, contAppMsg->getRequestId());
    //        throw cRuntimeError("MecOrchestrator::startMECApp - Application package with AppDId[%s] not onboarded", contAppMsg->getAppDId());
    }

    const ApplicationDescriptor& desc = it->second;

    // Registering message in pending request
    pendingRequests.insert(std::pair<std::string, CreateContextAppMessage*>(contAppMsg->getDevAppId(), contAppMsg->dup()));

    findBestMecHost(contAppMsg->getDevAppId(), desc);
}

void MecOrchestratorApp::handleResourceReply(inet::Packet *packet)
{
    EV << "MEOApp::handleResourceReply - reply from vim and mepm" << endl;
    auto data = packet->peekData<MecHostResponse>();

    const MecHostResponse* receivedData = data.get();

    auto itResponse = responseMap.find(receivedData->getDeviceAppId());

    if(itResponse == responseMap.end())
    {
        EV << "MEOApp::Request not found served by someone else..." << endl;
        return;
    }

    // Finding corresponding mecHost request
    // The searching process is done on a vector of mecHost response
    // this contains the responses received so far by the MECHost
    // two responses needed: mepm and vim
    MECHostResponseEntry *response = nullptr; // it should not be possible that this response is not found
    int responseIndex = 0; // This index is used in case of negative reply
    for(auto res : itResponse->second)
    {
        if(res->mecHostID == receivedData->getMecHostId())
        {
            response = res;
            break;
        }
        responseIndex ++;
    }

    if(response == nullptr)
    {
        EV_ERROR << "MEOApp::Response corresponding to request ["<< receivedData->getDeviceAppId() << "] not found for that MECHost!" << endl;
        return;
    }

    // finding pending UALCMP request
    auto itRequest = pendingRequests.find(receivedData->getDeviceAppId());

    if(itRequest == pendingRequests.end())
    {
        EV << "MEOApp::handleResourceReply - pending request " << receivedData->getDeviceAppId() << " not found - something failed" <<endl;
        return;
    }


    if(std::strcmp(packet->getName(), "AvailabilityResponseMepm") == 0)
    {
        EV << "MEOApp::Received reply from mepm of mechost: " << receivedData->getMecHostId() <<
                ", result: " << receivedData->getResult()<< endl;
        response->mepmRes = static_cast<ResponseResult>(receivedData->getResult());

    }
    else if(std::strcmp(packet->getName(), "AvailabilityResponseVim") == 0)
    {
       EV << "MEOApp::Received reply from vim of mechost: " << receivedData->getMecHostId() <<
               ", result: " << receivedData->getResult()<< endl;
       response->vimRes = static_cast<ResponseResult>(receivedData->getResult());
    }
    else
    {
        EV << "MEOApp::not recognised message abort.." << endl;
        //delete packet;
        return;
    }

    EV << "MEOApp::Response monitoring changed"<< response->toString() << endl;
    if(response->vimRes==TRUE && response->mepmRes == TRUE)
    {
        MECHostDescriptor* bestHost = nullptr;
        for(auto &itHosts : mecHosts)
        {
            if(itHosts->mecHostId == receivedData->getMecHostId())
            {
                bestHost = itHosts;
            }
        }

        // It is impossible that this happen
        if(bestHost == nullptr)
        {
            EV << "MEOApp::mechost with id: " << receivedData->getMecHostId() << " not found!" << endl;
            return;
        }

        EV << "MEOApp::Found candidate best MECHost (" << receivedData->getMecHostId() <<") for deviceApp: " << receivedData->getDeviceAppId() << endl;


        CreateContextAppMessage *contAppMsg = itRequest->second;

        // Be careful! Can we consider this as valid reply?
        // we actually don't know in an asynchronous situation
        // it depends on the amount of requests sent to that MEC Host
        // after or during its last allocation: we keep trace of the allocation time by using the
        // simulation clock (in a real situation where clocks are not synchronised we may use the orchestrator clock)
        //

        if(bestHost->lastAllocation != -1 && (bestHost->lastAllocation - response->requestTime) > 0)
        {
            // Response not valid - we should again send the request to that MECHost
            EV << "MEOApp::MECHost (" << receivedData->getMecHostId() << ") not valid! - last all: "<< bestHost->lastAllocation << ", request: " << response->requestTime <<  " - Sending request again!" << endl;

            // Send request again
            // 1. Getting application Descriptor
            auto it = mecApplicationDescriptors_.find(contAppMsg->getAppDId());

            // It must exist because request has already been made once
            const ApplicationDescriptor& appDesc = it->second;

            // Building packets
            // mm3
            inet::Packet* pktMM3 = makeAvailableServiceRequestPacket(contAppMsg->getDevAppId(), appDesc);

            // mm4
            inet::Packet* pktMM4 = makeResourceRequestPacket(contAppMsg->getDevAppId(), appDesc.getVirtualResources().cpu, appDesc.getVirtualResources().ram, appDesc.getVirtualResources().disk);

            response->requestTime = sendSRRequest(pktMM3, pktMM4, bestHost->mepmHostIp, bestHost->vimHostIp, bestHost->vimPort, bestHost->mepmPort);

            // resetting replies
            response->vimRes = NO_VALUE;
            response->mepmRes = NO_VALUE;

            return;
        }

        EV << "MEOApp::MECHost [" << receivedData->getMecHostId() << "] valid! - next start mecApp" << endl;

        // if the response is valid
        responseMap.erase(receivedData->getDeviceAppId());
        // here we can start the mecApp
        EV << "MEOApp::handleResourceReply " << contAppMsg->getAppDId() << endl;
        startMECApp(contAppMsg, bestHost);
    }
    else if(response->vimRes!=NO_VALUE && response->mepmRes != NO_VALUE)
    {
        // If we have a reply by both the entities but either one of them or both
        // are false it means that, this MEC Host cannot instantiate the MECApp
        // we can remove it
        EV << "MEOApp::MECHost [" << receivedData->getMecHostId() << "] is not available to host mecApp: " << itRequest->second->getDevAppId() << endl;

        itResponse->second.erase(std::next(itResponse->second.begin(), responseIndex));

        if(itResponse->second.size()==0)
        {
            EV << "MEOApp::No available mechosts found!" << endl;
            //responseMap.erase(receivedData->getDeviceAppId());
            // Sending nack
            sendCreateAppContextAck(false, itRequest->second->getRequestId(), -1, receivedData->getDeviceAppId());
        }

    }
    return;

}

void MecOrchestratorApp::handleInstantiationResponse(inet::Packet *packet)
{
    auto data = packet->peekData<InstantiationApplicationResponse>();
    const InstantiationApplicationResponse *appResponse = data.get();

    // finding corresponding request
    auto itUALCMPRequest = pendingRequests.find(appResponse->getDeviceAppId());


    // If true update allocation time as well
    if(appResponse->getStatus())
    {
        // get MECHost descriptor
        MECHostDescriptor *bestHost;
        for(auto &it : mecHosts)
        {
            if(appResponse->getMecHostId() == it->mecHostId)
            {
                bestHost = it;
            }
        }

        // Update allocation time
        //bestHost->lastAllocation = simTime().dbl();

        // adding mecapp parameters
        mecApp_s newMecApp;
        newMecApp.appDId = itUALCMPRequest->second->getDevAppId();
        newMecApp.mecUeAppID = atoi(itUALCMPRequest->second->getDevAppId());
        newMecApp.mecHostDesc = bestHost;
        newMecApp.ueAddress = inet::L3AddressResolver().resolve(itUALCMPRequest->second->getUeIpAddress());

        newMecApp.mecAppName = appResponse->getAppName();

        // emulation not supported
        newMecApp.isEmulated = false;

        newMecApp.mecAppAddress = appResponse->getMecAppRemoteAddress();
        newMecApp.mecAppPort = appResponse->getMecAppRemotePort();
        newMecApp.mecAppIsntanceId = appResponse->getInstanceId();
        newMecApp.contextId = appResponse->getContextId();
        meAppMap[appResponse->getContextId()] = newMecApp;
        EV << "MEOApp::meapp instantiated contextid: " << appResponse->getContextId() << endl;



        // send successful ack
        sendCreateAppContextAck(true, itUALCMPRequest->second->getRequestId(), appResponse->getContextId());

        // delete pending request
        pendingRequests.erase(itUALCMPRequest);

    }else
    {
        // Something went wrong
        // Here we may try to find again the MECHost or
        // if available choose among the remaining host.
        // For now, we just send a nack to the UALCMP so is the client that has
        // to repeat the procedure
        EV << "MEOApp::handleInstantiationResponse-something went wrong on the MECHost [" << appResponse->getMecHostId() << "]" << endl;
        sendCreateAppContextAck(false, itUALCMPRequest->second->getRequestId(), -1, itUALCMPRequest->second->getDevAppId());
    }

}

void MecOrchestratorApp::handleTerminationResponse(inet::Packet *packet)
{
    auto data = packet->peekData<TerminationAppInstResponse>().get();
    EV << "MEOApp::Received termination response!" << endl;
    sendDeleteAppContextAck(data->getStatus(), data->getRequestId(), data->getContextId());
}

void MecOrchestratorApp::startMECApp(CreateContextAppMessage* contAppMsg, MECHostDescriptor *bestHost)
{
    EV << "MEOApp::sending instantiate mecapp to mechost - " << bestHost->toString() << endl;
    auto it = mecApplicationDescriptors_.find(contAppMsg->getAppDId());

    const ApplicationDescriptor& appDesc = it->second;
    EV << "MEOApp:: appdesc " << appDesc.getAppName() << endl;

    inet::Packet* pktMM3 = new inet::Packet("instantiationApplicationRequest");
    auto instAppRequest = inet::makeShared<InstantiationApplicationRequest>();
    instAppRequest->setUeAppID(atoi(contAppMsg->getDevAppId()));
    instAppRequest->setMEModuleName(appDesc.getAppName().c_str());
    instAppRequest->setMEModuleType(appDesc.getAppProvider().c_str());


    instAppRequest->setRequiredCpu(appDesc.getVirtualResources().cpu);
    instAppRequest->setRequiredRam(appDesc.getVirtualResources().ram);
    instAppRequest->setRequiredDisk(appDesc.getVirtualResources().disk);

    // insert OMNeT like services, only one is supported, for now
    if(!appDesc.getOmnetppServiceRequired().empty())
        instAppRequest->setRequiredService(appDesc.getOmnetppServiceRequired().c_str());
    else
        instAppRequest->setRequiredService("NULL");

    instAppRequest->setContextId(contextIdCounter);

    contextIdCounter++;

    instAppRequest->setChunkLength(inet::B(2000));


    pktMM3->insertAtBack(instAppRequest);

    // Updating allocationTime
    bestHost->lastAllocation = simTime().dbl();

    // sending instantiation application request to MEPM
    socket.sendTo(pktMM3, bestHost->mepmHostIp, bestHost->mepmPort);
    EV << "MEOApp::startMECApp instantiation request sent! " << endl;

}

void MecOrchestratorApp::stopMECApp(UALCMPMessage* msg)
{
    EV << "MEOApp::StopMECApp shutting down mecApp" << endl;

    DeleteContextAppMessage* contAppMsg = check_and_cast<DeleteContextAppMessage*>(msg);

    int contextId = contAppMsg->getContextId();

    EV << "MEOApp::StopMECApp processing contextId: " << contextId << endl;

    auto itMeApp = meAppMap.find(contextId);

    if(itMeApp == meAppMap.end())
    {
        EV << "MEOApp::StopMECApp app not found!" << endl;
        sendDeleteAppContextAck(false, contAppMsg->getRequestId(), contextId);
        return;
    }

    inet::Packet *packet = new inet::Packet("terminationAppInstRequest");

    auto deleteAppMsg = inet::makeShared<TerminationAppInstRequest>();

    deleteAppMsg->setDeviceAppId(std::to_string(itMeApp->second.mecUeAppID).c_str());
    deleteAppMsg->setContextId(contextId);
    deleteAppMsg->setRequestId(contAppMsg->getRequestId());
    deleteAppMsg->setChunkLength(inet::B(1000));

    packet->insertAtBack(deleteAppMsg);

    // sending message to the MEPM
    socket.sendTo(packet, itMeApp->second.mecHostDesc->mepmHostIp, itMeApp->second.mecHostDesc->mepmPort);

}


//MECHostDescriptor* MecOrchestratorApp::findBestMecHost(const ApplicationDescriptor& appDesc)
void MecOrchestratorApp::findBestMecHost(std::string deviceAppId, const ApplicationDescriptor& appDesc)
{
    EV << "MEOApp::findBestMecHost - finding best MecHost..." << endl;

    // Preparing the packet to send
    // mm3
    inet::Packet* pktMM3 = makeAvailableServiceRequestPacket(deviceAppId, appDesc);

    // mm4
    inet::Packet* pktMM4 = makeResourceRequestPacket(deviceAppId, appDesc.getVirtualResources().cpu, appDesc.getVirtualResources().ram, appDesc.getVirtualResources().disk);


    std::string key = deviceAppId;
    for(auto &it : mecHosts)
    {
        MECHostResponseEntry *responseEntry;
        // A MecHost can be used if and only if all the parameters - vimPort and mepmPort - are available
        if(it->vimPort != -1 && it->mepmPort != -1)
        {
            EV << "MEOApp::Found candidate MECHost - id: " << it->mecHostId << " - sending requests" << endl;
            responseEntry = new MECHostResponseEntry;
            responseEntry->mecHostID = it->mecHostId;
            responseEntry->requestTime = sendSRRequest(pktMM3, pktMM4, it->mepmHostIp, it->vimHostIp, it->vimPort, it->mepmPort);

            responseMap[key].push_back(responseEntry);
        }
    }

}

void MecOrchestratorApp::onboardApplicationPackages()
{
    EV << "MEOApp::onboarding Application packages" << endl;
    // Getting mec application packages from ned file list "mecApplicationPackageList"
    if(this->hasPar("mecApplicationPackageList") && strcmp(par("mecApplicationPackageList").stringValue(), "")){

        char* token = strtok ( (char*) par("mecApplicationPackageList").stringValue(), ", ");            // split by commas

        while (token != NULL)
        {
            int len = strlen(token);
            char buf[len+strlen(".json")+strlen("ApplicationDescriptors/")+1];
            strcpy(buf,"ApplicationDescriptors/");
            strcat(buf,token);
            strcat(buf,".json");
            onboardApplicationPackage(buf);
            token = strtok (NULL, ", ");
        }
    }
    else{
        EV << "MEOApp::onboardApplicationPackages - No mecApplicationPackageList found" << endl;
    }
}

const ApplicationDescriptor& MecOrchestratorApp::onboardApplicationPackage(const char* fileName)
{
    // Reading json file
    EV <<"MEOApp::onBoardApplicationPackages - onboarding application package (from request): "<< fileName << endl;
        ApplicationDescriptor appDesc(fileName);
        if(mecApplicationDescriptors_.find(appDesc.getAppDId()) != mecApplicationDescriptors_.end())
        {
            EV << "MEOApp::onboardApplicationPackages() - Application descriptor with appName ["<< fileName << "] is already present. Skipping...\n" << endl;
        }
        else
        {
            mecApplicationDescriptors_[appDesc.getAppDId()] = appDesc; // add to the mecApplicationDescriptors_
        }

        return mecApplicationDescriptors_[appDesc.getAppDId()];
}

double MecOrchestratorApp::sendSRRequest(inet::Packet* pktMM3, inet::Packet* pktMM4, inet::L3Address mepmHostAddress, inet::L3Address vimHostAddress, int vimPort, int mepmPort)
{
    EV << "MEOApp::Sending requests to " << mepmHostAddress.str() << " and " << vimHostAddress.str() <<endl;

    // Requesting to vim if MECApp is allocable
    socket.sendTo(pktMM4->dup(), vimHostAddress, vimPort);

    // Requesting to MEPM if the MECPlatform has the needed services
    socket.sendTo(pktMM3->dup(), mepmHostAddress, mepmPort);

    return simTime().dbl();
}

void MecOrchestratorApp::sendCreateAppContextAck(bool result, unsigned int requestSno, int contextId, const std::string &deviceAppId)
{
    EV << "MEOApp::sendCreateAppContextAck - result: "<< result << " reqSno: " << requestSno << " contextId: " << contextId << endl;
    CreateContextAppAckMessage *ack = new CreateContextAppAckMessage();
    ack->setType(ACK_CREATE_CONTEXT_APP);
    if(result)
    {
        if(meAppMap.empty() || meAppMap.find(contextId) == meAppMap.end())
        {
            EV << "MEOApp::ackMEAppPacket - ERROR meApp["<< contextId << "] does not exist!" << endl;
//            throw cRuntimeError("MecOrchestrator::ackMEAppPacket - ERROR meApp[%d] does not exist!", contextId);
            return;
        }

        mecApp_s mecAppStatus = meAppMap[contextId];

        ack->setSuccess(true);
        ack->setContextId(contextId);
        ack->setAppInstanceId(mecAppStatus.mecAppIsntanceId.c_str());
        ack->setRequestId(requestSno);
        std::stringstream uri;

        uri << mecAppStatus.mecAppAddress.str()<<":"<<mecAppStatus.mecAppPort;

        ack->setAppInstanceUri(uri.str().c_str());
    }
    else
    {
        ack->setSuccess(false);
        ack->setRequestId(requestSno);

        if(!deviceAppId.empty()){
            EV << "MEOApp::Deleting pending requests and responses"<<endl;
            // Deleting pending request from UALCMP
            pendingRequests.erase(deviceAppId);

            // Deleting pending response from MECHosts
            responseMap.erase(deviceAppId);

            EV << "MEOApp::Pending UALCMP requests: " << pendingRequests.size() << ", pending mechost responses: " << responseMap.size() << endl;
        }

    }
    send(ack, "toUALCMP");
}

void MecOrchestratorApp::sendDeleteAppContextAck(bool result, unsigned int requestSno, int contextId)
{
    //no changes
    EV << "MEOApp::sendDeleteAppContextAck - result: "<< result << " reqSno: " << requestSno << " contextId: " << contextId << endl;
    DeleteContextAppAckMessage * ack = new DeleteContextAppAckMessage();
    ack->setType(ACK_DELETE_CONTEXT_APP);
    ack->setRequestId(requestSno);
    ack->setSuccess(result);

    send(ack, "toUALCMP");
}

inet::Packet* MecOrchestratorApp::makeResourceRequestPacket(std::string deviceAppId, double cpu, double ram, double disk)
{
    inet::Packet* pktMM4 = new inet::Packet("ResourceRequest");
    auto resourcePkt = inet::makeShared<MeoVimRequest>();
    resourcePkt->setDeviceAppId(deviceAppId.c_str());
    resourcePkt->setCpu(cpu);
    resourcePkt->setRam(ram);
    resourcePkt->setDisk(disk);
    resourcePkt->setChunkLength(inet::B(2000));
    pktMM4->insertAtBack(resourcePkt);

    return pktMM4;
}

inet::Packet* MecOrchestratorApp::makeAvailableServiceRequestPacket(std::string deviceAppId, const ApplicationDescriptor& appDesc)
{
    inet::Packet* pktMM3 = new inet::Packet("ServiceRequest");
    std::vector<std::string> requiredServiceNames = appDesc.getAppServicesRequired();

    //std::copy(appDesc.getAppServicesRequired().begin(), appDesc.getAppServicesRequired().end(), serviceNames);

    auto servicePkt = inet::makeShared<MeoMepmRequest>();
    servicePkt->setDeviceAppId(deviceAppId.c_str());
    servicePkt->setChunkLength(inet::B(2000));
    servicePkt->setRequiredServiceNamesArraySize(requiredServiceNames.size());
    // the index is needed to populate the other packet array
    // so we use the old fashioned way
    for(int i = 0; i < requiredServiceNames.size(); i++)
    {
        servicePkt->setRequiredServiceNames(i, requiredServiceNames[i].c_str());
    }
    pktMM3->insertAtBack(servicePkt);

    return pktMM3;
}

const ApplicationDescriptor* MecOrchestratorApp::getApplicationDescriptorByAppName(std::string& appName) const
{
    for(const auto& appDesc : mecApplicationDescriptors_)
    {
        if(appDesc.second.getAppName().compare(appName) == 0)
            return &(appDesc.second);

    }

    return nullptr;
}

void MecOrchestratorApp::printAvailableMECHosts()
{
    EV << "#### MEOAPP::Available Resources ####" << endl;
    for(auto &it : mecHosts)
    {
        EV << it->toString() << endl;
        EV << "------------------------------------" << endl;
    }
    EV << "####################################" << endl;
}

void MecOrchestratorApp::printAvailableAppDescs()
{
    EV << "#### MEOAPP::Available Packages ####" << endl;
    for(auto it = mecApplicationDescriptors_.begin(); it != mecApplicationDescriptors_.end(); ++it)
    {
        EV << "MEOApp::App Name: " << it->second.getAppName() << ", Description" << it->second.getAppDescription()<< endl;
        EV << "------------------------------------" << endl;
    }
    EV << "####################################" << endl;

}
