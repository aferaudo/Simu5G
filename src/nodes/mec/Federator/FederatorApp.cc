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


#include "../Federator/FederatorApp.h"
#include <random>
#include "nodes/mec/Federator/Messages/MEFmessages_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"

#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"

Define_Module(FederatorApp);


void FederatorApp::initialize()
{
    EV << "FederatorApp inizializzato!\n";


    appName = par("appName").stdstringValue();
    systemRegistrer.clear();


    socket.setOutputGate(gate("socketOut"));

    cMessage *bindMsg = new cMessage("bindSocket");
    scheduleAt(simTime() + 1.0, bindMsg);

    socket.setCallback(this);

}

void FederatorApp::handleMessage(cMessage *msg){
    EV << "MEFApp:: message!";
    if (msg->isSelfMessage()) {
        if (strcmp(msg->getName(), "bindSocket") == 0) {
            // Ora esegui il bind dopo il ritardo
            const char *localAddress = par("localAddress");
            localIPAddress = *localAddress ? inet::L3AddressResolver().resolve(localAddress) : inet::L3Address();
            socket.bind(localIPAddress, par("localPort"));
            EV << "Socket bind effettuato con successo!" << localAddress << par("localPort") << endl;

            localAddress = par("MEOAddress");
            MEOAddress = *localAddress ? inet::L3AddressResolver().resolve(localAddress) : inet::L3Address();
            MEOPort = par("MEOPort");
        }

    }
    else if (socket.belongsToSocket(msg))
    {
        socket.processMessage(msg);
    }
    else{
        EV << "MEFApp::Not recognised message!";
    }
    delete msg;
}

std::string FederatorApp::generateUniqueId(){
    static const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<int> dist(0, sizeof(charset)-2);

    std::string id;

    for(int i=0; i<9; ++i){
        id += charset[dist(rng)];
    }

    return id;
}

void FederatorApp::socketDataArrived(inet::UdpSocket *socket, inet::Packet *packet)
{
    EV << "FederatorApp::Arrived packet on UDP socket " << socket->getSocketId() << ", packet name: " << packet->getName()<< endl;

    if(std::strcmp(packet->getName(), "Registration") == 0)
    {
        handleRegistration(packet);
    }
    else if(std::strcmp(packet->getName(), "Cancellation") == 0)
    {
        handleCancelRegistration(packet);
    }
    else if(std::strcmp(packet->getName(), "Update") == 0)
    {
        handleUpdateRegistration(packet);
    }
    else if(std::strcmp(packet->getName(), "MECSystemReq") == 0)
    {
        handleMECSystemReq(packet);
    }
    else if(std::strcmp(packet->getName(), "ForwardReqToMEF") == 0)
    {
        handleForwardReqToMEF(packet);
    }
    else if(std::strcmp(packet->getName(), "MECSystemInfoRes") == 0)
    {
        handleMECSystemInfoRes(packet);
    }
    else if(std::strcmp(packet->getName(), "MEFSystemInfoRes") == 0)
    {
        handleMEFSystemInfoRes(packet);
    }
    else if(std::strcmp(packet->getName(), "appRequestMEOtoMEF") == 0)
    {
        handleAppRequestMEOtoMEF(packet);
    }
    else if(std::strcmp(packet->getName(), "appRequestMEFtoMEF") == 0)
    {
        handleAppRequestMEFtoMEF(packet);
    }
    else if(std::strcmp(packet->getName(), "appResponseMEOtoMEF") == 0)
    {
        handleAppResponseMEOtoMEF(packet);
    }
    else if(std::strcmp(packet->getName(), "appResponseMEFtoMEF") == 0)
    {
        handleAppResponseMEFtoMEF(packet);
    }
    else if(std::strcmp(packet->getName(), "appMigrationRequestMEOtoMEF") == 0)
    {
        handleAppMigrationReqeustMEOtoMEF(packet);
    }
    else if(std::strcmp(packet->getName(), "appMigrationRequestMEFtoMEF") == 0)
    {
        handleAppMigrationReqeustMEFtoMEF(packet);
    }
    else
    {
        EV << "MEOApp::Not recognized packet!"<< endl;
    }

}

