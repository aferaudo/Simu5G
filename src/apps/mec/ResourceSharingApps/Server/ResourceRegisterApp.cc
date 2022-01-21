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
    // currentHttpMessage = nullptr;
    baseUri_ = "/resourceRegisterApp/v1/";
    // TODO fill this dynamically
    rewardMap_.insert(std::pair<std::string, int>("reward1", 10));
    rewardMap_.insert(std::pair<std::string, int>("reward2", 30));
    rewardMap_.insert(std::pair<std::string, int>("reward3", 15));
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

        // Do we need other parameters?
    }
//    if(stage != inet::INITSTAGE_APPLICATION_LAYER)
//       return;

    inet::ApplicationBase::initialize(stage);
}

void ResourceRegisterApp::removeThread(ResourceRegisterThread *thread)
{
    // remove socket
    socketMap.removeSocket(thread->getSocket());
    threadSet.erase(thread);

    // remove thread object
    thread->deleteModule();
}

void ResourceRegisterApp::handleStartOperation(inet::LifecycleOperation *operation){

    const char *localAddress = par("localAddress");

    EV << "ResourceRegisterApp::handleStartOperation - local Address: " << localAddress << " port: " << localPort << endl;
    localIPAddress = inet::L3AddressResolver().resolve(localAddress);
    EV << "ResourceRegisterApp::handleStartOperation - local Address resolved: "<< localIPAddress << endl;

    host_ = baseUri_ + ":" + std::to_string(localPort);

    serverSocket = new inet::TcpSocket();
    serverSocket->setOutputGate(gate("socketOut"));
    serverSocket->bind(localIPAddress, localPort);
    serverSocket->setCallback(this); //set this as callback



    cMessage *msg = new cMessage("listen");
    scheduleAt(simTime()+startTime, msg);

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
    else{
        if(msg->arrivedOn("socketIn"))
        {
            inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(socketMap.findSocketFor(msg));
            if (socket)
                socket->processMessage(msg);
            else if (serverSocket->belongsToSocket(msg))
                serverSocket->processMessage(msg);
        }
        else
            throw cRuntimeError("Unknown message");
    }
}

void ResourceRegisterApp::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo){

    auto newSocket = new inet::TcpSocket(availableInfo);
    newSocket->setOutputGate(gate("socketOut")); //gate connection
    //newSocket->setCallback(this); // for now is this but it must be a thread!!!

    EV << "ResourceRegisterApp::Creating a dedicated thread for socket " << newSocket->getSocketId() << endl;
    ResourceRegisterThread *proc = new ResourceRegisterThread();
    proc->init(this, newSocket);
    newSocket->setCallback(proc);

    EV << "ResourceRegisterApp::Storing socket " << endl;
    socketMap.addSocket(newSocket);


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








