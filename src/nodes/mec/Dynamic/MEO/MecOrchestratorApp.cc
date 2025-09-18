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
#include <random>
#include <chrono>
#include <string>

#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h" // TODO add this messages to our list
#include "inet/networklayer/common/L3AddressTag_m.h"

#include "nodes/mec/Federator/Messages/MEFmessages_m.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"

#include "nodes/mec/MECPlatform/MECServices/ApplicationMobilityService/Messages/MobilityMessages_m.h"

Define_Module(MecOrchestratorApp);

MecOrchestratorApp::MecOrchestratorApp()
{
    mecHosts.clear();
    responseMap.clear();
    mecApplicationDescriptors_.clear();
    contextIdCounter = 0;
    processResourceRequest_ = nullptr;
    lastUsedHostIndex = -1;
}

MecOrchestratorApp::~MecOrchestratorApp()
{
//    mecHosts.clear();
//    mecApplicationDescriptors_.clear();
    pendingRequests.clear();
    cancelAndDelete(processResourceRequest_);

    while(!resourceRequestQueue_.empty()) {resourceRequestQueue_.pop();}
}

void MecOrchestratorApp::initialize(int stage)
{

    saveMigration = "";

    if (stage == inet::INITSTAGE_LOCAL)
    {
        EV << "MEOApp::initialising parameters" << endl;
        localPort = par("localPort");

        processResourceRequest_ = new cMessage("processResourceRequest");
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

    const char *localMEFAddress = par("MEFAddress");
    MEFAddress = *localMEFAddress ? inet::L3AddressResolver().resolve(localMEFAddress) : inet::L3Address();
    MEFPort = par("MEFPort");


    //cMessage *tester = new cMessage("Migration");
    //scheduleAt(simTime()+1.7, tester);


    //cMessage *tester = new cMessage("InitApp");
    //scheduleAt(simTime()+1.7, tester);

}

void MecOrchestratorApp::handleMessageWhenUp(omnetpp::cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if(std::strcmp(msg->getName(),"Test") == 0)
        {
            EV << "MEOApp::ReceivedSelfMessage - TEST!!";
            //        // TESTING METHOD FIND BEST MEC HOST
            //        if(mecHosts.size() > 0)
            //        {
            //            const ApplicationDescriptor& appDesc = mecApplicationDescriptors_["WAMECAPP"];
            //            findBestMecHost(appDesc);
            //        }
        }
        else if(std::strcmp(msg->getName(),"processResourceRequest") == 0)
        {
        sendSRRequest();
        if(resourceRequestQueue_.size() > 0 && !processResourceRequest_->isScheduled())
        {
            scheduleAt(simTime(), processResourceRequest_);
        }
        return;
        }else if(std::strcmp(msg->getName(),"TestRegistration") == 0){
            inet::Packet *packetResp = new inet::Packet("Registration");

            auto response = inet::makeShared<SystemInfo>();
            response->setSystemId("111111111");
            response->setSystemName("systemName");
            response->setSystemProvider("systemProvider");

            response->setChunkLength(inet::B(64));
            packetResp->insertAtBack(response);

            socket.sendTo(packetResp, MEFAddress, MEFPort);
        }else if(std::strcmp(msg->getName(),"TestCancellation") == 0){
            inet::Packet *packetResp = new inet::Packet("Cancellation");

            auto response = inet::makeShared<SystemInfo>();
            response->setSystemId("111111111");
            response->setSystemName("systemName");
            response->setSystemProvider("systemProvider");

            response->setChunkLength(inet::B(64));
            packetResp->insertAtBack(response);

            socket.sendTo(packetResp, MEFAddress, MEFPort);
        }else if(std::strcmp(msg->getName(),"TestUpdate") == 0){
            inet::Packet *packetResp = new inet::Packet("Update");

            auto response = inet::makeShared<SystemInfo>();
            response->setSystemId("111111111");
            response->setSystemName("systemName1");
            response->setSystemProvider("systemProvider");

            response->setChunkLength(inet::B(64));
            packetResp->insertAtBack(response);

            socket.sendTo(packetResp, MEFAddress, MEFPort);
        }else if(std::strcmp(msg->getName(),"TestRequestSystem") == 0){

            inet::Packet *packetResp = new inet::Packet("MECSystemReq");

            auto response = inet::makeShared<SystemInfo>();
            response->setSystemId("222222222");

            response->setChunkLength(inet::B(64));
            packetResp->insertAtBack(response);

            socket.sendTo(packetResp, MEFAddress, MEFPort);
        }
        else if(std::strcmp(msg->getName(),"TestRequestApp") == 0){

            inet::Packet *packetResp = new inet::Packet("appRequestAPPtoMEO");

            auto request = inet::makeShared<AppRequest>();

            request->setAppName("PongApp");

            request->setChunkLength(inet::B(64));
            packetResp->insertAtBack(request);

            socket.sendTo(packetResp, localIPAddress, par("localPort"));

        }
        else if(std::strcmp(msg->getName(),"InitApp") == 0){

            inet::Packet *pkt = new inet::Packet("StartAppContext");
            auto chunk = inet::makeShared<StartAppContextChunk>();

            chunk->setAppName("PingApp");
            chunk->setAppPackageSource("ApplicationDescriptors/PingApp.json");
            chunk->setAppIsOnboarded(true);
            chunk->setDevAppId(1);

            pkt->insertAtBack(chunk);
            send(pkt, "toUALCMP");


        }
        else if(std::strcmp(msg->getName(),"Migration") == 0){

            if(strcmp(par("localAddress"), "mecOrchestrator1") ==0){
                //handleAppMigrationRequest();
            }


        }


    // handle message from the LCM proxy
    }
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

void MecOrchestratorApp::socketDataArrived(inet::UdpSocket *socket, inet::Packet *packet){
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
    else if(std::strcmp(packet->getName(),"ACK") == 0){
        auto data = packet->peekData<AckPk>();

        EV << "MECOrchestrator ACK recived: " << data->getAckId() << endl;
    }
    else if(std::strcmp(packet->getName(),"MECSystemInfoRes") == 0){
        auto data = packet->peekData<SystemInfo>();

        EV << "MECOrchestrator MECSystemInfoRes recived: " << data->getSystemId() << " " << data->getSystemName() << data->getSystemProvider() <<  endl;
    }
    else if(std::strcmp(packet->getName(),"ReqMECSystemInfo") == 0){
        handleReqMECSystemInfo(packet);
    }
    else if(std::strcmp(packet->getName(), "appRequestVIMtoMEO") == 0)
    {
        handleAppRequestVIMtoMEO(packet);
    }
    else if(std::strcmp(packet->getName(), "appRequestMEFtoMEO") == 0)
    {
        handleAppRequestMEFtoMEO(packet);
    }
    else if(std::strcmp(packet->getName(), "appResponseMEFtoMEO") == 0)
    {
        handleAppResponseMEFtoMEO(packet);
    }
    else if(std::strcmp(packet->getName(), "appMigrationRequestMEFtoMEO") == 0)
    {
        EV << "MEOApp::appMigrationRequestMEFtoMEO arrived!"<< endl;
        handleAppMigrationRequestMEFtoMEO(packet);
    }
    else if(std::strcmp(packet->getName(), "appMigrationRequestMEOtoMEF") == 0)
    {
        EV << "MEOApp::appMigrationRequestMEOtoMEF arrived!"<< endl;
        handleAppMigrationRequestMEFtoMEO(packet);
    }
    else if(std::strcmp(packet->getName(),"FederationMigrationTrigger") == 0){
        EV << "MEOApp::FederationMigrationTrigger arrived!"<< endl;
        handleAppMigrationRequest(packet);
    }
    else if(std::strcmp(packet->getName(), "ackMEFtoMEO") == 0)
    {
        EV << "MEOApp::ackMEFtoMEO arrived!"<< endl;
        handleAckMEFtoMEO(packet);
    }
    else
    {
        EV << "MEOApp::Not recognized packet!"<< endl;
    }
}

void MecOrchestratorApp::handleAckMEFtoMEO(inet::Packet *contAppMsg){

    auto data = contAppMsg->peekData<AckMigration>();

    inet::Packet *packet = new inet::Packet("terminationAppInstRequest");
    auto deleteAppMsg = inet::makeShared<TerminationAppInstRequest>();

    EV << "MecOrchestratorApp::handleAckMEFtoMEO: arrived ack and send termination at: " << data->getAppId() << endl;

    for(const auto& pair : meAppMap){
            const mecApp_s& app = pair.second;

            EV << app.mecAppIsntanceId << " - " << data->getAppId() << endl;
            if(app.mecAppIsntanceId == data->getAppId()){
                EV << "MEO::handleAppMigrationRequest: FOUND info: " << app.mecAppAddress <<":"<< app.mecAppPort<< endl;

                deleteAppMsg->setDeviceAppId(std::to_string(app.mecUeAppID).c_str());
                deleteAppMsg->setContextId(app.contextId);
                deleteAppMsg->setRequestId(1);
                deleteAppMsg->setChunkLength(inet::B(1000));
                packet->insertAtBack(deleteAppMsg);

                // sending message to the MEPM
                //socket.sendTo(packet, app.mecHostDesc->mepmHostIp, app.mecHostDesc->mepmPort);


                simtime_t start = handoverStartTime[app.ueAddress.str().c_str()];
                simtime_t end = simTime();

                simtime_t duration = end - start;
                emit(totalHandoverMigrationTime, duration);
                std::cout << getFullPath() << ": HO+MIG emit = " << duration << endl;


                return;
            }
    }
}

void MecOrchestratorApp::handleAppMigrationRequestMEFtoMEO(inet::Packet *contAppMsg){



    EV << "MEOApp::handleAppMigrationRequestMEFtoMEO" << endl;
    auto data = contAppMsg->peekData<AppMigrationRequest>();

    inet::Packet* pktdup = new inet::Packet("appMigrationRequestMEOtoMPM");
    auto request = inet::makeShared<AppMigrationRequest>();

    request->setAppName(data->getAppName());
    request->setAppPackageSource(data->getAppPackageSource());
    request->setAppIsOnboarded(data->getAppIsOnboarded());
    request->setDevAppId(data->getDevAppId());
    request->setAppAddress(data->getAppAddress());
    request->setAppPort(data->getAppPort());
    request->setUeIpAddress(data->getUeIpAddress());
    request->setIpMefRequest(data->getIpMefRequest());


    EV << "MEOApp::handleAppMigrationRequestMEFtoMEO recived : " << data->getDevAppId() << endl;

    request->setChunkLength(inet::B(64));
    pktdup->insertAtBack(request);


    if (mecHosts.size() == 0) {
        EV << "MEOApp::no hosts available!" << endl;
        return;
    }

    std::string appDid;
    if (!data->getAppIsOnboarded()) {
        EV << "MecOrchestrator::startMECApp - onboarding appDescriptor from: " << data->getAppPackageSource() << endl;
        const ApplicationDescriptor& appDesc = onboardApplicationPackage(data->getAppPackageSource());
        appDid = appDesc.getAppDId();
    } else {

    }

    auto it = mecApplicationDescriptors_.find(appDid);
    if (it == mecApplicationDescriptors_.end()) {
        EV << "MecOrchestrator::startMECApp - Application package with AppDId[" << appDid << "] not onboarded." << endl;
        return;
    }

    const ApplicationDescriptor& desc = it->second;

    CreateContextAppMessage* r = new CreateContextAppMessage();
    EV << "DEBUG INDIRIZZO: " << r->getAddressMigration() << endl;
    EV << "DEBUG INDIRIZZO: " << data->getAppAddress() << endl;

    r->setDevAppId(data->getDevAppId());
    r->setAppPackagePath(data->getAppPackageSource());
    r->setOnboarded(data->getAppIsOnboarded());
    r->setUeIpAddress(data->getUeIpAddress());
    r->setAddressMigration(data->getAppAddress());
    r->setPortMigration(data->getAppPort());

    EV << "DEBUG INDIRIZZO: " << r->getAddressMigration() << endl;

    if (!data->getAppIsOnboarded()) {
        r->setAppDId(appDid.c_str());
    }

    EV << "DEBUG appID: " << data->getDevAppId() << endl;
    pendingRequests.insert(std::pair<std::string, CreateContextAppMessage*>(data->getDevAppId(), r->dup()));
    migrationInfo[data->getDevAppId()] = { data->getAppAddress(), data->getAppPort() };




    auto itRequest = pendingRequests.find(data->getDevAppId());

    if (itRequest == pendingRequests.end()) {
        EV << "MEOApp::handleResourceReply - pending request "
           << data->getDevAppId() << " non trovata!" << endl;
        return;
    }

    EV << "Trovata chiave [" << itRequest->first << "] con valore: "
       << itRequest->second->getAddressMigration() <<  endl;


    if (desc.getAppDeploymentSetting() != "")
        deployOnSpecifiedMecHost(data->getDevAppId(), desc, desc.getAppDeploymentSetting());

    EV << "MecOrchestrator::startMECApp - Application package with AppDId[" << appDid << "] find best host." << endl;
    findMecHostByTargetId(data->getDevAppId(), desc, data->getGbNode());



    EV << "MEOAPP::debug - ipMefRequest: " << data->getIpMefRequest() << endl;
    if (strcmp(data->getIpMefRequest(), "INTERNO") != 0){
        inet::Packet* pktdup1 = new inet::Packet("ackMEOtoMEF");
        auto request1 = inet::makeShared<AckMigration>();

        request1->setAppId(data->getAppName());
        request1->setIpMefRequest(data->getIpMefRequest());


        request1->setChunkLength(inet::B(64));
        pktdup1->insertAtBack(request1);

        EV << "MEOAPP::startMECApp - ACK al Federator" << endl;
        socket.sendTo(pktdup1, MEFAddress, MEFPort);

        simtime_t delay = simTime() - data->getStartTime();
        emit(handoverFed, delay);
    }else{
        inet::Packet* pktdup1 = new inet::Packet("ackMEFtoMEO");
        auto request1 = inet::makeShared<AckMigration>();

        request1->setAppId(data->getAppName());


        request1->setChunkLength(inet::B(64));
        pktdup1->insertAtBack(request1);

        EV << "MEOAPP::startMECApp - ACK al MEO (funzione)" << endl;
        handleAckMEFtoMEO(pktdup1);

        simtime_t delay = simTime() - data->getStartTime();
        emit(handoverInt, delay);
    }

}

void MecOrchestratorApp::handleAppMigrationRequest(inet::Packet *contAppMsg){
    auto data = contAppMsg->peekData<FederationMigrationTrigger>();

    std::string input = data->getAppId();
    std::size_t pos = input.find('[');
    std::string appName = (pos != std::string::npos) ? input.substr(0, pos) : input;


    std::string appPackageSource = "ApplicationDescriptors/" + appName + ".json";

    inet::Packet *pkt = new inet::Packet("appMigrationRequestMEOtoMEF");
    auto request = inet::makeShared<AppMigrationRequest>();

    request->setAppName(input.c_str());
    request->setAppPackageSource(appPackageSource.c_str());
    request->setAppIsOnboarded(false);
    std::string inputStr = data->getUeIpAddress() + input;
    //request->setDevAppId(inputStr.c_str());


    request->setDevAppId(std::to_string(data->getAppIdMigration()).c_str());
    request->setGbNode(data->getGbNode());
    request->setUeIpAddress(data->getUeIpAddress());
    request->setAppAddress(data->getAddressMigration());
    request->setAppPort(data->getPortMigration());
    request->setIpMefRequest("INTERNO");
    request->setStartTime(data->getStartTime());

    EV << "MEOApp::handleAppMigrationRequest recived UEIPADDRESS: " << data->getUeIpAddress() << endl;

    //ho usato quel campo cambiato nel caso cambia
    /*
    for(const auto& pair : meAppMap){
            const mecApp_s& app = pair.second;

            std::string x = std::to_string(data->getAppIdMigration()).c_str();
            if (app.appDId == x) {


                break;
            }
    }


    */
    //EV << "MEO::handleAppMigrationRequest: FOUND info: " << app.appDId <<":"<< x << endl;

    request->setChunkLength(inet::B(2048));
    pkt->insertAtBack(request);



    std::unordered_map<int, std::tuple<std::string, std::string, int>> gbNodeToAddressVimPort;

    std::string path = par("mapFile").stdstringValue();
    std::ifstream infile(path);
    if (!infile.is_open()) {
        throw cRuntimeError("Impossibile aprire il file di mappatura: %s", path.c_str());
    }

    int gbNode, port;
    std::string address, vim;
    while (infile >> gbNode >> address >> vim >> port) {
        gbNodeToAddressVimPort[gbNode] = std::make_tuple(address, vim, port);
    }


    gbNode = data->getGbNode();
    if (gbNodeToAddressVimPort.count(gbNode)) {
        const auto& entry = gbNodeToAddressVimPort[gbNode];
        std::string destAddr = std::get<0>(entry);
        std::string vimAddr = std::get<1>(entry);
        int destPort = std::get<2>(entry);

        EV << "MEO::handleAppMigrationRequest: send to " << destAddr << ":" << destPort
           << " (vim: " << vimAddr << ")" << endl;

        socket.sendTo(pkt, inet::L3AddressResolver().resolve(destAddr.c_str()), destPort);
    } else {
        EV_ERROR << "GBNode non trovato nella mappa: " << gbNode << endl;
    }

    handoverStartTime[data->getUeIpAddress()] = simTime();
    std::cout << getFullPath() << ": HO+MIG start" << endl;

    simtime_t delay = simTime() - data->getStartTime();
    emit(msgFederationTrigger, delay);





}


void MecOrchestratorApp::handleReqMECSystemInfo(inet::Packet *packet)
{
    auto data = packet->peekData<SystemInfo>();

    EV << "MECOrchestrator MEF request: " << data->getSystemId() <<  endl;

    if(std::strcmp(data->getSystemId(),"111111111") == 0){
        inet::Packet *packetResp = new inet::Packet("MEFSystemInfoRes");

        auto response = inet::makeShared<SystemInfo>();
        response->setSystemId("111111111");
        response->setSystemName("systemName");
        response->setSystemProvider("systemProvider");
        response->setIpMefRequest(data->getIpMefRequest());

        response->setChunkLength(inet::B(64));
        packetResp->insertAtBack(response);

        socket.sendTo(packetResp, MEFAddress, MEFPort);
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

    if(desc.getAppDeploymentSetting() != "")
    {
        deployOnSpecifiedMecHost(contAppMsg->getDevAppId(), desc, desc.getAppDeploymentSetting());
    }
    //Francesco Milione added
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
            inet::Packet* pktMM3 = makeAvailableServiceRequestPacket(bestHost->mepmHostIp, bestHost->mepmPort, contAppMsg->getDevAppId(), appDesc);

            // mm4
            inet::Packet* pktMM4 = makeResourceRequestPacket(bestHost->vimHostIp, bestHost->vimPort,  contAppMsg->getDevAppId(), appDesc.getVirtualResources().cpu, appDesc.getVirtualResources().ram, appDesc.getVirtualResources().disk);
            ResourceRequest *r = new ResourceRequest();
            r->pktMM3 = pktMM3;
            r->pktMM4 = pktMM4;
            r->vimHostAddress = bestHost->vimHostIp;
            r->mepmHostAddress = bestHost->mepmHostIp;
            r->vimPort = bestHost->vimPort;
            r->mepmPort = bestHost->mepmPort;
            resourceRequestQueue_.push(r);
            response->requestTime = simTime().dbl();
            // resetting replies
            response->vimRes = NO_VALUE;
            response->mepmRes = NO_VALUE;
            if(!processResourceRequest_->isScheduled())
                scheduleAt(simTime(),processResourceRequest_);
//            response->requestTime = sendSRRequest(pktMM3, pktMM4, bestHost->mepmHostIp, bestHost->vimHostIp, bestHost->vimPort, bestHost->mepmPort);

            return;
        }

        EV << "MEOApp::MECHost [" << receivedData->getMecHostId() << "] valid! - next start mecApp" << endl;

        // if the response is valid
       //clearResponseMapByAppIdAndRequest(receivedData->getDeviceAppId());
        // here we can start the mecApp
        EV << "MEOApp::handleResourceReply " << contAppMsg->getAppDId() << endl;
        std::cout << "MEOApp::handleAppMigrationRequestMEFtoMEO recived appId: " << receivedData->getDeviceAppId() << endl;

        responseMap.erase(receivedData->getDeviceAppId());


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
        std::cout << "FATALERROR" << endl;

    }
    return;

}

