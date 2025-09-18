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


#include "../Federator/FederatorBrokerApp.h"
#include <random>
#include "nodes/mec/Federator/Messages/MEFmessages_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"

#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"

Define_Module(FederatorBrokerApp);


FederatorBrokerApp::FederatorBrokerApp() {

}

FederatorBrokerApp::~FederatorBrokerApp() {
}


void FederatorBrokerApp::initialize()
{
    EV << "FederatorBrokerApp inizializzato!\n";


    appName = par("appName").stdstringValue();
    federatorRegistry.clear();

    socket.setOutputGate(gate("socketOut"));

    cMessage *bindMsg = new cMessage("bindSocket");
    scheduleAt(simTime() + 0.01, bindMsg);

    socket.setCallback(this);


}

void FederatorBrokerApp::handleMessage(cMessage *msg){
    EV << "FederatorBrokerApp:: message!";
    if (msg->isSelfMessage()) {
        if (strcmp(msg->getName(), "bindSocket") == 0) {
            // Ora esegui il bind dopo il ritardo
            const char *localAddress = par("localAddress");
            localIPAddress = *localAddress ? inet::L3AddressResolver().resolve(localAddress) : inet::L3Address();
            socket.bind(localIPAddress, par("localPort"));
            EV << "Socket bind effettuato con successo!" << localAddress << par("localPort") << endl;
        }

        delete msg;

    }
    else if (socket.belongsToSocket(msg))
    {
        socket.processMessage(msg);
    }
    else{
        EV << "FederatorBrokerApp::Not recognised message! : " << msg->getName();

        delete msg;
    }

}

void FederatorBrokerApp::socketDataArrived(inet::UdpSocket *socket, inet::Packet *packet)
{
    EV << "FederatorBrokerApp::Arrived packet on UDP socket " << socket->getSocketId() << ", packet name: " << packet->getName()<< endl;

    if(std::strcmp(packet->getName(), "RegistrationFederator") == 0)
    {
        handleRegistration(packet);
    }else if(std::strcmp(packet->getName(), "appMigrationRequestMEFtoMEFB") == 0)
    {
        handleAppMigrationReqeustMEFtoMEFB(packet);
    }else if(std::strcmp(packet->getName(), "appRequestMEFtoMEFB") == 0)
    {
        handleAppRequestMEFtoMEFB(packet);
    }else if(std::strcmp(packet->getName(), "appResponseMEFtoMEFB") == 0)
    {
        handleAppResponseMEFtoMEFB(packet);
    }
    else if(std::strcmp(packet->getName(), "ackMEFtoMEFB") == 0)
    {
        handleAckMEFtoMEFB(packet);
    }
    else
    {
        EV << "FederatorBrokerApp::Not recognized packet!"<< endl;
    }

}

void FederatorBrokerApp::handleRegistration(inet::Packet *packet){

    EV << "FederatorBrokerApp::Received MEO federation registration " << endl;


    auto data = packet->peekData<FederatorInfo>();


    FederatorEntry* systemInfo = nullptr;

    for(auto &entry : federatorRegistry)
        {
            if(entry->address == data->getAddress())
            {
                return;
            }
        }


    systemInfo = new FederatorEntry;

    systemInfo->address = data->getAddress();
    systemInfo->port = data->getPort();


    federatorRegistry.push_back(systemInfo);
    emit(registerSignal("fedDiscovery"), simTime().dbl()-data->getTempo());

}


