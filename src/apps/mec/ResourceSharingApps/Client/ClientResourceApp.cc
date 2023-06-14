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

simsignal_t ClientResourceApp::parkingReleased_ = registerSignal("parkingReleased");
ClientResourceApp::ClientResourceApp()
{
    // Messages initialisation
    currentHttpMessage = nullptr;
}

ClientResourceApp::~ClientResourceApp()
{
    // Free memory (cancelAndDelete)
    // delete currentHttpMessage;
    //delete tcpSocket;

}


void ClientResourceApp::initialize(int stage)
{
    if(stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    
    
    EV << "ClientResourceApp::initialization" << endl;

    // Get parameters
    startTime = par("startTime");
    parkTime = par("parkTime");
    if (parkTime <= SIMTIME_ZERO)
    {
        parkTime = SimTime(2, SIMTIME_S); //default 2 seconds
    }

    localPort = par("localPort");
    viPort = par("viPort");
    
    minReward = par("minReward");
    host = getParentModule();

    viApp = check_and_cast<VirtualisationInfrastructureApp*>(getParentModule()->getSubmodule("viApp"));

    // Available resources
    localResources.ram = host->par("localRam").doubleValue();
    localResources.disk = host->par("localDisk").doubleValue();
    localResources.cpu = host->par("localCpuSpeed").doubleValue();

    // Socket local binding
    tcpSocket.setOutputGate(gate("socketOut"));
    tcpSocket.bind(localPort);
    tcpSocket.setCallback(this);

    const char *lcAddress = par("localAddress").stringValue();
    localIPAddress = inet::L3AddressResolver().resolve(lcAddress);

    destPort = par("destPort");
    destIPAddress = inet::L3AddressResolver().resolve(par("destAddress").stringValue());

    EV << "ClientResourceApp::Client Address: " << localIPAddress.str() << endl;


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
    if (code >= 400)
    {
        EV << "ClientResourceApp::handleResponse - problem during the request: " << code <<endl;
        EV << "ClientResourceApp::nothing to do...closing socket";
        close();
    }

    switch(appState)
    {
        case INIT:
        {
            if (code == 200) // TODO is this a good way to distinguish between replies?
            {

                EV << "ClientResourceApp:: Body not parsed: " << response->getBody();
                nlohmann::json jsonResponseBody = nlohmann::json::parse(response->getBody());
                EV << "ClientResourceApp::handleResponse - jsonobject: " << jsonResponseBody << endl;
                // Apply some policy to Accept the reward
                std::string reward = processRewards(jsonResponseBody);
                if(!reward.empty())
                {
                    EV << "ClientResourceApp::handleResponse - reward accepted: " << reward << endl;
                    EV << "ClientResourceApp:: Available resources - ram: " << localResources.ram << ", cpu: " << localResources.cpu << ", storage: " << localResources.disk << endl;
                    choosenReward = reward;
                    sendRegisterRequest();
                }
                else
                {
                    EV << "ClientResourceApp::handleResponse - reward rejected" << endl;
                    // Close socket - no interaction needed anymore
                    close();

                }
            }
            else if (code == 201)
            {
                EV << "ClientResourceApp::handleResponse - registration OK!" << endl;
                // Car is going to park now, so socket can be closed and reopened when the car leaves the park
                close();


                simtime_t now = simTime(); // Time in which a car starts its parking period
                // Scheduling exit from park
                cMessage *msg = new cMessage("parkLeaving");
                scheduleAt(simTime()+parkTime, msg);
            }
            else
            {
                EV << "ClientResourceApp::handleResponse - response code not recognised " << endl;
                close();
            }
            break;
        }
        case RELEASING:
        {
            bool released = false;
            if(code == 200 || code == 204)
            {
                EV << "ClientResourceApp::handleResponse - resource release status: SUCCESS" << endl;
                // TODO should we do something else?
                released = true;
            }
            else
            {
                EV << "ClientResourceApp::handleResponse - resource release status: FAILED - client not found" << endl;
                // TODO should we do something else?
            }
            // Here socket should be closed;
            EV << "ClientResourceApp::handleResponse - closing socket..bye" << endl;
            close();

            if(released && viApp->getHostedAppNum() == 0){
                emit(parkingReleased_, getParentModule()->getName());
            }
            break;
        }
    }

}

void ClientResourceApp::handleSelfMessage(cMessage *msg){
    
    if(strcmp(msg->getName(), "connect") == 0)
    {
        appState = INIT;
        EV << "ClientResourceApp::connecting, state: " << appState << "..." << endl;

        //delete msg;
    }
    else if (strcmp(msg->getName(), "parkLeaving") == 0)
    {
        appState = RELEASING;
        EV << "ClientResourceApp::connecting, state: " << appState << "..." << endl;
    }

    connectToSRR();

//    else if (strcmp(msg->getName(), "resend") == 0)
//    {
//        //delete msg;
//        sendRewardRequest();
//
//    }
    delete msg;
}

State ClientResourceApp::getState(){
    return appState;
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

    std::string uri("/resourceRegisterApp/v1/rewardList/");

    std::string serverHost = tcpSocket.getRemoteAddress().str() + ":" + std::to_string(tcpSocket.getRemotePort());
    std::string params = ""; //no params needed
    if(tcpSocket.getState() == inet::TcpSocket::CONNECTED)
    {

        EV << "ClientResourceApp::Reward - sending http GET Request to " << serverHost << endl;
        Http::sendGetRequest(&tcpSocket, serverHost.c_str(), uri.c_str(), params.c_str());

    }else
    {
        // schedule another http request when connected
//        cMessage *msg = new cMessage("resend");
//        scheduleAt(simTime()+0.05, msg);
        EV << "ClientResourceApp::RewardRequest - not connected yet" << endl;
    }
}

void ClientResourceApp::sendRegisterRequest()
{
    EV << "ClientResourceApp::Sending Register request" << endl;

    std::string uri("/resourceRegisterApp/v1/availableResources/");

    std::string serverHost = tcpSocket.getRemoteAddress().str() + ":" + std::to_string(tcpSocket.getRemotePort());

    if(tcpSocket.getState() == inet::TcpSocket::CONNECTED)
    {
        nlohmann::json jsonBody = nlohmann::json::object();
        jsonBody["deviceInfo"]["deviceId"] = getParentModule()->getId();
        jsonBody["deviceInfo"]["reward"] = choosenReward;
        jsonBody["deviceInfo"]["ipAddress"] = localIPAddress.str();
        jsonBody["deviceInfo"]["resourceInfo"]["maxRam"] = localResources.ram;
        jsonBody["deviceInfo"]["resourceInfo"]["maxDisk"] = localResources.disk;
        jsonBody["deviceInfo"]["resourceInfo"]["maxCPU"] = localResources.cpu;
        jsonBody["deviceInfo"]["viPort"] = viPort;

        EV << "Request body " << jsonBody.dump() << endl;
        Http::sendPostRequest(&tcpSocket, jsonBody.dump().c_str(), serverHost.c_str(), uri.c_str());
    }
    else
    {
        EV_ERROR << "ClientResourceApp::sendRegisterRequest - internal error" << endl;
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
            if(tcpSocket.belongsToSocket(msg))
                tcpSocket.processMessage(msg);
        }

    }
}

void ClientResourceApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent){

    EV << "ClientResourceApp::Reply received from the server" << packet->peekData() << endl;

    std::vector<uint8_t> bytes =  packet->peekDataAsBytes()->getBytes();
    delete packet;

    std::string msg(bytes.begin(), bytes.end());

    bool res = Http::parseReceivedMsg(msg, &buffer, &currentHttpMessage);
    if(res)
    {
        if(currentHttpMessage->getType() == RESPONSE)
        {
            HttpResponseMessage * response = dynamic_cast<HttpResponseMessage*> (currentHttpMessage);
            handleResponse(response);
        }
        else
        {
            EV_ERROR << "ClientResourceApp::HTTP message not recognised" << endl;
        }
    }
    else
    {
        EV << "ClientResourceApp::Data arrived on socket (NO HTTP)" << endl;
        // schedule another http request when connected
//        cMessage *msg = new cMessage("resend");
//        scheduleAt(simTime()+0.05, msg);
    }

    // After response has been processed it can be deleted
    if(currentHttpMessage != nullptr)
    {
      currentHttpMessage = nullptr;
    }


}

void ClientResourceApp::socketEstablished(inet::TcpSocket *socket){

    EV << "ClientResourceApp::connection established sending a message";
    // First request to send: which is the reward system?
    switch(appState)
    {
        case INIT:
            sendRewardRequest();
            break;
        case RELEASING:
        {
            EV << "Release state behaviour to be defined!" << endl;
            sendReleaseMessage();
            break;
        }
        default:
            throw cRuntimeError("ClientResourceApp::Error in app state");
    }

}

void ClientResourceApp::close(){
    EV_INFO << "ClientResourceApp::close - closing socket " << endl;
    tcpSocket.close();
}

void ClientResourceApp::finish()
{
    if(tcpSocket.getState() == inet::TcpSocket::CONNECTED)
        close();

}

std::string ClientResourceApp::processRewards(nlohmann::json jsonResponseBody)
{
    std::string toReturn= "";

    int thresholdReward = minReward;
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

void ClientResourceApp::sendReleaseMessage()
{
    EV << "ClientResourceApp::Sending Reward request" << endl;

    std::string uri = "/resourceRegisterApp/v1/availableResources/" + std::to_string(getParentModule()->getId());
    std::string serverHost = tcpSocket.getRemoteAddress().str() + ":" + std::to_string(tcpSocket.getRemotePort());

    Http::sendDeleteRequest(&tcpSocket, serverHost.c_str(), uri.c_str());

}