void MecOrchestratorApp::handleAppRequestVIMtoMEO(inet::Packet* contAppMsg){
    auto data = contAppMsg->peekData<AppRequest>();

    auto srcAddr = contAppMsg->getTag<inet::L3AddressInd>()->getSrcAddress();
    int srcPort = contAppMsg->getTag<inet::L4PortInd>()->getSrcPort();

    //Controllo se l'instanza c'è
    bool found = false;

    std::string requestAppId = data->getAppId();
    std::string requestAppName = data->getAppName();

    for(const auto& pair : meAppMap){
        const mecApp_s& app = pair.second;

        bool matchAppId = !requestAppId.empty() && app.appDId == requestAppId;
        bool matchAppName = !requestAppName.empty() && app.mecAppName == requestAppName;

        if(matchAppId || matchAppName){
            EV << "MEO::handleAppRequestVIMtoMEO: FOUND info: " << app.mecAppAddress <<":"<< app.mecAppPort<< endl;

            //Ritorno instanza

            found = true;

            inet::Packet* pktdup = new inet::Packet("appResponseMEOtoVIM");
            auto request = inet::makeShared<AppResponse>();

            request->setAppId(data->getAppId());
            request->setAppName(data->getAppName());
            request->setAppAddress(app.mecAppAddress.str().c_str());
            request->setAppPort(app.mecAppPort);
            request->setHostId(data->getHostId());
            request->setPortRequest(data->getPortRequest());

            request->setChunkLength(inet::B(64));
            pktdup->insertAtBack(request);

            inet::L3Address destIp;
            int destPort = -1;

            //Cerco il vim

            for(auto &entry : mecHosts)
            {
                if(entry->mecHostId == data->getHostId())
                {
                    destIp = entry->vimHostIp;
                    destPort = entry->vimPort;

                    break;
                }
            }


            if(destPort == -1)
            {
                EV << "MEO::handleAppRequestVIMtoMEO - ERROR vim not found" << endl;
            }else{
                EV << "MEO::handleAppRequestVIMtoMEO - sending to:  " << destIp << ":" << destPort << endl;
                socket.sendTo(pktdup, destIp, destPort);
            }

            break;
        }
    }

    if(!found){
        inet::Packet* pktdup = new inet::Packet("appRequestMEOtoMEF");
        auto request = inet::makeShared<AppRequest>();

        request->setAppId(data->getAppId());
        request->setAppName(data->getAppName());
        //request->setIpRequest(srcAddr.str().c_str());
        //request->setPortRequest(srcPort);
        request->setIpRequest(data->getIpRequest());
        request->setPortRequest(data->getPortRequest());
        request->setHostId(data->getHostId());

        request->setChunkLength(inet::B(64));
        pktdup->insertAtBack(request);

        EV << "MEO::handleAppRequestAPPtoMEO - sending to MEF request:  " << MEFAddress << ":" << MEFPort << endl;
        socket.sendTo(pktdup, MEFAddress, MEFPort);

    }
}

