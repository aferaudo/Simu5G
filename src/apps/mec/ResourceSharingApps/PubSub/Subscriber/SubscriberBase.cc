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

#include "SubscriberBase.h"

Define_Module(SubscriberBase);


SubscriberBase::SubscriberBase()
{
    subscriptionUri = "/broker/subscribe";
    webHook = "/newresources/"; // TODO should we load it from a NED File?
}

void SubscriberBase::initialize(int stage)
{
    if(stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    EV << "SubscriberBase::Initialization" << endl;

//    brokerPort = par("brokerPort");
//    localPort = par("localPort");
//
//
//    // remote connection
//    brokerIPAddress = inet::L3AddressResolver().resolve(par("brokerAddress").stringValue());
    inet::ApplicationBase::initialize(stage);

}

void SubscriberBase::handleStartOperation(inet::LifecycleOperation *operation)
{
    host = getParentModule();

    inet::IMobility *mod = check_and_cast<inet::IMobility *>(host->getSubmodule("mobility"));
    center = mod->getCurrentPosition();

    // Socket local binding
    tcpSocket.setOutputGate(gate("socketBrokerOut"));
    tcpSocket.bind(localToBrokerPort);
    tcpSocket.setCallback(this);

    connectToBroker();
}

void SubscriberBase::handleMessageWhenUp(omnetpp::cMessage *msg)
{
    if(msg->arrivedOn("socketIn"))
    {
        if (tcpSocket.belongsToSocket(msg))
            tcpSocket.processMessage(msg);
    }
    else{
        throw cRuntimeError("Unknown message");
        delete msg;
    }

}

void SubscriberBase::connectToBroker()
{
    tcpSocket.renewSocket();
    if (brokerIPAddress.isUnspecified()) {
        EV_ERROR << "Connecting to " << brokerIPAddress << " port=" << brokerPort << ": cannot resolve destination address\n";
        throw cRuntimeError("Broker address is unspecified!");
    }
    else {
        EV << "Connecting to " << brokerIPAddress << " port=" << brokerPort << endl;
        tcpSocket.connect(brokerIPAddress, brokerPort);
    }
}

void SubscriberBase::socketEstablished(inet::TcpSocket *socket)
{
    EV << "SubscriberBase::connection established!" << endl;
    EV << "SubscriberBase::sendSubscription" << endl;
    sendSubscription();

}

void SubscriberBase::sendSubscription()
{
    EV << "SubscriberBase::subscriber to " << brokerIPAddress.str() << endl;
    nlohmann::json jsonBody = infoToJson();

    std::string serverHost = tcpSocket.getRemoteAddress().str() + ":" + std::to_string(tcpSocket.getRemotePort());

    Http::sendPostRequest(&tcpSocket, jsonBody.dump().c_str(), serverHost.c_str(), subscriptionUri.c_str());

}

nlohmann::json SubscriberBase::infoToJson()
{
    /*
     * Return a json object with
     * clientId: int
     * subscription: obj
     *
     * Subscription obj:
     *      coordinateX:
     *      coordinateY:
     *      coordinateZ: -> typically null
     *      radius
     * */

    nlohmann::json jsonObj = nlohmann::json::object();

    jsonObj["clientId"] = getId();
    jsonObj["clientWebhook"] = webHook;
    jsonObj["subscription"]["coordinateX"] = center.getX();
    jsonObj["subscription"]["coordinateY"] = center.getY();
    jsonObj["subscription"]["coordinateZ"] = center.getZ();
    jsonObj["subscription"]["radius"] = radius;

    return jsonObj;
}


