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

#include "MecOrchestratorApp.h"

Define_Module(MecOrchestratorApp);

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
    socket.setOutputGate(gate("socketOut"));
    const char *localAddress = par("localAddress");
    localIPAddress = *localAddress ? inet::L3AddressResolver().resolve(localAddress) : inet::L3Address();
    socket.bind(localIPAddress, par("localPort"));
    socket.setCallback(this);
}

void MecOrchestratorApp::handleMessageWhenUp(omnetpp::cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        EV << "MEOApp::ReceivedSelfMessage";
        delete msg;
    }
    else if (socket.belongsToSocket(msg))
    {
        socket.processMessage(msg);
    }
    else
    {
        EV << "MEOApp::Not recognised message!";
    }
}

void MecOrchestratorApp::socketDataArrived(inet::UdpSocket *socket, inet::Packet *packet)
{
    EV << "MEOApp::Arrived packet on UDP socket " << socket->getSocketId() << ", packet name: " << packet->getName()<< endl;

    if(std::strcmp(packet->getName(), "Registration") == 0)
    {
        auto data = packet->peekData<RegistrationPkt>();
        handleRegistration(data.get());
    }
}

void MecOrchestratorApp::handleRegistration(const RegistrationPkt *data)
{
    EV << "MEOApp::Received MEC Host registration " << endl;

    bool exist = false;
    int hostId = data->getHostId();
    auto entry = mecHosts.find(hostId);
    MECHostDescriptor *mecHost = nullptr;
    MECHostRegistrationEntry *registrationEntry;

    if(entry == mecHosts.end())
    {
        // Create a new MECHost
        registrationEntry = new MECHostRegistrationEntry;
        mecHost = &registrationEntry->mecHostDesc;
        mecHost->mecHostId = hostId;
        mecHost->mecHostIp = data->getSourceAddress();

    }
    else
    {
        exist = true;
        EV << "MEOApp::MECHost partially exists " << endl;
        // It means that part of the MECHost already exists
        // So we can complete the subscription
        // If already exists it means that this is the information we miss
        registrationEntry = entry->second;
        registrationEntry->information = COMPLETE;
        mecHost = &registrationEntry->mecHostDesc;

    }

    if(data->getType() == MM3)
    {
        EV << "MEOApp::Received MEPM registration" << endl;
        mecHost->mepmPort = data->getSourcePort();
    }
    else if(data->getType() == MM4)
    {
        EV << "MEOApp::Received VIM registration" << endl;
        mecHost->vimPort = data->getSourcePort();
    }
    else
    {
        cRuntimeError("MEOApp::handleRegistration - Errore Registration packet type = NONE");
    }

    if(!exist)
        mecHosts.insert(std::pair<int, MECHostRegistrationEntry *>(hostId,registrationEntry));

    printAvailableMECHosts();
}


void MecOrchestratorApp::printAvailableMECHosts()
{
    EV << "####MEOAPP::Available Resources ####" << endl;
    for(auto it = mecHosts.begin(); it != mecHosts.end(); ++it)
    {
        EV << it->second->toString() << endl;
        EV << "------------------------------------" << endl;
    }
    EV << "####################################" << endl;
}