void MecOrchestratorApp::handleAppRequestMEFtoMEO(inet::Packet* contAppMsg){
    auto data = contAppMsg->peekData<AppRequest>();

    auto srcAddr = contAppMsg->getTag<inet::L3AddressInd>()->getSrcAddress();
    int srcPort = contAppMsg->getTag<inet::L4PortInd>()->getSrcPort();

    //Controllo se l'instanza c'è
    bool found = false;

    std::string requestAppId = data->getAppId();
    std::string requestAppName = data->getAppName();

    for(const auto& pair : meAppMap){

        const mecApp_s& app = pair.second;
        EV << "MEO::DEBUG:" << app.mecAppName<< endl;

        bool matchAppId = !requestAppId.empty() && app.appDId == requestAppId;
        bool matchAppName = !requestAppName.empty() && app.mecAppName == requestAppName;

        EV_INFO << "------ Stampa mecApp_s ------" << endl;
        EV_INFO << "contextId: " << app.contextId << endl;
        EV_INFO << "appDId: " << app.appDId << endl;
        EV_INFO << "mecAppName: " << app.mecAppName << endl;
        EV_INFO << "mecAppIsntanceId: " << app.mecAppIsntanceId << endl;
        EV_INFO << "mecUeAppID: " << app.mecUeAppID << endl;

        if (app.mecHostDesc != nullptr) {
            EV_INFO << "mecHostDesc " << app.mecHostDesc->toString() << endl;
        } else {
            EV_INFO << "mecHostDesc: nullptr" << endl;
        }

        EV_INFO << "ueSymbolicAddres: " << app.ueSymbolicAddres << endl;
        EV_INFO << "ueAddress: " << app.ueAddress << endl;
        EV_INFO << "uePort: " << app.uePort << endl;
        EV_INFO << "mecAppAddress: " << app.mecAppAddress << endl;
        EV_INFO << "mecAppPort: " << app.mecAppPort << endl;
        EV_INFO << "isEmulated: " << (app.isEmulated ? "true" : "false") << endl;
        EV_INFO << "lastAckStartSeqNum: " << app.lastAckStartSeqNum << endl;
        EV_INFO << "lastAckStopSeqNum: " << app.lastAckStopSeqNum << endl;
        EV_INFO << "-----------------------------" << endl;

        if(matchAppId || matchAppName){
            EV << "MEO::handleAppRequestMEFtoMEO: FOUND info: "<< app.mecAppAddress <<":"<< app.mecAppPort<< endl;


            //Ritorno instanza

            found = true;

            inet::Packet* pktdup = new inet::Packet("appResponseMEOtoMEF");
            auto request = inet::makeShared<AppResponse>();

            request->setAppId(data->getAppId());
            request->setAppName(data->getAppName());
            request->setAppAddress(app.mecAppAddress.str().c_str());
            request->setAppPort(app.mecAppPort);
            request->setIpRequest(data->getIpRequest());
            request->setPortRequest(data->getPortRequest());
            request->setIpMefRequest(data->getIpMefRequest());
            request->setHostId(data->getHostId());

            request->setChunkLength(inet::B(64));
            pktdup->insertAtBack(request);

            EV << "MEO::handleAppRequestMEFtoMEO - sending to MEF request:  " << MEFAddress << ":" << MEFPort << endl;
            socket.sendTo(pktdup, MEFAddress, MEFPort);

            break;
        }
    }

    if(!found){
        EV << "MEO::handleAppRequestMEFtoMEO - NOTHING" << endl;
    }
}


