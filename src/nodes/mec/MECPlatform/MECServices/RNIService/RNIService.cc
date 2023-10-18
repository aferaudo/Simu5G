//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "inet/transportlayer/contract/tcp/TcpCommand_m.h"
#include "inet/applications/tcpapp/GenericAppMsg_m.h"
#include <iostream>
#include "nodes/mec/MECPlatform/MECServices/RNIService/RNIService.h"

#include <string>
#include <vector>
//#include "apps/mec/MECServices/packets/HttpResponsePacket.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "common/utils/utils.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"

#include "nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/CellChangeSubscription.h"

#include "nodes/mec/MECPlatform/EventNotification/CellChangeEvent.h"

Define_Module(RNIService);


RNIService::RNIService():L2MeasResource_(){
    baseUriQueries_ = "/example/rni/v2/queries";
    baseUriSubscriptions_ = "/example/rni/v2/subscriptions";
    supportedQueryParams_.insert("cell_id");
    supportedQueryParams_.insert("ue_ipv4_address");
    // supportedQueryParams_s_.insert("ue_ipv6_address");
}

void RNIService::initialize(int stage)
{
    MecServiceBase::initialize(stage);

    if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        L2MeasResource_.addEnodeB(eNodeB_);
        baseSubscriptionLocation_ = host_+ baseUriSubscriptions_ + "/";
        antennaMonitorInterval_ = par("monitoringInterval").doubleValue();
        antennaMonitorMsg_ = antennaMonitorInterval_ > 0 ? new cMessage("antennaMonitorTimer") : nullptr;
    }
}

void RNIService::handleStartOperation(inet::LifecycleOperation *operation)
{
    MecServiceBase::handleStartOperation(operation);
    baseSubscriptionLocation_ = host_+ baseUriSubscriptions_;
}

void RNIService::handleSelfMessage(cMessage *msg)
{
    
    if(std::strcmp(msg->getName(), "antennaMonitorTimer") == 0)
    {
        // Recovering handover status from antenna for each subscription
        // (send notificaiton?)
        if(subscriptions_.size() > 0)
        {
            // TODO
            // 1. For each associate id of each subscriber, check if the ue is in handover
            // 2. If the ue is in handover, send a notification to the subscriber 
            for(auto subscriber : subscriptions_)
            {
                if(subscriber.second->getSubscriptionType() == "CellChangeSubscription")
                {
                    CellChangeSubscription *sub = check_and_cast<CellChangeSubscription*>(subscriber.second);
                    FilterCriteriaAssocHo *filters = check_and_cast<FilterCriteriaAssocHo*>(sub->getFilterCriteria());
                    std::vector<AssociateId> associateIds = filters->getAssociateId();
                    for(auto associateId : associateIds)
                    {
                        // TODO
                        // 1. Check if the ue is in handover
                        // 2. If the ue is in handover, send a notification to the subscriber'
                    }
                }
            }
        }
        if(!antennaMonitorMsg_->isScheduled())
            scheduleAt(simTime() + antennaMonitorInterval_, antennaMonitorMsg_);
        return;
    }

    MecServiceBase::handleSelfMessage(msg);
}

