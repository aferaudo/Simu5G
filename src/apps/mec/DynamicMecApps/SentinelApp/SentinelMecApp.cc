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

#include "SentinelMecApp.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/FilterCriteriaAssocHo.h"

Define_Module(SentinelMecApp);

SentinelMecApp::~SentinelMecApp()
{
    cancelAndDelete(ueMonitorTimer_);
}

void SentinelMecApp::initialize(int stage)
{
    ExtMecAppBase::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;
    localPort = par("localPort");

    mp1Socket_ = addNewSocket();

    MecServiceInfo* serviceInfo = new MecServiceInfo();
    serviceInfo->name = ("RNIService");

    servicesData_.push_back(serviceInfo);

    webHook_ = "/cellChangeNotification_" + std::to_string(getId());

    ueMonitoringInterval_ = par("ueMonitoringInterval").doubleValue();
    ueMonitorTimer_ = new cMessage("ueMonitorTimer");

    cellIdRegistered_ = false;

    cMessage *msg = new cMessage("connectMp1");
    scheduleAt(simTime() + 0, msg);
}

void SentinelMecApp::handleSelfMessage(omnetpp::cMessage *msg)
{
    if(strcmp(msg->getName(), "connectMp1") == 0)
    {
       EV << "SentinelMecApp::handleSelfMessage " << msg->getName() << endl;
       connect(mp1Socket_, mp1Address, mp1Port);
    }
    else if(strcmp(msg->getName(), "connectService") == 0)
    {
        for(auto service : servicesData_)
        {
            if(!service->host.addr.isUnspecified())
            {
                inet::TcpSocket *serviceSocket  = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(service->sockid));
                if(serviceSocket != nullptr && serviceSocket->getState() != inet::TcpSocket::CONNECTED
                        && serviceSocket->getState() != inet::TcpSocket::CONNECTING)
                {
                    EV << "SentinelMecApp::connecting to the service: " << service->name << endl;
                    connect(serviceSocket, service->host.addr, service->host.port);
                }
                else
                {
                    EV_ERROR << "SentinelMecApp::handleSelfMessage - service socket is nullptr or already connected" << endl;
                }
            }
           else
           {

               std::cout << "SentinelMecApp::handleSelfMessage - service IP address is  unspecified (maybe response from the service registry is arriving)" << endl;
               if(!connectService_->isScheduled())
                   scheduleAt(simTime()+0.005, connectService_);

           }
        }
    }
    else if(strcmp(msg->getName(), "ueMonitorTimer") == 0)
    {
        EV << "SentinelMecApp::handleSelfMessage - monitoring the antennas" << endl;
        inet::TcpSocket *rniSocket = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(servicesData_[RNI]->sockid));
        sendGetL2Meas(rniSocket);
    }
    else
    {
        EV_ERROR << "SentinelMecApp::handleSelfMessage - message not recognized" << endl;
    }

}

void SentinelMecApp::handleHttpMessage(int connId)
{
    if (connId == mp1Socket_->getSocketId())
    {
        handleMp1Message(connId);
        return;
    }
    int index = findServiceFromSocket(connId);
    if(index != -1)
    {
        EV << "SentinelMecApp::handling service message" << endl;
        handleServiceMessage(index);
    }
    else
    {
        EV_INFO << "SentinelMecApp::handleHttpMessage - service not found" << endl;
    }
}

void SentinelMecApp::handleReceivedMessage(int sockId, inet::Packet *msg)
{
    // TODO - Generated method body
}

void SentinelMecApp::handleServiceMessage(int index)
{
    inet::TcpSocket *serviceSocket_ = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(servicesData_[index]->sockid));
    EV << "SentinelMecApp::handleServiceMessage - service name: " << servicesData_[index]->name << endl;
    if(std::strcmp(servicesData_[index]->name.c_str(), "RNIService") == 0)
    {
        handleRNIMessage(serviceSocket_);
    }
    else
    {
        EV_ERROR << "SentinelMecApp::Error in handleServiceMessage" << endl;
    }
}