void MecOrchestratorApp::handleAppResponseMEFtoMEO(inet::Packet* contAppMsg){
    auto data = contAppMsg->peekData<AppResponse>();

    inet::Packet* pktdup = new inet::Packet("appResponseMEOtoVIM");
    auto request = inet::makeShared<AppResponse>();

    request->setAppId(data->getAppId());
    request->setAppName(data->getAppName());
    request->setAppAddress(data->getAppAddress());
    request->setAppPort(data->getAppPort());
    request->setIpRequest(data->getIpRequest());
    request->setPortRequest(data->getPortRequest());
    request->setIpMefRequest(data->getIpMefRequest());
    request->setHostId(data->getHostId());

    request->setChunkLength(inet::B(64));
    pktdup->insertAtBack(request);

    inet::L3Address destIp;
    int destPort = -1;

    //Cerco il vim
    for(auto &entry : mecHosts)
    {
        if(entry->mecHostId == data->getHostId())
        {
            destIp = entry->vimHostIp;
            destPort = entry->vimPort;

            break;
        }
    }


    if(destPort == -1)
    {
        EV << "MEO::handleAppResponseMEFtoMEO - ERROR vim not found" << endl;
    }else{
        EV << "MEO::handleAppResponseMEFtoMEO - sending to:  " << destIp << ":" << destPort << endl;
        socket.sendTo(pktdup, destIp, destPort);
    }





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

        bool mobilitySupportRequired = false;

        for(int i = 0; i < appResponse->getRequiredStandardServiceArraySize() && !mobilitySupportRequired; i++)
        {
          if(std::strcmp(appResponse->getRequiredStandardService(i), "ApplicationMobilityService") == 0)
              mobilitySupportRequired = true;
        }

        std::string amsUri = "";
        if(mobilitySupportRequired)
        {
            amsUri = appResponse->getAmsAdddress().str() + ":" + std::to_string(appResponse->getAmsPort());
        }
        // send successful ack
        sendCreateAppContextAck(true, itUALCMPRequest->second->getRequestId(), appResponse->getContextId(), "", amsUri);


        auto it = startTimes.find(itUALCMPRequest->second->getDevAppId());
        if (it != startTimes.end()) {
            double start = it->second;
            double delay = simTime().dbl() - start;
            emit(registerSignal("mechostdelay"), delay);

            startTimes.erase(it);
        }

        auto it1 = startTimesInit.find(itUALCMPRequest->second->getDevAppId());
            if (it1 != startTimesInit.end()) {
                double start = it1->second;
                double delay = simTime().dbl() - start;
                emit(registerSignal("totalInit"), delay);

                startTimesInit.erase(it1);
            }


        delete itUALCMPRequest->second;
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

    if(it == mecApplicationDescriptors_.end())
    {
        throw cRuntimeError("Application descriptor: %s not found", contAppMsg->getAppDId());
        //return;
    }
    const ApplicationDescriptor& appDesc = it->second;
    EV << "MEOApp:: appdesc " << appDesc.getAppName() << endl;

    EV << "DEBUG APPDEVID:  " << contAppMsg->getDevAppId() << endl;
    startTimesInit[contAppMsg->getDevAppId()] = simTime().dbl();

    inet::Packet* pktMM3 = new inet::Packet("instantiationApplicationRequest");
    auto instAppRequest = inet::makeShared<InstantiationApplicationRequest>();
    instAppRequest->setUeAppID(atoi(contAppMsg->getDevAppId()));
    instAppRequest->setMEModuleName(appDesc.getAppName().c_str());
    instAppRequest->setMEModuleType(appDesc.getAppProvider().c_str());
    if(appDesc.getAppDeploymentLocation().size() != 0)
        instAppRequest->setDeploymentLocation(appDesc.getAppDeploymentLocation().c_str());

    instAppRequest->setRequiredCpu(appDesc.getVirtualResources().cpu);
    instAppRequest->setRequiredRam(appDesc.getVirtualResources().ram);
    instAppRequest->setRequiredDisk(appDesc.getVirtualResources().disk);

    instAppRequest->setUeIpAddress(inet::L3Address(contAppMsg->getUeIpAddress()));


    auto it1 = migrationInfo.find(contAppMsg->getDevAppId());
    if (it1 != migrationInfo.end()) {
        std::string address = it1->second.first;
        int port = it1->second.second;
        EV << "DEBUG address=" << address << " port=" << port << endl;

        instAppRequest->setAddressMigration(address.c_str());
        instAppRequest->setPortMigration(port);
    }





    // insert OMNeT like services, only one is supported, for now
    if(!appDesc.getOmnetppServiceRequired().empty())
        instAppRequest->setRequiredService(appDesc.getOmnetppServiceRequired().c_str());
    else
        instAppRequest->setRequiredService("NULL");

    // insert ETSI std required services
    // get app descriptor
    std::vector<std::string> requiredServiceNames = appDesc.getAppServicesRequired();
    instAppRequest->setRequiredStandardServiceArraySize(requiredServiceNames.size());
    for(int i = 0; i < requiredServiceNames.size(); i++)
    {
        instAppRequest->setRequiredStandardService(i, requiredServiceNames[i].c_str());
    }

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

void MecOrchestratorApp::findBestMecHostFake(std::string deviceAppId, const ApplicationDescriptor& appDesc)
{
    EV << "MEOApp::findBestMecHost - alternating between MecHosts..." << endl;

    std::string key = deviceAppId;
    int totalHosts = mecHosts.size();
    if (totalHosts == 0) return;

    // ricerca in modo circolare a partire dal prossimo
    for (int i = 0; i < totalHosts; ++i) {
        int index = (lastUsedHostIndex + 1 + i) % totalHosts;
        auto& it = mecHosts[index];
        if (it->vimPort != -1 && it->mepmPort != -1) {
            lastUsedHostIndex = index; // aggiorna l'ultimo usato

            EV << "MEOApp::Using MECHost - id: " << it->mecHostId << endl;
            MECHostResponseEntry *responseEntry = new MECHostResponseEntry;
            responseEntry->mecHostID = it->mecHostId;

            ResourceRequest *r = new ResourceRequest();
            inet::Packet* pktMM3 = makeAvailableServiceRequestPacket(it->mepmHostIp, it->mepmPort, deviceAppId, appDesc);
            inet::Packet* pktMM4 = makeResourceRequestPacket(it->vimHostIp, it->vimPort, deviceAppId,
                                                              appDesc.getVirtualResources().cpu,
                                                              appDesc.getVirtualResources().ram,
                                                              appDesc.getVirtualResources().disk);

            r->pktMM3 = pktMM3;
            r->pktMM4 = pktMM4;
            r->vimHostAddress = it->vimHostIp;
            r->mepmHostAddress = it->mepmHostIp;
            r->vimPort = it->vimPort;
            r->mepmPort = it->mepmPort;
            resourceRequestQueue_.push(r);
            responseEntry->requestTime = simTime().dbl();

            if (!processResourceRequest_->isScheduled())
                scheduleAt(simTime(), processResourceRequest_);

            responseMap[key].push_back(responseEntry);
            break; // termina dopo il primo valido trovato (round robin)
        }
    }
}

void MecOrchestratorApp::findMecHostByTargetId(std::string deviceAppId, const ApplicationDescriptor& appDesc, int target)
{
    EV << "MEOApp::findMecHostByTargetId - search based on given ID..." << endl;

    std::string key = deviceAppId;

    if (mecHosts.empty()) return;

    std::string targetHost;

    std::unordered_map<int, std::tuple<std::string, std::string, int>> gbNodeToAddressVimPort;

    std::string path = par("mapFile").stdstringValue();
    std::ifstream infile(path);
    if (!infile.is_open()) {
        throw cRuntimeError("Impossibile aprire il file di mappatura: %s", path.c_str());
    }

    int gbNode, port;
    std::string address, vim;
    while (infile >> gbNode >> address >> vim >> port) {
        gbNodeToAddressVimPort[gbNode] = std::make_tuple(address, vim, port);
    }


    EV << "Contenuto della mappa caricata:\n";
    for (const auto& [k, v] : gbNodeToAddressVimPort) {
        EV << "gbNode: " << k
           << ", address: " << std::get<0>(v)
           << ", vim: " << std::get<1>(v)
           << ", port: " << std::get<2>(v) << endl;
    }


    if (gbNodeToAddressVimPort.count(target)) {
        const auto& entry = gbNodeToAddressVimPort[target];
        targetHost = std::get<1>(entry);

    } else {
        EV_ERROR << "GBNode non trovato nella mappa: " << target << endl;
    }

    double now = simTime().dbl();

    for (auto& it : mecHosts)
    {
        inet::L3Address resolvedTarget = inet::L3AddressResolver().resolve(targetHost.c_str());
        EV << "DEBUG " << it->vimHostIp << " - " << resolvedTarget << endl;
        if (it->vimHostIp == resolvedTarget && it->vimPort != -1 && it->mepmPort != -1)
{
            EV << "MEOApp::Using MECHost - id: " << it->mecHostId << " matching address: " << targetHost << endl;

            MECHostResponseEntry *responseEntry = new MECHostResponseEntry;
            responseEntry->mecHostID = it->mecHostId;

            ResourceRequest *r = new ResourceRequest();
            inet::Packet* pktMM3 = makeAvailableServiceRequestPacket(it->mepmHostIp, it->mepmPort, deviceAppId, appDesc);
            inet::Packet* pktMM4 = makeResourceRequestPacket(it->vimHostIp, it->vimPort, deviceAppId,
                                                              appDesc.getVirtualResources().cpu,
                                                              appDesc.getVirtualResources().ram,
                                                              appDesc.getVirtualResources().disk);

            r->pktMM3 = pktMM3;
            r->pktMM4 = pktMM4;
            r->vimHostAddress = it->vimHostIp;
            r->mepmHostAddress = it->mepmHostIp;
            r->vimPort = it->vimPort;
            r->mepmPort = it->mepmPort;
            resourceRequestQueue_.push(r);
            responseEntry->requestTime = simTime().dbl();
            responseEntry->nRichiesta = now;

            if (!processResourceRequest_->isScheduled())
                scheduleAt(simTime(), processResourceRequest_);

            std::cout << "DEBUG: deviceAppId = \"" << key << "\"" << endl;

            startTimes[key] = now;

            responseMap[key].push_back(responseEntry);
            break;
        }
    }
}



//MECHostDescriptor* MecOrchestratorApp::findBestMecHost(const ApplicationDescriptor& appDesc)
void MecOrchestratorApp::findBestMecHost(std::string deviceAppId, const ApplicationDescriptor& appDesc)
{
    EV << "MEOApp::findBestMecHost - finding best MecHost..." << endl;


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
            ResourceRequest *r = new ResourceRequest();
            // mm3
            inet::Packet* pktMM3 = makeAvailableServiceRequestPacket(it->mepmHostIp, it->mepmPort, deviceAppId, appDesc);

            // mm4
            inet::Packet* pktMM4 = makeResourceRequestPacket(it->vimHostIp, it->vimPort, deviceAppId, appDesc.getVirtualResources().cpu, appDesc.getVirtualResources().ram, appDesc.getVirtualResources().disk);


            r->pktMM3 = pktMM3;
            r->pktMM4 = pktMM4;
            r->vimHostAddress = it->vimHostIp;
            r->mepmHostAddress = it->mepmHostIp;
            r->vimPort = it->vimPort;
            r->mepmPort = it->mepmPort;
            resourceRequestQueue_.push(r);
            responseEntry->requestTime = simTime().dbl();
            if(!processResourceRequest_->isScheduled())
                scheduleAt(simTime(),processResourceRequest_);
//            responseEntry->requestTime = sendSRRequest(pktMM3, pktMM4, it->mepmHostIp, it->vimHostIp, it->vimPort, it->mepmPort);

            responseMap[key].push_back(responseEntry);

            //FRANCESCO MILIONE ERRORE
            break;
        }
    }

}