void FederatorBrokerApp::handleAppMigrationReqeustMEFtoMEFB(inet::Packet *contAppMsg){
    const char *localAddress = par("localAddress");

    auto data = contAppMsg->peekData<AppMigrationRequest>();

    //Inolto al MEF e aggiungo il mio indirizzo
    inet::Packet* pktdup = new inet::Packet("appMigrationRequestMEFBtoMEF");
    auto request = inet::makeShared<AppMigrationRequest>();

    request->setAppName(data->getAppName());
    request->setAppPackageSource(data->getAppPackageSource());
    request->setAppIsOnboarded(data->getAppIsOnboarded());
    request->setDevAppId(data->getDevAppId());
    request->setAppAddress(data->getAppAddress());
    request->setAppPort(data->getAppPort());
    request->setGbNode(data->getGbNode());
    request->setUeIpAddress(data->getUeIpAddress());
    request->setIpMefRequest(data->getIpMefRequest());
    request->setStartTime(data->getStartTime());

    request->setChunkLength(inet::B(1024));
    pktdup->insertAtBack(request);


    //INVIA A TUTTI

    for (auto entry : federatorRegistry) {
        if (entry->address == data->getIpMefRequest()) {
                continue;
            }
        EV << "MEFB::sending to federator at: " << entry->address << ":" << entry->port << endl;
        socket.sendTo(pktdup->dup(), inet::L3AddressResolver().resolve(entry->address.c_str()), entry->port);
    }



}


void FederatorBrokerApp::handleAppRequestMEFtoMEFB(inet::Packet *contAppMsg){
    auto data = contAppMsg->peekData<AppRequest>();

    inet::Packet* pktdup = new inet::Packet("appRequestMEFBtoMEF");
    auto request = inet::makeShared<AppRequest>();

    request->setAppId(data->getAppId());
    request->setAppName(data->getAppName());
    request->setIpRequest(data->getIpRequest());
    request->setPortRequest(data->getPortRequest());
    request->setIpMefRequest(data->getIpMefRequest());
    request->setHostId(data->getHostId());

    request->setChunkLength(inet::B(1024));
    pktdup->insertAtBack(request);;


    for (auto entry : federatorRegistry) {
        if (entry->address == data->getIpMefRequest()) {
            continue;
        }

        EV << "MEFB::handleAppRequestMEFtoMEFB: sending to federator at: " << entry->address << ":" << entry->port << endl;
        socket.sendTo(pktdup->dup(), inet::L3AddressResolver().resolve(entry->address.c_str()), entry->port);
    }


}

void FederatorBrokerApp::handleAppResponseMEFtoMEFB(inet::Packet *contAppMsg){
    auto data = contAppMsg->peekData<AppResponse>();

    inet::Packet* pktdup = new inet::Packet("appResponseMEFBtoMEF");
    auto request = inet::makeShared<AppResponse>();

    request->setAppId(data->getAppId());
    request->setAppName(data->getAppName());
    request->setAppAddress(data->getAppAddress());
    request->setAppPort(data->getAppPort());
    request->setIpRequest(data->getIpRequest());
    request->setPortRequest(data->getPortRequest());
    request->setIpMefRequest(data->getIpMefRequest());
    request->setHostId(data->getHostId());

    request->setChunkLength(inet::B(1024));
    pktdup->insertAtBack(request);




    EV << "MEF::handleAppResponseMEOtoMEF - sending to:  " << data->getIpMefRequest() << ":" << "1000" << endl;
    socket.sendTo(pktdup, inet::L3AddressResolver().resolve(data->getIpMefRequest()), 1000);


}

void FederatorBrokerApp::handleAckMEFtoMEFB(inet::Packet *contAppMsg){
    auto data = contAppMsg->peekData<AckMigration>();

    inet::Packet* pktdup = new inet::Packet("ackMEFBtoMEF");
    auto request = inet::makeShared<AckMigration>();

    request->setAppId(data->getAppId());
    request->setIpMefRequest(data->getIpMefRequest());

    request->setChunkLength(inet::B(1024));
    pktdup->insertAtBack(request);


    EV << "MEFB::handleAckMEFtoMEFB - sending to:  " << data->getIpMefRequest() << ":" << "1000" << endl;
    socket.sendTo(pktdup, inet::L3AddressResolver().resolve(data->getIpMefRequest()), 1000);

}


