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

#include "ApplicationMobilityService.h"

Define_Module(ApplicationMobilityService);


ApplicationMobilityService::ApplicationMobilityService()
{
    baseUriQueries_ = "/example/amsi/v1/queries/";
    baseUriSerDer_ = "/example/amsi/v1/app_mobility_services/"; // registration and deregistration uri
    callbackUri_ = "example/amsi/v1/eventNotification/";
    baseUriSubscriptions_ = "/example/amsi/v1/subscriptions/";
    baseSubscriptionLocation_ = host_+ baseUriSubscriptions_;

    applicationServiceIds = 0;

    subscriptionId_ = 0;
    subscriptions_.clear();


}

ApplicationMobilityService::~ApplicationMobilityService()
{

}

void ApplicationMobilityService::initialize(int stage)
{
    EV << "AMS::Initializing..." << endl;
    MecServiceBase::initialize(stage);
}

void ApplicationMobilityService::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage())
    {
        // TODO - No message so far
    }
    MecServiceBase::handleMessage(msg);
}

void ApplicationMobilityService::handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    EV << "AMS::handleGETRequest" << endl;
    std::string uri = std::string(currentRequestMessageServed->getUri());
    if(uri.compare(baseUriQueries_ + "/adjacent_app_instances") == 0)
    {
        EV << "AMS::This API has not been implemented yet!" << endl;
        Http::send404Response(socket);
    }
    else if(uri.compare(baseUriSerDer_) == 0)
    {
        EV << "AMS::Retrieve information about the registered application mobility service" << endl;
        nlohmann::ordered_json response = registrationResources_.toJson();
        Http::send200Response(socket, response.dump().c_str());
    }
    else if(uri.find(baseUriSerDer_) == 0)
    {
        uri.erase(0, uri.find(baseUriSerDer_) + baseUriSerDer_.length());
        if(uri.length() > 0 && uri.find("deregister_task") == std::string::npos)
        {
            EV << "AMS::Getting specific id " << endl;

            Http::send200Response(socket,registrationResources_.toJsonFromId(uri.c_str()).dump().c_str());
        }
        else
        {
            EV << "AMS::deregister_task API not implemented" << endl;
        }

    }
    else
    {
        EV << "AMS::Bad Request" << endl;
        Http::send400Response(socket);
    }
}

void ApplicationMobilityService::handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    EV << "AMS::handlePOSTRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();
    if(uri.compare(baseUriSerDer_) == 0)
    {

        nlohmann::ordered_json request = nlohmann::json::parse(currentRequestMessageServed->getBody());
        RegistrationInfo* r = new RegistrationInfo();//registrationResources_.buildRegistrationInfoFromJson(request);
        bool res = r->fromJson(request);
        if(!res)
        {
            EV << "AMS::Post request - bad request " << endl;
            Http::send400Response(socket);
            return;
        }
        r->setAppMobilityServiceId(std::to_string(applicationServiceIds));
        request["appMobilityServiceId"] =  std::to_string(applicationServiceIds);
        registrationResources_.addRegistrationInfo(r);

        applicationServiceIds ++;
        Http::send201Response(socket, request.dump().c_str());
    }
    else if(uri.compare(baseUriSubscriptions_) == 0)
    {
        // New subscriber
        nlohmann::ordered_json request = nlohmann::json::parse(currentRequestMessageServed->getBody());
        if(request.contains("subscriptionType"))
        {

            if(request["subscriptionType"] == "MobilityProcedureSubscription")
            {
                MobilityProcedureSubscription *subscription = new MobilityProcedureSubscription(subscriptionId_, socket, baseSubscriptionLocation_, eNodeB_);
                handleSubscriptionRequest(subscription, socket, request); // This method helps to avoid repeated code for the other type of subscription
            }
            // Here should be added AdjacentAppInfoSubscription
            else
            {
                EV << "AMS::Subscription type not recognized" << endl;
                Http::send400Response(socket);
            }
        }
        else
        {
            EV << "AMS::Bad request!" << endl;
            Http::send400Response(socket);
        }
    }
    else if(uri.compare(callbackUri_) == 0)
    {
        EV << "AMS::Received a notification event from: " << currentRequestMessageServed->getHost()  << endl;
        // notification type
        nlohmann::ordered_json request = nlohmann::json::parse(currentRequestMessageServed->getBody());
        if(request.contains("notificationType"))
        {
            handleNotificationCallback(socket,request);
        }
        else
        {
            EV << "AMS::Bad request!" << endl;
            Http::send400Response(socket);
        }
    }
    else
    {
        EV << "AMS::Post request - bad uri " << endl;
        Http::send400Response(socket);
    }
}