void FederatorApp::handleRegistration(inet::Packet *packet){

    EV << "FederatorApp::Received MEO federation registration " << endl;


    auto data = packet->peekData<SystemInfo>();
    std::string uniqueId;


    SystemInfoEntry* systemInfo = nullptr;

    for(auto &entry : systemRegistrer)
        {
            if(entry->systemId == data->getSystemId())
            {
                systemInfo = entry;
                break;
            }
        }

    if(systemInfo == nullptr){
        systemInfo = new SystemInfoEntry;
        uniqueId = generateUniqueId();

        systemInfo->systemId = data->getSystemId();
        systemInfo->systemName = data->getSystemName();
        systemInfo->systemProvider = data->getSystemProvider();

        systemInfo->uniqueId = uniqueId;

        systemRegistrer.push_back(systemInfo);
    }


    inet::Packet *packetResp = new inet::Packet("uniqueId");
    auto response = inet::makeShared<MsgId>();

    response->setId(systemInfo->uniqueId.c_str());

    response->setChunkLength(inet::B(64));

    packetResp->insertAtBack(response);


    int srcPort = 2000;

    //INVIA AL MEO
    socket.sendTo(packetResp, MEOAddress, MEOPort);

}

void FederatorApp::handleCancelRegistration(inet::Packet *packet){

    EV << "FederatorApp::Received MEO federation cancellation " << endl;


    auto data = packet->peekData<SystemInfo>();


    inet::Packet *ackPacket  = new inet::Packet("ACK");
    auto response = inet::makeShared<AckPk>();


    response->setAckId("ERROR");


    for (auto it = systemRegistrer.begin(); it != systemRegistrer.end(); ++it) {
        if ((*it)->systemId == data->getSystemId()) {
            EV << "FederatorApp::cancellation completed " << endl;

            delete *it;
            systemRegistrer.erase(it);
            response->setAckId("OK");
            break;
        }
    }


    response->setChunkLength(inet::B(32));
    ackPacket->insertAtBack(response);

    int srcPort = 2000;

    //INVIA AL MEO
    socket.sendTo(ackPacket, MEOAddress, MEOPort);
}

void FederatorApp::handleUpdateRegistration(inet::Packet *packet){

    EV << "FederatorApp::Received MEO federation update registration " << endl;


    auto data = packet->peekData<SystemInfo>();
    std::string uniqueId;


    inet::Packet *ackPacket  = new inet::Packet("ACK");
    auto response = inet::makeShared<AckPk>();


    response->setAckId("ERROR");


    for(auto &entry : systemRegistrer)
    {
        if(entry->systemId == data->getSystemId())
        {
            entry->systemId = data->getSystemId();
            entry->systemName = data->getSystemName();
            entry->systemProvider = data->getSystemProvider();

            response->setAckId("OK");
            break;
        }
    }

    response->setChunkLength(inet::B(32));
    ackPacket->insertAtBack(response);

    int srcPort = 2000;

    //INVIA AL MEO
    socket.sendTo(ackPacket, MEOAddress, MEOPort);

}

void FederatorApp::handleMECSystemReq(inet::Packet *packet){

    EV << "FederatorApp::Received from MEO request system information " << endl;


    auto data = packet->peekData<SystemInfo>();


    inet::Packet *request  = new inet::Packet("ForwardReqToMEF");
    auto response = inet::makeShared<SystemInfo>();

    response->setSystemId(data->getSystemId());
    response->setSystemName(data->getSystemName());
    response->setSystemProvider(data->getSystemProvider());
    response->setIpMefRequest(localIPAddress.str().c_str());

    response->setChunkLength(inet::B(64));
    request->insertAtBack(response);

    //INVIA A TUTTI I MEF
    const char *localAddress = par("localAddress");
    if( strcmp(localAddress, "federator1")==0){
        EV << "MEF::handleMECSystemReq - sending:  " << "federator2" << ":" << "1000" << endl;
        socket.sendTo(request, inet::L3AddressResolver().resolve("federator2"), 1000);
    }else{
        EV << "MEF::handleMECSystemReq - sending:  " << "federator1" << ":" << "1000" << endl;
        socket.sendTo(request, inet::L3AddressResolver().resolve("federator1"), 1000);
    }

}

