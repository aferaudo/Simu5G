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

#include "inet/applications/tcpapp/GenericAppMsg_m.h"
#include "inet/common/TimeTag_m.h"


Define_Module(ClientResourceApp);

ClientResourceApp::ClientResourceApp()
{
    // Messages initialisation
    currentHttpMessage = nullptr;
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

void ClientResourceApp::handleResponse(HttpResponseMessage* response)
{
    // Manage Replies
    // Reward Reply
    // Success Registration reply
    // Error reply
    int code = response->getCode();

    if(code >= 400)
    {
        EV << "ClientResourceApp::handleResponse - Received reply with code: " << code << endl;
    }
    else if (code == 200) // TODO is this a good way to distinguish between replies?
    {
        nlohmann::json jsonResponseBody = nlohmann::json::parse(response->getBody());
        EV << "ClientResourceApp::handleResponse - jsonobject: " << jsonResponseBody << endl;
        // Apply some policy to Accept the reward
        std::string reward = processRewards(jsonResponseBody);
        if(!reward.empty())
        {
            EV << "ClientResourceApp::handleResponse - reward accepted: " << reward << endl;
        }
        else
        {
            EV << "ClientResourceApp::handleResponse - reward rejected" << endl;
        }
    }
    else if (code == 201)
    {
        EV << "ClientResourceApp::handleResponse - registration OK!" << endl;
    }

}

void ClientResourceApp::handleSelfMessage(cMessage *msg){
    
    if(strcmp(msg->getName(), "connect") == 0)
    {
        EV << "ClientResourceApp::connecting...";
        connectToSRR();
        delete msg;
    }else if (strcmp(msg->getName(), "resend") == 0)
    {
        delete msg;
        sendRewardRequest();

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
        cMessage *msg = new cMessage("resend");
        scheduleAt(simTime()+0.05, msg);
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

    std::vector<uint8_t> bytes =  packet->peekDataAsBytes()->getBytes();
    std::string msg(bytes.begin(), bytes.end());

    bool res = Http::parseReceivedMsg(msg, &buffer, &currentHttpMessage);
    if(res)
    {
        if(currentHttpMessage->getType() == RESPONSE)
        {
            handleResponse(check_and_cast<HttpResponseMessage*>(currentHttpMessage));
        }
        else
        {
            EV_ERROR << "ClientResourceApp::HTTP message not recognised" << endl;
        }
    }
    else
    {
        EV << "ClientResourceApp::Data arrived on socket (NO HTTP)" << endl;
    }

    delete packet;
}

void ClientResourceApp::socketEstablished(inet::TcpSocket *socket){

    EV << "ClientResourceApp::connection established sending a message";
    // First request to send: which is the reward system?
    sendRewardRequest();
}

std::string ClientResourceApp::processRewards(nlohmann::json jsonResponseBody)
{
    std::string toReturn= "";

    int thresholdReward = 14; //TODO Load minimum threshold from a ned file
    nlohmann::json jsonRewardList = jsonResponseBody["rewardList"];

    for(nlohmann::json::iterator it = jsonRewardList.begin(); it != jsonRewardList.end(); ++it)
    {
        if(it.value() >= thresholdReward)
        {
            thresholdReward = it.value();
            toReturn = it.key();
        }
    }


    return toReturn;
}