void ApplicationMobilityService::handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    EV << "AMS::handlePUTRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();
    if(uri.find(baseUriSerDer_) == 0)
    {
        uri.erase(0, uri.find(baseUriSerDer_) + baseUriSerDer_.length());
        nlohmann::ordered_json request = nlohmann::json::parse(currentRequestMessageServed->getBody());
        // build registration info
        //RegistrationInfo *r = registrationResources_.buildRegistrationInfoFromJson(request);
        RegistrationInfo *r = new RegistrationInfo();
        bool res = r->fromJson(request);
        if(!res)
        {
            EV << "AMS::PUT- BAD REQUEST" << endl;
            Http::send400Response(socket);
            return;
        }
        res = registrationResources_.updateRegistrationInfo(uri, r);
        if(res)
        {
            EV << "AMS::Service Consumer " << uri << " correctly updated! " << endl;
            Http::send200Response(socket, request.dump().c_str());
        }
        else
        {
            EV << "AMS::Service Consumer " << uri << " not found! " << endl;
            Http::send404Response(socket);
        }
    }
    else
    {
        EV << "AMS::PUT request - bad uri " << endl;
        Http::send404Response(socket);
    }
}

void ApplicationMobilityService::handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    EV << "AMS::handleDELETERequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();
    if(uri.find(baseUriSerDer_) == 0)
    {
        EV << "AMS::Delete registration info " <<  endl;
        uri.erase(0, uri.find(baseUriSerDer_) + baseUriSerDer_.length());
        if(!registrationResources_.removeRegistrationInfo(uri.c_str()))
        {
            EV << "AMS::Delete request - service consumer not found!" << endl;
            Http::send404Response(socket);
        }
    }
    else
    {
        EV << "AMS::DELETE request - bad uri " << endl;
        Http::send404Response(socket);
    }
}

void ApplicationMobilityService::handleSubscriptionRequest(SubscriptionBase *subscription, inet::TcpSocket *socket, const nlohmann::ordered_json &request)
{
    bool res = subscription->fromJson(request);
    if(res)
    {

        subscription->set_links(baseSubscriptionLocation_);

        subscriptions_[subscriptionId_] = subscription;

        nlohmann::ordered_json response = request;
        response["subscriptionId"] = subscriptionId_;
        subscriptionId_ ++;

        Http::send201Response(socket, response.dump().c_str());

        // TODO Add new parameter in MobilityProcedureSubscription:
        // requestTestNotification - here you can start the test!

        EV << serviceName_ << " - correct subscription created!" << endl;
    }
    else
    {
        EV << "AMS::an error occurred during request parsing!" << endl;
        Http::send400Response(socket);
    }
}

void ApplicationMobilityService::handleNotificationCallback(inet::TcpSocket *socket, const nlohmann::ordered_json &request)
{
    /*
     * This method manages two types of notification
     * - MobilityProcedureNotification
     * - AdjacentAppInfoNotification
     * */
    NotificationBase *notification = nullptr;
    std::string subscriptionType = "";
    std::vector<int> ids;
    if(request["notificationType"]=="MobilityProcedureNotification")
    {
        notification = new MobilityProcedureNotification();
        subscriptionType = "MobilityProcedureSubscription";
    }
    else if(request["notificationType"]=="AdjacentAppInfoNotification")
    {
        EV << "AMS::Notification type not implemented" << endl;
        Http::send400Response(socket);
        return;
    }
    else
    {
        EV << "AMS::Notification type not managed " << endl;
        Http::send400Response(socket);
        return;
    }

    notification->fromJson(request);
    std::vector<int> appInstanceId = registrationResources_.getAppInstanceIds(notification->getAssociateId());
    for(auto subscriber : subscriptions_)
    {
        EventNotification *event = nullptr;
        if(subscriptionType==subscriber.second->getSubscriptionType())
        {
            bool removeChecks = false;
            if(std::find(appInstanceId.begin(), appInstanceId.end(), std::stoi(subscriber.second->getFilterCriteria()->getAppInstanceId()))
                    != appInstanceId.end())
            {
                removeChecks = true;
            }
            event = notification->handleNotification(subscriber.second->getFilterCriteria(), removeChecks);
            if(event != nullptr)
            {
              event->setSubId(subscriber.second->getSubscriptionId());
              newSubscriptionEvent(event);
            }
        }

    }



}
