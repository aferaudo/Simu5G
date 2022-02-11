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

#include "PublisherBase.h"


PublisherBase::PublisherBase()
{
    publishURI = "/broker/publish/";
    currentHttpMessage = nullptr;
}

void PublisherBase::initialize(int stage)
{
    inet::ApplicationBase::initialize(stage);
}

void PublisherBase::handleStartOperation(inet::LifecycleOperation *operation)
{
    // open socket with broker
    EV << "PublisherBase::local binding broker socket" << endl;

    brokerSocket.setOutputGate(gate("socketOut"));
    brokerSocket.bind(localToBrokerPort);
    brokerSocket.setCallback(this);

    cMessage* msg = new cMessage("connect");
    scheduleAt(simTime(), msg);
}


void PublisherBase::handleMessageWhenUp(omnetpp::cMessage *msg)
{
    if(brokerSocket.belongsToSocket(msg))
        brokerSocket.processMessage(msg);
    else{
        throw cRuntimeError("PublisherBase::Unknown message");
        delete msg;
    }
}

void PublisherBase::socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent)
{
    EV << "PublisherBase::Reply received from the server" << packet->peekData() << endl;

    std::vector<uint8_t> bytes =  packet->peekDataAsBytes()->getBytes();
    delete packet;

    std::string msg(bytes.begin(), bytes.end());

    bool res = Http::parseReceivedMsg(msg, &buffer, &currentHttpMessage);
    if(res)
    {
        if(currentHttpMessage->getType() == RESPONSE)
        {
            HttpResponseMessage * response = dynamic_cast<HttpResponseMessage*> (currentHttpMessage);
            if(response->getCode() == 200)
            {
                EV << "PublisherBase::message published correctly!!"<< endl;
            }
            else if(response->getCode() == 204)
            {
                EV << "PublisherBase::delete published correctly!!" << endl;
            }
            else
            {
                EV << "Error in publishg in message - http code: " << response->getCode() << endl;
            }
        }
        else
        {
            EV_ERROR << "PublisherBase::HTTP message not recognised" << endl;
        }
    }
    else
    {
        EV << "PublisherBase::Error in response processing" << endl;
    }
}


void PublisherBase::connectToBroker()
{
    brokerSocket.renewSocket();
    if (brokerIpAddress.isUnspecified()) {
        EV_ERROR << "PublisherBase::Connecting to " << brokerIpAddress << " port=" << brokerPort << ": cannot resolve destination address\n";
        throw cRuntimeError("Broker address is unspecified!");
    }
    else {
        EV << "PublisherBase::Connecting to " << brokerIpAddress << " port=" << brokerPort << endl;
        brokerSocket.connect(brokerIpAddress, brokerPort);
    }
}