void SentinelMecApp::handleMp1Message(int connId)
{
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) mp1Socket_->getUserData();
    mp1HttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();
    responseCounter_ --;
    try
    {
        nlohmann::json jsonBody = nlohmann::json::parse(mp1HttpMessage->getBody()); // get the JSON structure
        if(!jsonBody.empty())
        {
            jsonBody = jsonBody[0];
            std::string serName = jsonBody["serName"];
            EV << "SentinelMecApp::handleMp1Message - service found: " << serName << endl;
            if(jsonBody.contains("transportInfo"))
            {
                nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                EV << "SentinelMecApp:: address: " << endPoint["host"] << " port: " <<  endPoint["port"] << endl;
                std::string address = endPoint["host"];
                int port = endPoint["port"];
                if(serName.compare("RNIService") == 0)
                {
                    // Adding service
                    servicesData_[RNI]->host.addr = inet::L3AddressResolver().resolve(address.c_str());
                    servicesData_[RNI]->host.port = port;
                    EV << "SentinelMecApp::registered addr " << servicesData_[RNI]->host.addr << ", port " <<  servicesData_[RNI]->host.port << endl;
                    inet::TcpSocket *serviceSocket = addNewSocket();
                    servicesData_[RNI]->sockid = serviceSocket->getSocketId();

                    if(!connectService_->isScheduled())
                        scheduleAt(simTime() + 0.005, connectService_);
                }

                // other service
            }
        }
        else
        {
            EV << "SentinelMecApp::handleMp1Message - body is empty - service not found" << endl;
        }
    }
    catch(nlohmann::detail::parse_error e)
    {
        EV <<  e.what() << std::endl;
        // body is not correctly formatted in JSON, manage it
        return;
    }

}

void SentinelMecApp::handleRNIMessage(inet::TcpSocket *socket)
{
    HttpMessageStatus *msgStatus = (HttpMessageStatus*) socket->getUserData();
    HttpBaseMessage* serviceHttpMessage = (HttpBaseMessage*) msgStatus->httpMessageQueue.front();
    EV << "SentinelMecApp::handling RNI message " <<  serviceHttpMessage->getBody() << endl;
    if(serviceHttpMessage->getType() == REQUEST)
    {
        // Manage notification
    }
    else if(serviceHttpMessage->getType() == RESPONSE)
    {
        // Manage subscription response and polling
        EV << "SentinelMecApp::handling RNI response" << endl;
        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(serviceHttpMessage);
        if(rspMsg->getCode() == 200)
        {
            EV << "SentinelMecApp::handling RNI response - get L2 meas ok" << endl;
            // responseCounter_ --;
            nlohmann::json jsonBody = nlohmann::json::parse(rspMsg->getBody());
            EV << "SentinelMecApp::handling RNI response - get L2 meas body: " << jsonBody.dump().c_str() << endl;
            
            // Processing the response
            processL2MeasResponse(jsonBody);
            
            // Scheduling again the request
            if(!ueMonitorTimer_->isScheduled())
                scheduleAt(simTime()+ueMonitoringInterval_, ueMonitorTimer_);
        }
        else if(rspMsg->getCode() == 201)
        {
            EV << "SentinelMecApp::handling RNI response - subscription ok: " << rspMsg->getBody() << endl;
            nlohmann::json jsonBody = nlohmann::json::parse(rspMsg->getBody());
            if(!jsonBody.empty())
            {
                // Correct subscription
                std::stringstream stream;
                stream << "sub" << jsonBody["subscriptionId"];
                subId_ = stream.str();

                EV << "SentinelMecApp::handling RNI response - subscription id: " << subId_ << endl; 
            }
            
        }
        else if(rspMsg->getCode() == 204)
        {
            EV << "SentinelMecApp::handling RNI response - delete subscription ok" << endl;
            responseCounter_ --;
        }
        else
        {
            EV << "SentinelMecApp::handling RNI response - not recognized code error: " << rspMsg->getCode() << ", body:\n" << rspMsg->getBody() << endl;
        }


    }

}

void SentinelMecApp::handleUeMessage(omnetpp::cMessage *msg)
{
    // TODO - Generated method body
}

void SentinelMecApp::established(int connId)
{
    if(connId == mp1Socket_->getSocketId())
    {
        bool res = ExtMecAppBase::getInfoFromServiceRegistry();
        if(!res)
        {
            EV << "SentinelMecApp::service not found" << endl;
        }
        EV << "Everything is ok" << endl;
        return;
    }
    int index = findServiceFromSocket(connId);
    inet::TcpSocket *socket_ = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(connId));
    if(index != -1)
    {
        if(std::strcmp(servicesData_[index]->name.c_str(), "RNIService") == 0)
        {
            EV << "SentinelMecApp::established - connected to RNIService" << endl;
            sendGetL2Meas(socket_);
            // sendCellChangeSubscription(socket_);
        }

    }
    else
    {
        EV_ERROR << "SentinelMecApp::established - socket for service not found" << endl;
    }   

}

