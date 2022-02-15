/*
 * ResourceRegisterThread.cc
 *
 */

#include "ResourceRegisterThread.h"
#include "nodes/mec/utils/httpUtils/json.hpp"



using namespace omnetpp;

Register_Class(ResourceRegisterThread);
Define_Module(ResourceRegisterThread);


ResourceRegisterThread::~ResourceRegisterThread()
{
    delete sock;
    //delete currentHttpMessage;
    //delete server;
}
void ResourceRegisterThread::dataArrived(inet::Packet *packet, bool urgent){

    EV << "ResourceRegisterThread::Received packet on socket: " << packet->peekData() << endl;
    packet->removeControlInfo();

    std::vector<uint8_t> bytes =  packet->peekDataAsBytes()->getBytes();
    delete packet;

    std::string msg(bytes.begin(), bytes.end());

    EV << "ResourceRegisterThread::Packet bytes: " << msg;

    bool res = Http::parseReceivedMsg(msg, &buffer, &currentHttpMessage);
    if(res)
    {
        currentHttpMessage->setSockId(sock->getSocketId());
        if(currentHttpMessage->getType() == REQUEST)
        {
           /*
            * Get Requests:
            *   - rewardList
            * Post Requests:
            *   - Register Resources
            *   - Release Resoures
            * */
            HttpRequestMessage *request = check_and_cast<HttpRequestMessage*>(currentHttpMessage);
            handleRequest(request);
        }
        else
        {
            delete currentHttpMessage;
        }

        // After request has been processed it can be deleted
        if(currentHttpMessage != nullptr)
        {
          currentHttpMessage = nullptr;
        }
    }



}

void ResourceRegisterThread::established(){
    EV << "connected" << endl;
}


void ResourceRegisterThread::failure(int code){
    EV << "ResourceRegisterThread::socket of " << getSocket()->getRemoteAddress() << " failed. Code: " << code << endl;
    server->removeThread(this);
}

void ResourceRegisterThread::peerClosed(){
    EV << "ResourceRegisterThread::Peer closed the connection " << sock->getRemoteAddress() << endl;
    sock->setState(inet::TcpSocket::PEER_CLOSED);
    sock->close();
}

void ResourceRegisterThread::socketClosed(inet::TcpSocket *socket)
{
    EV << "ResourceRegisterThread::socket-closed" << endl;
    sock->setState(inet::TcpSocket::CLOSED);
    server->removeThread(this);
}



void ResourceRegisterThread::refreshDisplay() const
{
    getDisplayString().setTagArg("t", 0, inet::TcpSocket::stateName(sock->getState()));
}

void ResourceRegisterThread::handleRequest(const HttpRequestMessage *currentRequestMessageServed){
    EV << "ResourceRegisterThread::handleRequest - request method " << currentRequestMessageServed->getMethod() << endl;

    if(std::strcmp(currentRequestMessageServed->getMethod(),"GET") == 0)
    {
        handleGETRequest(currentRequestMessageServed);
    }
    else if(std::strcmp(currentRequestMessageServed->getMethod(),"POST") == 0)
    {
        handlePOSTRequest(currentRequestMessageServed);
    }
    else if(std::strcmp(currentRequestMessageServed->getMethod(),"DELETE") == 0)
    {
        handleDELETERequest(currentRequestMessageServed);
    }
    else
    {
        handlePUTRequest(currentRequestMessageServed);
    }
}

void ResourceRegisterThread::handleGETRequest(const HttpRequestMessage *currentRequestMessageServed){
    std::string uri = currentRequestMessageServed->getUri();

    if(uri.compare(server->getBaseUri() + "rewardList/") == 0)
    {
        EV << "ResourceRegisterThread::handleGETRequest - reward requested" << endl;
        std::map<std::string, int> rewards = server->getRewardMap();

        std::map<std::string, int>::iterator it;

        // Create json body
        nlohmann::json jsonBody = nlohmann::json::object();
        for(it = rewards.begin(); it != rewards.end(); it ++){
            jsonBody["rewardList"][it->first.c_str()] = it->second;
        }

        EV << "ResourceRegisterThread::handleGETRequest - rewards: " << jsonBody << endl;
        // Send reply
        Http::send200Response(sock, jsonBody.dump().c_str());

    }
    else
    {
        // BAD URI
        Http::send404Response(sock); //no reply for this request
    }


}

void ResourceRegisterThread::handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed){
    std::string uri = currentRequestMessageServed->getUri();

    if(uri.compare(server->getBaseUri() + "availableResources/") == 0)
    {

        nlohmann::json jsonBody = nlohmann::json::parse(currentRequestMessageServed->getBody());
        EV << "ResourceRegisterThread::post-request: " << jsonBody << endl;

        ClientEntry entry;
        entry.clientId = jsonBody["deviceInfo"]["deviceId"];


//        std::string reward_str = jsonBody["deviceInfo"]["reward"];
//        entry.reward = reward_str.c_str();
        entry.reward = jsonBody["deviceInfo"]["reward"];

        std::string ipAddress_str = jsonBody["deviceInfo"]["ipAddress"];
        entry.ipAddress = inet::L3Address(ipAddress_str.c_str());

        entry.resources.ram = jsonBody["deviceInfo"]["resourceInfo"]["maxRam"];
        entry.resources.disk = jsonBody["deviceInfo"]["resourceInfo"]["maxDisk"];
        entry.resources.cpu = jsonBody["deviceInfo"]["resourceInfo"]["maxCPU"];
        entry.viPort = jsonBody["deviceInfo"]["viPort"];

        EV << "ResourceRegisterThread::post-request - saving resources " << std::to_string(entry.resources.disk) << endl;

        server->insertClientEntry(entry);
//
        std::pair<std::string, std::string> locHeader("Location: ", uri);
        // Should we add other parameter (e.g. VIM address)
        // Reply okay
        Http::send201Response(sock, "{DONE}", locHeader);



        // Send resources to the VIM (this operation should be managed by the initial server)
        // After this should we close the connection?

    }
    else
    {
        // BAD URI
        Http::send404Response(sock);
    }
}

void ResourceRegisterThread::handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed)
{
    // URI example: /resourceRegisterApp/v1/availableResources/256
    // We need the last code in order to update the map

    std::string uri = currentRequestMessageServed->getUri();
    std::string delimeter("availableResources");

    // valid uri: /resourceRegisterApp/v1/availableResources
    std::string uriToCheck = uri.substr(0, uri.find(delimeter) + delimeter.length());
    if(uriToCheck.compare(server->getBaseUri() + "availableResources") == 0)
    {
        uri.erase(0, uri.find(delimeter) + delimeter.length()+1);
        int clientId = std::atoi(uri.c_str());
        if(server->getClientRewards().find(clientId) != server->getClientRewards().end())
        {
            // client resources exist we can remove them
            server->deleteClientEntry(clientId);
            Http::send204Response(sock);
        }
        else
        {
            // Not found
            Http::send404Response(sock);
        }
    }
    else
    {
        // BAD URI
        Http::send400Response(sock);
    }
}
