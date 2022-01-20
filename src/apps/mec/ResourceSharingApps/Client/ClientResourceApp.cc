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

#include "ClientResourceApp.h"

// simu5g http utils
#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"

#include "inet/applications/tcpapp/GenericAppMsg_m.h"
#include "inet/common/TimeTag_m.h"


Define_Module(ClientResourceApp);

ClientResourceApp::ClientResourceApp()
{
    // Messages initialisation
}

ClientResourceApp::~ClientResourceApp()
{
    // Free memory (cancelAndDelete)

    //delete tcpSocket;
}


void ClientResourceApp::initialize(int stage)
{
    if(stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    
    
    EV << "ClientResourceApp::initialization" << endl;

    // Get parameters
    startTime = par("startTime");
    stopTime = par("stopTime");

    localPort = par("localPort");
    
    // Socket local binding
    tcpSocket.setOutputGate(gate("socketOut"));
    tcpSocket.bind(localPort);
    tcpSocket.setCallback(this);

    const char *lcAddress = par("localAddress").stringValue();
    localIPAddress = inet::L3AddressResolver().resolve(lcAddress);

    destPort = par("destPort");
    destIPAddress = inet::L3AddressResolver().resolve(par("destAddress").stringValue());

    EV << "ClientResourceApp::Client Address: " << localIPAddress.str() << endl;

    if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
        throw cRuntimeError("Invalid startTime/stopTime parameters");

    cMessage *msg = new cMessage("connect");
    scheduleAt(simTime()+startTime, msg);
}


void ClientResourceApp::handleSelfMessage(cMessage *msg){
    
    if(strcmp(msg->getName(), "connect") == 0)
    {
        EV << "ClientResourceApp::connecting...";
        connectToSRR();
        delete msg;
    }else if (strcmp(msg->getName(), "send") == 0)
    {
        delete msg;

        long requestLength = par("requestLength");
        long replyLength = par("replyLength");
        EV << "ClientResourceApp::TCP SOCKET STATE: " << tcpSocket.getState() << endl;
        EV << "ClientResourceApp::sending a message..." << endl;
        const auto& payload = inet::makeShared<inet::GenericAppMsg>();
        inet::Packet *packet = new inet::Packet(localIPAddress.str().data());
        payload->setChunkLength(inet::B(requestLength));
        payload->setExpectedReplyLength(inet::B(replyLength));
        payload->setServerClose(false);
        payload->addTag<inet::CreationTimeTag>()->setCreationTime(simTime());

        packet->insertAtBack(payload);
        tcpSocket.send(packet);
        EV << "ClientResourceApp::MESSAGE SENT! <--------------------------------"<< endl;

    }
}

// This method opens a tcp socket with the Server Resource Register
void ClientResourceApp::connectToSRR()
{
    tcpSocket.renewSocket();

    if (destIPAddress.isUnspecified()) {
        EV_ERROR << "Connecting to " << destIPAddress << " port=" << destPort << ": cannot resolve destination address\n";
        throw cRuntimeError("Server Resource Register address is unspecified!");
    }
    else {
        EV << "Connecting to " << destIPAddress << " port=" << destPort << endl;
        tcpSocket.connect(destIPAddress, destPort);
//        // For testing: After a socket has been opened, a message containing the localIPAddress is sent
//        // Consider that we need to wait until the socket is opened
//        cMessage *msg = new cMessage("send");
//        scheduleAt(simTime()+0.05, msg);
    }

}

void ClientResourceApp::sendRewardRequest(){
    EV << "ClientResourceApp::Sending Reward request" << endl;

    std::string uri("/resourceRegisterApp/v1/reward_list");

    std::string host = tcpSocket.getRemoteAddress().str() + ":" + std::to_string(tcpSocket.getRemotePort());
    std::string params = ""; //no params needed
    if(tcpSocket.getState() == inet::TcpSocket::CONNECTED)
    {
        EV << "ClientResourceApp::Connected - sending httpRequest to " << host << endl;
        Http::sendGetRequest(&tcpSocket, host.c_str(), uri.c_str(), params.c_str());
    }else
    {
        // schedule another http request when connected
    }
}


void ClientResourceApp::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage())
    {
        handleSelfMessage(msg);
    }
    else
    {
        if(msg->arrivedOn("socketIn"))
        {
            ASSERT(tcpSocket && tcpSocket.belongsToSocket(msg));
            tcpSocket.processMessage(msg);
        }

    }
}

void ClientResourceApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent){

    EV << "ClientResourceApp::Reply received from the server" << packet->peekData() << endl;
}

void ClientResourceApp::socketEstablished(inet::TcpSocket *socket){

    EV << "ClientResourceApp::connection established sending a message";
    // First request to send: which is the reward system?
    sendRewardRequest();
}
