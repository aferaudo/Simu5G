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

#include "ExtMECWarningAlertApp.h"

#include "nodes/mec/utils/httpUtils/json.hpp"

Define_Module(ExtMECWarningAlertApp);

using namespace omnetpp;

ExtMECWarningAlertApp::ExtMECWarningAlertApp()
{
}

ExtMECWarningAlertApp::~ExtMECWarningAlertApp()
{
}

void ExtMECWarningAlertApp::initialize(int stage)
{
    ExtMecAppBase::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    //retrieving parameters
    size_ = par("packetSize");

    // set Udp Socket
    ueSocket.setOutputGate(gate("socketOut"));

    localUePort = par("localUePort");
    ueSocket.bind(localUePort);

    EV << "ExtMECWarningAlertApp::initializing" << endl;
    mp1Socket_ = addNewSocket();

    MecServiceInfo *m1 = new MecServiceInfo();
    m1->name = "LocationService";
    MecServiceInfo *m2 = new MecServiceInfo();
    m2->name = "ApplicationMobilityService";

    // std MEC services needed
    servicesData_[0] = m1;
    servicesData_[1] = m2;


    // connect with the service registry
    cMessage *msg = new cMessage("connectMp1");
    scheduleAt(simTime() + 0, msg);
}

void ExtMECWarningAlertApp::handleSelfMessage(cMessage *msg)
{
    if(strcmp(msg->getName(), "connectMp1") == 0)
    {
        EV << "ExtMECWarningAlertApp::handleSelfMessage " << msg->getName() << endl;
        connect(mp1Socket_, mp1Address, mp1Port);
    }

    delete msg;
}


void ExtMECWarningAlertApp::handleProcessedMessage(cMessage *msg)
{
    // TODO here we should manage ue messages
    ExtMecAppBase::handleProcessedMessage(msg);
}

void ExtMECWarningAlertApp::handleHttpMessage(int connId)
{
    // we should consider here, that we may have socket which are not related
    // to a communication with a MEC service
    // for example when we need to exchange the state with another app during migration
    if (connId == mp1Socket_->getSocketId())
    {
        handleMp1Message(connId);
        return;
    }
    int index = findServiceFromSocket(connId);
    if(index != -1)
    {
        handleServiceMessage(connId, index);
    }
    else
    {
        EV_INFO << "ExtMECWarningAlertApp::message not recognised as service -- This case has not been implemented yet!" << endl;
    }

}

void ExtMECWarningAlertApp::handleServiceMessage(int connId, int index)
{
    inet::TcpSocket *serviceSocket_ = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(connId));
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) serviceSocket_->getUserData();
    servicesData_[index]->currentHttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();

    if(std::strcmp(servicesData_[index]->name.c_str(), "LocationService") == 0)
    {
        handleLSMessage(serviceSocket_, index);
    }
    else
    {
        // case Application mobility service
        handleAMSMessage(serviceSocket_, index);
    }

}

void ExtMECWarningAlertApp::handleMp1Message(int connId)
{
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) mp1Socket_->getUserData();
    mp1HttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();

    EV << "ExtMECWarningAlertApp::handleMp1Message - payload: " << mp1HttpMessage->getBody() << endl;
    try
    {
        nlohmann::json jsonBody = nlohmann::json::parse(mp1HttpMessage->getBody()); // get the JSON structure
        if(!jsonBody.empty())
        {
            jsonBody = jsonBody[0];
            std::string serName = jsonBody["serName"];
            if(serName.compare("LocationService") == 0)
            {
                EV << "ExtMECWarningAlertApp::Location service info arrived from service registry" << endl;
                if(jsonBody.contains("transportInfo"))
                {
                    nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                    EV << "address: " << endPoint["host"] << " port: " <<  endPoint["port"] << endl;
                    std::string address = endPoint["host"];

                    // Adding service
                    servicesData_[0]->host.addr = inet::L3AddressResolver().resolve(address.c_str());
                    servicesData_[0]->host.port = endPoint["port"];
                    inet::TcpSocket *serviceSocket = addNewSocket();
//                    serviceNamesSocketMap_[serviceSocket->getSocketId()] = servicesData_[0]->name;
                }
            }
            else if(serName.compare("ApplicationMobilityService") == 0)
            {
                EV << "ExtMECWarningAlertApp::Location service info arrived from service registry" << endl;
                if(jsonBody.contains("transportInfo"))
                {
                    nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                    EV << "address: " << endPoint["host"] << " port: " <<  endPoint["port"] << endl;
                    std::string address = endPoint["host"];

                    servicesData_[1]->host.addr = inet::L3AddressResolver().resolve(address.c_str());
                    servicesData_[1]->host.port = endPoint["port"];
                    inet::TcpSocket *serviceSocket = addNewSocket();
//                    serviceNamesSocketMap_[serviceSocket->getSocketId()] = servicesData_[1]->name;
                }
            }
            else
            {
                EV << "MECWarningAlertApp::handleMp1Message - LocationService not found"<< endl;
//                serviceAddress = L3Address();
            }
        }

    }
    catch(nlohmann::detail::parse_error e)
    {
        EV <<  e.what() << std::endl;
        // body is not correctly formatted in JSON, manage it
        return;
    }
}

void ExtMECWarningAlertApp::handleUeMessage(omnetpp::cMessage *msg)
{
}

void ExtMECWarningAlertApp::modifySubscription()
{
}

void ExtMECWarningAlertApp::sendSubscription()
{
}

void ExtMECWarningAlertApp::sendDeleteSubscription()
{
}

void ExtMECWarningAlertApp::established(int connId)
{
    if(connId == mp1Socket_->getSocketId())
    {
        EV << "ExtMECWarningAlertApp::established connection with Service Registry" << endl;
        bool res = ExtMecAppBase::getInfoFromServiceRegistry();
        if(!res)
        {
            EV << "ExtMECWarningAlertApp::service not found" << endl;
        }
        return;
    }
}