void RNIService::handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    std::string uri = currentRequestMessageServed->getUri();
    // std::vector<std::string> splittedUri = simu5g::utils::splitString(uri, "?");
    // // uri must be in form example/v1/rni/queries/resource
    // std::size_t lastPart = splittedUri[0].find_last_of("/");
    // if(lastPart == std::string::npos)
    // {
    //     Http::send404Response(socket); //it is not a correct uri
    //     return;
    // }
    // // find_last_of does not take in to account if the uri has a last /
    // // in this case resourceType would be empty and the baseUri == uri
    // // by the way the next if statement solve this problem
    // std::string baseUri = splittedUri[0].substr(0,lastPart);
    // std::string resourceType =  splittedUri[0].substr(lastPart+1);

    // check it is a GET for a query or a subscription
    if(uri.compare(baseUriQueries_ + "/layer2_meas") == 0 ) //queries
    {
        std::string params = currentRequestMessageServed->getParameters();
        //look for query parameters
        if(!params.empty())
        {
            std::vector<std::string> queryParameters = simu5g::utils::splitString(params, "&");
            /*
            * supported paramater:
            * - cell_id
            * - ue_ipv4_address
            * - ue_ipv6_address // not implemented yet
            */

            std::vector<MacNodeId> cellIds;
            std::vector<inet::Ipv4Address> ues;

            typedef std::map<std::string, std::vector<std::string>> queryMap;
            queryMap queryParamsMap; // e.g cell_id -> [0, 1]

            std::vector<std::string>::iterator it  = queryParameters.begin();
            std::vector<std::string>::iterator end = queryParameters.end();
            std::vector<std::string> params;
            std::vector<std::string> splittedParams;
            for(; it != end; ++it){
                if(it->rfind("cell_id", 0) == 0) // cell_id=par1,par2
                {
                    params = simu5g::utils::splitString(*it, "=");
                    if(params.size()!= 2) //must be param=values
                    {
                        Http::send400Response(socket);
                        return;
                    }
                    splittedParams = simu5g::utils::splitString(params[1], ",");
                    std::vector<std::string>::iterator pit  = splittedParams.begin();
                    std::vector<std::string>::iterator pend = splittedParams.end();
                    for(; pit != pend; ++pit){
                        cellIds.push_back((MacNodeId)std::stoi(*pit));
                    }
                }
                else if(it->rfind("ue_ipv4_address", 0) == 0)
                {
                    // TO DO manage acr:10.12
                    params = simu5g::utils::splitString(*it, "=");
                    if(params.size()!= 2) //must be param=values
                    {
                        Http::send400Response(socket);
                        return;
                    }
                    splittedParams = simu5g::utils::splitString(params[1], ",");
                    std::vector<std::string>::iterator pit  = splittedParams.begin();
                    std::vector<std::string>::iterator pend = splittedParams.end();
                    for(; pit != pend; ++pit)
                       ues.push_back(inet::Ipv4Address((*pit).c_str()));
                }
                else // bad parameters
                {
                    Http::send400Response(socket);
                    return;
                }

            }

            //send response
            if(!ues.empty() && !cellIds.empty())
            {
                Http::send200Response(socket, L2MeasResource_.toJson(cellIds, ues).dump().c_str());
            }
            else if(ues.empty() && !cellIds.empty())
            {
                Http::send200Response(socket, L2MeasResource_.toJsonCell(cellIds).dump().c_str());
            }
            else if(!ues.empty() && cellIds.empty())
           {
              Http::send200Response(socket, L2MeasResource_.toJsonUe(ues).dump().c_str());
           }
           else
           {
               Http::send400Response(socket);
           }

        }
        else{
            //no query params
            Http::send200Response(socket,L2MeasResource_.toJson().dump().c_str());
            return;
        }
    }

    else if (uri.compare(baseUriSubscriptions_) == 0) //subs
    {
        EV << "RNIService::Queries information on subscriptions for notifications" << endl; 

        uri.erase(0, baseUriSubscriptions_.length());
        
        // we assume that the uri is in the form /subscriptions/sub{subscriptionId}
        if(uri.length() > 0 && uri.find("sub") != std::string::npos)
        {
            EV << "RNIService::retrieving information of subscriber: " << uri << endl;
            int id = std::stoi(uri.erase(0, std::string("sub").length())); // sub is the prefix added in subscription phase

            if(subscriptions_.find(id) != subscriptions_.end())
            {
                EV << "RNIService::subscriber found!" << endl;
                Http::send200Response(socket, subscriptions_[id]->toJson().dump().c_str());
            }
            else
            {
                EV << "RNIService::subscriber not found" << endl;
                Http::send404Response(socket);
            }
        
        } 


    }
    else // not found
    {
        Http::send404Response(socket);
    }

}

void RNIService::handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{

    EV << "RNIService::handlePOSTRequest - Received a POST request" << endl;
    std::string uri = currentRequestMessageServed->getUri();

    if(uri.compare(baseUriSubscriptions_) == 0)
    {
        nlohmann::ordered_json request = nlohmann::json::parse(currentRequestMessageServed->getBody());
        if(request.contains("subscriptionType"))
        {
            SubscriptionBase *subscription = nullptr;
            if(request["subscriptionType"] == "CellChangeSubscription")
            {
                subscription = new CellChangeSubscription(subscriptionId_, socket, baseSubscriptionLocation_, eNodeB_);

                // scheduling monitoring of antenna
                if(antennaMonitorMsg_->isScheduled())
                    cancelEvent(antennaMonitorMsg_);
                    
                scheduleAt(simTime() + 0, antennaMonitorMsg_);
            }
            // TODO define other type of subscriptions
            if(subscription == nullptr)
            {
                EV << "RNIService::Subscription type not recognized" << endl;
                Http::send400Response(socket);
            }
            else
                // This method helps to avoid repeated code for the other type of subscription
                handleSubscriptionRequest(subscription, socket, request); 
        }
        else
        {
            EV << "RNIService::handlePOSTRequest - bad request: subscriptionType not found" << endl;
            Http::send400Response(socket);
        }

        
    }
    else // not found
    {
        Http::send404Response(socket);
    }

}

