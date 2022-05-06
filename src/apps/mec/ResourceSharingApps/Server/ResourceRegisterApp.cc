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

/*
 *
 * @author Alessandro Calvio
 * @author Angelo Feraudo
 *
 * */

#include "ResourceRegisterApp.h"

#include "apps/mec/ResourceSharingApps/Server/ResourceRegisterThread.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"

Define_Module(ResourceRegisterApp);

ResourceRegisterApp::ResourceRegisterApp()
{
    serverSocket = nullptr;

    baseUri_ = "/resourceRegisterApp/v1/";

    // TODO fill reward map dynamically
    rewardMap_.insert(std::pair<std::string, int>("reward1", 10));
    rewardMap_.insert(std::pair<std::string, int>("reward2", 30));
    rewardMap_.insert(std::pair<std::string, int>("reward3", 15));

//    availableResources_.clear(); //operations on this map should be mutually exclusive
    clientRewards_.clear();
}

ResourceRegisterApp::~ResourceRegisterApp()
{
//    socketMap.deleteSockets();
//    availableResources_.clear();
    clientRewards_.clear();
    rewardMap_.clear();
    delete serverSocket;
}

void ResourceRegisterApp::initialize(int stage)
{
    EV << "ResourceRegisterApp::initialize stage " << stage << endl;
    if (stage == inet::INITSTAGE_LOCAL)
    {
        // Initialise lists and other parameters
        EV << "ResourceRegisterApp::initialize" << endl;
        startTime = par("startTime");
        localPort = par("localPort");

        // broker parameters
        localToBrokerPort = par("localBrokerPort");
        brokerPort = par("brokerPort");
        publishURI = std::string(par("publishURI").stringValue());
        EV << "ResourceSharingApp::publishURI - " << publishURI << endl;

        // Do we need other parameters?
    }
//    if(stage != inet::INITSTAGE_APPLICATION_LAYER)
//       return;

    inet::ApplicationBase::initialize(stage);
}

void ResourceRegisterApp::removeThread(ResourceRegisterThread *thread)
{
    // remove socket
    threadSet.erase(thread);
    socketMap.removeSocket(thread->getSocket());


    // remove thread object
    thread->deleteModule();

}

void ResourceRegisterApp::handleStartOperation(inet::LifecycleOperation *operation){

    const char *localAddress = par("localAddress");

    EV << "ResourceRegisterApp::handleStartOperation - local Address: " << localAddress << " port: " << localPort << endl;
    localIPAddress = inet::L3AddressResolver().resolve(localAddress);
    EV << "ResourceRegisterApp::handleStartOperation - local Address resolved: "<< localIPAddress << endl;

    EV << "ResourceRegisterApp::initialising broker addres" << endl;
    brokerIpAddress = inet::L3AddressResolver().resolve(par("brokerAddress"));

    host_ = baseUri_ + ":" + std::to_string(localPort);

    serverSocket = new inet::TcpSocket();
    serverSocket->setOutputGate(gate("socketOut"));
    serverSocket->bind(localIPAddress, localPort);
    serverSocket->setCallback(this); //set this as callback



    cMessage *msg = new cMessage("listen");
    scheduleAt(simTime()+startTime, msg);

    PublisherBase::handleStartOperation(operation);

}


void ResourceRegisterApp::handleMessageWhenUp(cMessage *msg)
{
    if(msg->isSelfMessage() && strcmp(msg->getName(), "listen") == 0)
    {
        // Start listening
        EV << "ResourceRegisterApp::handleMessage - listening on this Address: "<< localIPAddress << " and this port: " << localPort << endl;
        serverSocket->listen();
        delete msg;
    }
    else if(msg->isSelfMessage() && strcmp(msg->getName(), "connect") == 0)
    {
        delete msg;
        PublisherBase::connectToBroker();
    }
    else{
        if(msg->arrivedOn("socketIn"))
        {
            inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(socketMap.findSocketFor(msg));
            if (socket)
            {
                socket->processMessage(msg);
            }
            else if (serverSocket->belongsToSocket(msg))
            {
                serverSocket->processMessage(msg);
            }
            else
            {
                PublisherBase::handleMessageWhenUp(msg);
            }
        }
        else{
            throw cRuntimeError("Unknown message");
            delete msg;
        }
    }
}