void MecOrchestratorApp::deployOnSpecifiedMecHost(std::string deviceAppId, const ApplicationDescriptor& appDesc, std::string location)
{

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
        printAvailableAppDescs();
        return mecApplicationDescriptors_[appDesc.getAppDId()];
}

//double MecOrchestratorApp::sendSRRequest(inet::Packet* pktMM3, inet::Packet* pktMM4, inet::L3Address mepmHostAddress, inet::L3Address vimHostAddress, int vimPort, int mepmPort)
void MecOrchestratorApp::sendSRRequest()
{

    ResourceRequest *r = dynamic_cast<ResourceRequest*>(resourceRequestQueue_.front());
    resourceRequestQueue_.pop();
    EV << "MEOApp::Sending requests to " << r->mepmHostAddress.str() << " and " << r->vimHostAddress.str() <<endl;

    // Requesting to vim if MECApp is allocable
    socket.sendTo(r->pktMM4, r->vimHostAddress, r->vimPort);

    // Requesting to MEPM if the MECPlatform has the needed services
    socket.sendTo(r->pktMM3, r->mepmHostAddress, r->mepmPort);


//    return simTime().dbl();
}

void MecOrchestratorApp::sendCreateAppContextAck(bool result, unsigned int requestSno, int contextId, const std::string &deviceAppId, std::string amsUri)
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
        ack->setAmsUri(amsUri.c_str());

    }
    else
    {
        ack->setSuccess(false);
        ack->setRequestId(requestSno);

        if(!deviceAppId.empty()){
            EV << "MEOApp::Deleting pending requests and responses"<<endl;
            // Deleting pending request from UALCMP
            auto entry = pendingRequests.find(deviceAppId);
            delete entry->second;
            pendingRequests.erase(deviceAppId);

            // Deleting pending response from MECHosts
            responseMap.erase(deviceAppId);
            //clearResponseMapByAppIdAndRequest(deviceAppId);


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

inet::Packet* MecOrchestratorApp::makeResourceRequestPacket(inet::L3Address dstAddress, int dstPort, std::string deviceAppId, double cpu, double ram, double disk)
{
    inet::Packet* pktMM4 = new inet::Packet("ResourceRequest");
    auto resourcePkt = inet::makeShared<MeoVimRequest>();
    resourcePkt->setDeviceAppId(deviceAppId.c_str());
    resourcePkt->setCpu(cpu);
    resourcePkt->setRam(ram);
    resourcePkt->setDisk(disk);
    resourcePkt->setDstAddress(dstAddress);
    resourcePkt->setDstPort(dstPort);
    resourcePkt->setChunkLength(inet::B(2000));
    pktMM4->insertAtBack(resourcePkt);

    return pktMM4;
}

inet::Packet* MecOrchestratorApp::makeAvailableServiceRequestPacket(inet::L3Address dstAddress, int dstPort, std::string deviceAppId, const ApplicationDescriptor& appDesc)
{
    inet::Packet* pktMM3 = new inet::Packet("ServiceRequest");
    std::vector<std::string> requiredServiceNames = appDesc.getAppServicesRequired();

    //std::copy(appDesc.getAppServicesRequired().begin(), appDesc.getAppServicesRequired().end(), serviceNames);

    auto servicePkt = inet::makeShared<MeoMepmRequest>();
    servicePkt->setDeviceAppId(deviceAppId.c_str());
    servicePkt->setRequiredServiceNamesArraySize(requiredServiceNames.size());
    // the index is needed to populate the other packet array
    // so we use the old fashioned way
    for(int i = 0; i < requiredServiceNames.size(); i++)
    {
        servicePkt->setRequiredServiceNames(i, requiredServiceNames[i].c_str());
    }
    servicePkt->setDstAddress(dstAddress);
    servicePkt->setDstPort(dstPort);
    servicePkt->setChunkLength(inet::B(2000));
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
        EV << "MEOApp::DeploymentSetting: " << it->second.getAppDeploymentSetting() << endl;
        EV << "MEOAPP::Required services:" << endl;
        std::vector<std::string> requiredServices = it->second.getAppServicesRequired();
        for(int i = 0; i < requiredServices.size(); i++)
        {
            EV << requiredServices[i] << endl;
        }
        EV << "------------------------------------" << endl;
    }
    EV << "####################################" << endl;

}

void MecOrchestratorApp::clearResponseMapByAppIdAndRequest(std::string deviceAppId)
{
    EV << "DEBUG: deviceAppId = \"" << deviceAppId << "\"" << endl;

    EV << "\n===== PRIMA DELLA CANCELLAZIONE =====" << endl;
        for (const auto& [key, vec] : responseMap) {
            EV << "Key: " << key << " | Size: " << vec.size() << endl;
            for (const auto& entry : vec) {
                EV << "  -> mecHostID: " << entry->mecHostID << ", nRichiesta: " << entry->nRichiesta << endl;
            }
        }

    int targetNRichiesta = -1;

    // Trova il primo nRichiesta associato a deviceAppId
    auto it = responseMap.find(deviceAppId);
    if (it != responseMap.end() && !it->second.empty()) {
        targetNRichiesta = it->second.front()->nRichiesta;
    }

    for (auto it = responseMap.begin(); it != responseMap.end(); )
    {
        // Verifica che la chiave contenga lo stesso deviceAppId
        if (it->first.find(deviceAppId) != std::string::npos) {
            bool match = true;
            for (auto entry : it->second) {
                // Se targetNRichiesta è -1, elimina solo quelli con -1 o non settato
                if (targetNRichiesta == -1) {
                    if (entry->nRichiesta != -1) {
                        match = false;
                        break;
                    }
                } else {
                    if (entry->nRichiesta != targetNRichiesta) {
                        match = false;
                        break;
                    }
                }
            }
            if (match) {
                it = responseMap.erase(it);
                continue;
            }
        }
        ++it;
    }

    EV << "\n===== DOPO LA CANCELLAZIONE =====" << endl;
        for (const auto& [key, vec] : responseMap) {
            EV << "Key: " << key << " | Size: " << vec.size() << endl;
            for (const auto& entry : vec) {
                EV << "  -> mecHostID: " << entry->mecHostID << ", nRichiesta: " << entry->nRichiesta << endl;
            }
        }
}