void RNIService::handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    EV << "RNIService::handlePUTRequest - Received a PUT request" << endl;
    std::string uri = currentRequestMessageServed->getUri();
    if(uri.find(baseUriSubscriptions_) == 0)
    {
        // subscription update
        uri.erase(0, baseUriSubscriptions_.length());
        EV << "AMS::Received subscriptions update from " << uri << endl;

        nlohmann::ordered_json request = nlohmann::json::parse(currentRequestMessageServed->getBody());
        SubscriptionBase *subscription = nullptr;

        if(request["subscriptionType"] == "CellChangeSubscription")
        {
            subscription = new CellChangeSubscription(subscriptionId_, socket, baseSubscriptionLocation_, eNodeB_);
        }
        // other type of subscriptions
        // ...

        if(subscription == nullptr)
        {
            EV << "RNIService::Subscription type not recognized" << endl;
            Http::send400Response(socket);
        }
        else
        {
            // TODO
            // check if the subscription exists
            // if it exists, update it
            // else send 404
            // ...

            uri.erase(0,uri.find(baseUriSubscriptions_+"sub") + baseUriSubscriptions_.length() + 3);

            auto it = subscriptions_.find(std::atoi(uri.c_str()));
            if(it != subscriptions_.end())
            {   
                bool res = subscription->fromJson(request);
                if(res)
                {
                    subscription->set_links(baseSubscriptionLocation_);
                    subscriptions_[it->first] = subscription;
                    EV << "RNIService::handlePUTRequest - Updated subsciber [" << it->first << "]" << endl;
                    
                    // debug
                    printAllSubscriptions();
                    Http::send200Response(socket, request.dump().c_str());
                }
                else
                {
                    EV << "RNIService::Body Error" << endl;
                    Http::send400Response(socket);
                }
            }
            else
            {
                EV << "RNIService::handlePUTRequest - subscriber not found" << endl;
                Http::send404Response(socket);
                return;
            }
            
        }


    }
    

}

void RNIService::handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    EV << "RNIService::handleDELETERequest - Received a DELETE request" << endl;
    std::string uri = currentRequestMessageServed->getUri();
    if(uri.find(baseUriSubscriptions_) == 0)
    {
        uri.erase(0,uri.find(baseUriSubscriptions_+"sub") + baseUriSubscriptions_.length() + 3);
        auto it = subscriptions_.find(std::atoi(uri.c_str()));
        if(it == subscriptions_.end())
        {   
            EV << "RNIService::handleDELETERequest - subscriber not found" << endl;
            Http::send404Response(socket);
            return;
        }
        subscriptions_.erase(it);
        printAllSubscriptions();
        Http::send204Response(socket);
    }
    else
    { // not found
        Http::send404Response(socket);
    }
}

void RNIService::finish()
{
// TODO
    return;
}

RNIService::~RNIService(){

}

void RNIService::handleSubscriptionRequest(SubscriptionBase *subscription, inet::TcpSocket* socket, const nlohmann::ordered_json& request)
{
    EV << "RNIService::handleSubscriptionRequest - " << subscription->getSubscriptionType() << endl;
    
    bool res = subscription->fromJson(request);
    if(res)
    {
        // valid subscription body
        subscription->set_links(baseSubscriptionLocation_);
        subscriptions_[subscriptionId_] = subscription;

        // sending response
        nlohmann::ordered_json response = subscription->toJson();
        response["subscriptionId"] = subscriptionId_;

        Http::send201Response(socket, response.dump().c_str());

        EV << "RNIService::handleSubscriptionRequest - Added new subsciber [" << subscriptionId_ << "]" << endl;
        subscriptionId_++;

        // printing all subscriptions
        printAllSubscriptions();
    }
    else
    {
        EV << "RNIService::handleSubscriptionRequest - bad request" << endl;
        Http::send400Response(socket);
        return;
    }
    return;
}


bool RNIService::manageSubscription()
{
    // TODO
    return true;
}


void RNIService::printAllSubscriptions()
{
    auto it  = subscriptions_.begin();
    auto end = subscriptions_.end();
    for(; it != end; ++it)
    {
        EV << "SubscriptionId: " << it->first << " " << it->second->toJson() << endl;
    }
}