void ResourceRegisterApp::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo){

    auto newSocket = new inet::TcpSocket(availableInfo);
    newSocket->setOutputGate(gate("socketOut")); //gate connection

    EV << "ResourceRegisterApp::AvailableInfo " << availableInfo << endl;

    EV << "ResourceRegisterApp::Creating a dedicated thread for socket " << newSocket->getSocketId() << endl;
    //ResourceRegisterThread *proc = new ResourceRegisterThread();

    const char *serverThreadClass = par("serverThreadClass"); //static
    cModuleType *moduleType = cModuleType::get(serverThreadClass);
    char name[80];
    sprintf(name, "thread_%i", newSocket->getSocketId());
    ResourceRegisterThread *proc = check_and_cast<ResourceRegisterThread *>(moduleType->create(name, this));
    proc->finalizeParameters();
    proc->callInitialize();

    proc->init(this, newSocket);
    newSocket->setCallback(proc);

    EV << "ResourceRegisterApp::Storing socket " << endl;
    socketMap.addSocket(newSocket);
    threadSet.insert(proc);


    serverSocket->accept(availableInfo->getNewSocketId());
    EV << "ResourceRegisterApp::SocketMap size " << socketMap.size() << ", id: " << newSocket->getSocketId() << ", port: " << newSocket->getRemotePort() << endl;


}

void ResourceRegisterApp::handleStopOperation(inet::LifecycleOperation *operation){
    for (auto thread : threadSet)
             thread->getSocket()->close();
    serverSocket->close();
}

void ResourceRegisterApp::handleCrashOperation(inet::LifecycleOperation *operation){
    // remove and delete threads
    while (!threadSet.empty()) {
        auto thread = *threadSet.begin();
        // TODO destroy!!!
        thread->getSocket()->close();
        removeThread(thread);
    }
    // TODO always?
    if (operation->getRootModule() != getContainingNode(this))
        serverSocket->destroy();
}


void ResourceRegisterApp::finish()
{
    // remove and delete threads
    while (!threadSet.empty())
        removeThread(*threadSet.begin());
}

void ResourceRegisterApp::refreshDisplay() const
{
    inet::ApplicationBase::refreshDisplay();

    char buf[32];
    sprintf(buf, "%d threads", socketMap.size());
    getDisplayString().setTagArg("t", 0, buf);
}

void ResourceRegisterApp::insertClientEntry(ClientEntry c)
{
    // For the amount of nodes we manage this may not be needed
    mtx_write.lock();
//    availableResources_.insert(std::pair<int, ClientEntry>(c.clientId, c));
    clientRewards_.insert(std::pair<int, std::string>(c.clientId, c.reward));
    nlohmann::json jsonBody = nlohmann::json::object();
    jsonBody["id"] = c.clientId;
    jsonBody["ipAddress"] = c.ipAddress.str();
    jsonBody["ram"] = c.resources.ram;
    jsonBody["disk"] = c.resources.disk;
    jsonBody["cpu"] = c.resources.cpu;
    jsonBody["viPort"] = c.viPort;
    publish("POST", jsonBody.dump().c_str());
    mtx_write.unlock();

    // Printing reward seleceted
//    printAvailableResources();
    printSelectedRewards();

}

void ResourceRegisterApp::deleteClientEntry(int clientId)
{
    mtx_write.lock();
//    availableResources_.erase(clientId);
    clientRewards_.erase(clientId);
    publish("DELETE", "{}", clientId);
    mtx_write.unlock();

    // Printing rewards selected
    printSelectedRewards();
}

void ResourceRegisterApp::publish(const char* type, const char *body, int id)
{

    std::string brokerHost = brokerIpAddress.str() + ":" + std::to_string(brokerPort);

    if(std::strcmp(type, "POST") == 0)
    {
        EV << "ResourceregisterApp:: publishing - " << body << endl;


        Http::sendPostRequest(&brokerSocket, body, brokerHost.c_str(), publishURI.c_str());
    }
    else if(std::strcmp(type, "DELETE") == 0)
    {
        if(id == -1)
        {
            EV_ERROR << "ResourceRegisterApp::cannot delete published content - id (-1)" << endl;
            return;
        }

        EV << "ResourceregisterApp:: publishing - delete " << id << endl;
        std::string uri = publishURI + std::to_string(id);
        Http::sendDeleteRequest(&brokerSocket, brokerHost.c_str(), uri.c_str());
    }
}

void ResourceRegisterApp::printSelectedRewards()
{
    EV << "ResourceRegisterApp::Rewards Selected" << endl;
    EV << "#################################################" << endl;
    std::map<int, std::string>::iterator it;
    for(it = clientRewards_.begin(); it != clientRewards_.end(); ++it)
    {
        EV << "Client id: " << std::to_string(it->first) << ", reward selected: " << it->second << endl;
    }
    EV << "#################################################" << endl;
}