void FederatorApp::handleForwardReqToMEF(inet::Packet *packet){

    EV << "FederatorApp::Received from MEF forward request system information " << endl;


    auto data = packet->peekData<SystemInfo>();

    bool notHave = true;

    inet::L3Address ip = packet->getTag<inet::L3AddressInd>()->getSrcAddress();

    for(auto &entry : systemRegistrer)
        {
            if(entry->systemId == data->getSystemId())
            {
                inet::Packet *request  = new inet::Packet("MECSystemInfoRes");
                auto response = inet::makeShared<SystemInfo>();

                response->setSystemId(entry->systemId.c_str());

                response->setIpMefRequest(data->getIpMefRequest());

                response->setChunkLength(inet::B(64));
                request->insertAtBack(response);

                notHave = false;

                //INVIA AL MEF
                socket.sendTo(request, ip, 1000);

                break;
            }
        }


    if(notHave){
        inet::Packet *request  = new inet::Packet("ReqMECSystemInfo");
        auto response = inet::makeShared<SystemInfo>();

        response->setSystemId(data->getSystemId());
        response->setSystemName(data->getSystemName());
        response->setSystemProvider(data->getSystemProvider());
        response->setIpMefRequest(data->getIpMefRequest());

        response->setChunkLength(inet::B(64));
        request->insertAtBack(response);

        //INVIA AL MEO
        socket.sendTo(request, MEOAddress, MEOPort);
    }


}

void FederatorApp::handleMECSystemInfoRes(inet::Packet *packet){

    EV << "FederatorApp::Received from MEF request system information and send at MEO " << endl;

    auto data = packet->peekData<SystemInfo>();

    inet::Packet *request  = new inet::Packet("MECSystemInfoRes");
    auto response = inet::makeShared<SystemInfo>();

    response->setSystemId(data->getSystemId());
    response->setSystemName(data->getSystemName());
    response->setSystemProvider(data->getSystemProvider());
    response->setIpMefRequest(data->getIpMefRequest());

    response->setChunkLength(inet::B(64));
    request->insertAtBack(response);
    //INVIA AL MEO
    socket.sendTo(request, MEOAddress, MEOPort);

}

void FederatorApp::handleMEFSystemInfoRes(inet::Packet *packet){

    EV << "FederatorApp::Received from MEO request system information and send at MEF " << endl;

    auto data = packet->peekData<SystemInfo>();

    inet::Packet *request  = new inet::Packet("MECSystemInfoRes");
    auto response = inet::makeShared<SystemInfo>();

    response->setSystemId(data->getSystemId());
    response->setSystemName(data->getSystemName());
    response->setSystemProvider(data->getSystemProvider());

    response->setChunkLength(inet::B(64));
    request->insertAtBack(response);

    inet::L3Address ip = inet::L3Address(data->getIpMefRequest());
    EV << ip << " " << data->getIpMefRequest() << endl;

    //INVIA AL MEF
    socket.sendTo(request, ip, 1000);

}



void FederatorApp::handleAppRequestMEOtoMEF(inet::Packet *contAppMsg){
    const char *localAddress = par("localAddress");

    auto data = contAppMsg->peekData<AppRequest>();

    //Inolto al MEF e aggiungo il mio indirizzo
    inet::Packet* pktdup = new inet::Packet("appRequestMEFtoMEF");
    auto request = inet::makeShared<AppRequest>();

    request->setAppId(data->getAppId());
    request->setAppName(data->getAppName());
    request->setIpRequest(data->getIpRequest());
    request->setPortRequest(data->getPortRequest());
    request->setIpMefRequest(localAddress);
    request->setHostId(data->getHostId());

    request->setChunkLength(inet::B(64));
    pktdup->insertAtBack(request);

    //INVIA A TUTTI I MEF
    if( strcmp(localAddress, "federator1")==0){
        EV << "MEF::handleAppRequestMEOtoMEF - sending to:  " << "federator2" << ":" << "1000" << endl;
        socket.sendTo(pktdup, inet::L3AddressResolver().resolve("federator2"), 1000);
    }else{
        EV << "MEF::handleAppRequestMEOtoMEF - sending to:  " << "federator1" << ":" << "1000" << endl;
        socket.sendTo(pktdup, inet::L3AddressResolver().resolve("federator1"), 1000);
    }

}