void SentinelMecApp::handleTermination()
{
    inet::TcpSocket *rniSocket = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(servicesData_[RNI]->sockid));

    // delete subscription
    sendDeleteCellChangeSubscription(rniSocket);


    scheduleAt(simTime()+0.01, terminationMessage_);
    
}

void SentinelMecApp::sendCellChangeSubscription(inet::TcpSocket *socket)
{
    std::string uristring = "/example/rni/v2/subscriptions";
    std::string host = socket->getRemoteAddress().str()+":"+std::to_string(socket->getRemotePort());

    nlohmann::ordered_json subscriptionBody_;
    subscriptionBody_ = nlohmann::ordered_json();
    subscriptionBody_["subscriptionType"] = "CellChangeSubscription";
    subscriptionBody_["callbackReference"] = localAddress.str() + ":" + std::to_string(par("localUePort").intValue()) + webHook_;
    // subscriptionBody_["websockNotifConfig"] =
    subscriptionBody_["filterCriteriaAssocHo"]["appInstanceId"] = getName();
    subscriptionBody_["filterCriteriaAssocHo"]["associateId"] = nlohmann::ordered_json::array();
    subscriptionBody_["filterCriteriaAssocHo"]["hoStatus"] = nlohmann::ordered_json::array();
    subscriptionBody_["filterCriteriaAssocHo"]["hoStatus"].push_back(hoStatusString[IN_PREPARATION]); 
    // subscriptionBody_["filterCriteriaAssocHo"]["ecgi"] = nlohmann::ordered_json::array();
    subscriptionBody_["requestTestNotification"] = false;

    Http::sendPostRequest(socket, subscriptionBody_.dump().c_str(), host.c_str(), uristring.c_str());

}   

void SentinelMecApp::sendDeleteCellChangeSubscription(inet::TcpSocket *socket)
{
    EV << "SentinelMecApp::sendDeleteCellChangeSubscription" << endl;
    // responseCounter_ ++;
    std::string uristring = "/example/rni/v2/subscriptions/" + subId_;
    std::string host = socket->getRemoteAddress().str()+":"+std::to_string(socket->getRemotePort());

    Http::sendDeleteRequest(socket, host.c_str(), uristring.c_str());
}

void SentinelMecApp::sendGetL2Meas(inet::TcpSocket *socket)
{
    /*
    * In this case we need the infomation about all the antenna managed
    * by that mechost. So, no need to specify cellId or ueipv*address
    */
    responseCounter_ ++;
    EV << "SentinelMecApp::sendGetL2Meas" << endl;
    std::string uristring = "/example/rni/v2/queries/layer2_meas";
    std::string host = socket->getRemoteAddress().str()+":"+std::to_string(socket->getRemotePort());

    Http::sendGetRequest(socket, host.c_str(), uristring.c_str());
}

void SentinelMecApp::processL2MeasResponse(const nlohmann::ordered_json& json)
{
    EV << "SentinelMecApp::processL2MeasResponse - cellinfo: " << json["cellInfo"] <<  endl;
    
    if(!cellIdRegistered_)
    {
        for (const auto& cellInfo : json["cellInfo"]) {
            Plmn *plmn = new Plmn(cellInfo["ecgi"]["plmn"]["mcc"], cellInfo["ecgi"]["plmn"]["mnc"]);
            Ecgi *ecgi = new Ecgi(cellInfo["ecgi"]["cellId"], *plmn);
            cellIds_.push_back(ecgi);
        }
        cellIdRegistered_ = true;
    }

    EV << "SentinelMecApp::processL2MeasResponse - cellUEInfo: " << json["cellUEInfo"] <<  endl;

    for (const auto& ueInfo : json["cellUEInfo"]) {
        AssociateId a;

        a.setType(ueInfo["associatedId"]["type"]);
        a.setValue(ueInfo["associatedId"]["value"]);
        if(!doesUeExist(a))
        {
            EV << "SentinelMecApp::processL2MeasRespons - NEW UE FOUND" << endl;
            allUes_.push_back(a);
        }
    }

    // debug
    for (const auto& ue : allUes_) {
        EV << "SentinelMecApp::processL2MeasResponse - ue: " << ue.getType() << " " << ue.getValue() << endl;
    }

}

bool SentinelMecApp::doesUeExist(AssociateId& ue)
{
    for(auto a : allUes_)
    {
        if(a.getValue() == ue.getValue())
            return true;
    }
    
    return false;
}