void FederatorApp::handleAppRequestMEFtoMEF(inet::Packet *contAppMsg){
    const char *localAddress = par("localAddress");

    auto data = contAppMsg->peekData<AppRequest>();

    //Inolto al MEF e aggiungo il mio indirizzo
    inet::Packet* pktdup = new inet::Packet("appRequestMEFtoMEO");
    auto request = inet::makeShared<AppRequest>();

    request->setAppId(data->getAppId());
    request->setAppName(data->getAppName());
    request->setIpRequest(data->getIpRequest());
    request->setPortRequest(data->getPortRequest());
    request->setIpMefRequest(data->getIpMefRequest());
    request->setHostId(data->getHostId());

    request->setChunkLength(inet::B(64));
    pktdup->insertAtBack(request);;

    EV << "MEF::handleAppRequestMEFtoMEF - sending to:  " << MEOAddress << ":" << MEOPort << endl;
    socket.sendTo(pktdup, MEOAddress, MEOPort);


}

void FederatorApp::handleAppResponseMEOtoMEF(inet::Packet *contAppMsg){
    auto data = contAppMsg->peekData<AppResponse>();

    inet::Packet* pktdup = new inet::Packet("appResponseMEFtoMEF");
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


    EV << "MEF::handleAppResponseMEOtoMEF - sending to:  " << data->getIpMefRequest() << ":" << "1000" << endl;
    socket.sendTo(pktdup, inet::L3AddressResolver().resolve(data->getIpMefRequest()), 1000);


}

void FederatorApp::handleAppResponseMEFtoMEF(inet::Packet *contAppMsg){
    auto data = contAppMsg->peekData<AppResponse>();

    inet::Packet* pktdup = new inet::Packet("appResponseMEFtoMEO");
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


    EV << "MEF::handleAppResponseMEFtoMEF - sending to:  " << MEOAddress << ":" << MEOPort << endl;
    socket.sendTo(pktdup, MEOAddress, MEOPort);


}



void FederatorApp::handleAppMigrationReqeustMEOtoMEF(inet::Packet *contAppMsg){
    const char *localAddress = par("localAddress");

    auto data = contAppMsg->peekData<AppMigrationRequest>();

    //Inolto al MEF e aggiungo il mio indirizzo
    inet::Packet* pktdup = new inet::Packet("appMigrationRequestMEFtoMEF");
    auto request = inet::makeShared<AppMigrationRequest>();

    request->setAppName(data->getAppName());
    request->setAppPackageSource(data->getAppPackageSource());
    request->setAppIsOnboarded(data->getAppIsOnboarded());
    request->setDevAppId(data->getDevAppId());
    request->setAppAddress(data->getAppAddress());
    request->setAppPort(data->getAppPort());

    request->setChunkLength(inet::B(64));
    pktdup->insertAtBack(request);

    //INVIA A TUTTI I MEF
    if( strcmp(localAddress, "federator1")==0){
        EV << "MEF::handleAppMigrationReqeustMEOtoMEF - sending to:  " << "federator2" << ":" << "1000" << endl;
        socket.sendTo(pktdup, inet::L3AddressResolver().resolve("federator2"), 1000);
    }else{
        EV << "MEF::handleAppMigrationReqeustMEOtoMEF - sending to:  " << "federator1" << ":" << "1000" << endl;
        socket.sendTo(pktdup, inet::L3AddressResolver().resolve("federator1"), 1000);
    }

}

void FederatorApp::handleAppMigrationReqeustMEFtoMEF(inet::Packet *contAppMsg){
    auto data = contAppMsg->peekData<AppMigrationRequest>();

    inet::Packet* pktdup = new inet::Packet("appMigrationRequestMEFtoMEO");
    auto request = inet::makeShared<AppMigrationRequest>();

    request->setAppName(data->getAppName());
    request->setAppPackageSource(data->getAppPackageSource());
    request->setAppIsOnboarded(data->getAppIsOnboarded());
    request->setDevAppId(data->getDevAppId());
    request->setAppAddress(data->getAppAddress());
    request->setAppPort(data->getAppPort());

    request->setChunkLength(inet::B(64));
    pktdup->insertAtBack(request);

    EV << "MEF::handleAppMigrationReqeustMEFtoMEF - sending to:  " << MEOAddress << ":" << MEOPort << endl;
    socket.sendTo(pktdup, MEOAddress, MEOPort);

}
