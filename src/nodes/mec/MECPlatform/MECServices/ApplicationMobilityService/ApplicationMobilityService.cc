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
    registrationResources_ = new ApplicationMobilityResource();
    baseUriQueries_ = "/example/amsi/v1/queries/";
    baseUriSerDer_ = "/example/amsi/v1/app_mobility_services/"; // registration and deregistration uri
    callbackUri_ = "/example/amsi/v1/eventNotification/";
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
    migrationCounter_ = 0;
    totalMigrationsSignal_ = registerSignal("totalMigrations");
    MecServiceBase::initialize(stage);
}

void ApplicationMobilityService::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage() && std::strcmp(msg->getFullName(),"localMigration") == 0)
    {
        EV << "AMS::LOCAL MIGRATION RECEIVED: to be processed " << registrationResources_->getMigratedApps().size() << endl;
        if(registrationResources_->getMigratedApps().size() > 0)
        {
            EV << "AMS::an app has been correctly migrated -- generate local event" << endl;
            for(auto &el : registrationResources_->getMigratedApps())
            {
                // Generating notification
                MobilityProcedureNotification *notification = new MobilityProcedureNotification();
                std::cout << "DEvo bestemmiare? " << endl;
                RegistrationInfo *r = registrationResources_->getRegistrationInfoFromContext(el.second->getAppInstanceId());
                std::cout << "non bestemmio per poco" << endl;
                if(r != nullptr)
                {
                    notification->setMobilityStatus(INTERHOST_MOVEOUT_COMPLETED);
                    notification->setTargetAppInfo(*el.second);

                    // statistics
                    simtime_t migrationUpdateTime = simTime();
                    // total migration from the start
                    migrationCounter_ ++;
                    std::cout << "Migration counter has been updated: " << migrationCounter_ << endl;

                    // migration at this time (useful to set grop migration per an interval of time)
                    // value = 1 is just an indication
                    emit(totalMigrationsSignal_, 1);


                    std::vector<AssociateId> associateId;
                    for(auto devInfo : r->getDeviceInformation())
                    {
                        associateId.push_back(devInfo.getAssociateId());
                        EV << "AMS::local notification generated - associateID " << devInfo.getAssociateId().getValue() << " added" << endl;
                    }
                    notification->setAssociateId(associateId);

                    // removing migrated app from the list
                    registrationResources_->removingMigratedApp(el.first);


                    nlohmann::ordered_json jsonObject = notification->toJson();
                    EV << "AMS::local notification generated " << jsonObject.dump(2) << endl;
                    handleNotificationCallback(jsonObject);

                }
                else
                {
                    EV << "AMS:: no information regarding migrated app - " << el.second->getAppInstanceId() << " - has been found" << endl;

                }
            }
        }
        delete msg;
    }
    else
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
        nlohmann::ordered_json response = registrationResources_->toJson();
        Http::send200Response(socket, response.dump().c_str());
    }
    else if(uri.find(baseUriSerDer_) == 0)
    {
        uri.erase(0, baseUriSerDer_.length());
        if(uri.length() > 0 && uri.find("deregister_task") == std::string::npos)
        {
            EV << "AMS::Getting specific id " << endl;

            Http::send200Response(socket,registrationResources_->toJsonFromId(uri.c_str()).dump().c_str());
        }
        else
        {
            EV << "AMS::deregister_task API not implemented" << endl;
        }

    }
    else if(uri.compare(baseUriSubscriptions_) == 0)
    {
        /*
         * TODO to be implemented
         * Here we should return the list of links to requestor's subscriptions
         */
        EV << "AMS::subscriptions get - all - received (not implemented yet)" << endl;

    }
    else if(uri.find(baseUriSubscriptions_) == 0)
    {
        EV << "AMS::subscriptions get - single - received" << endl;
        uri.erase(0, baseUriSubscriptions_.length());
        if(uri.length() > 0 && uri.find("sub") != std::string::npos)
        {
            EV << "AMS::retrieving information of subscriber: " << uri << endl;
            int id = std::stoi(uri.erase(0, std::string("sub").length())); // sub is the prefix added in subscription phase
            if(subscriptions_.find(id) != subscriptions_.end())
            {
                EV << "AMS::subscriber found!" << endl;
                Http::send200Response(socket, subscriptions_[id]->toJson().dump().c_str());
            }
            else
            {
                EV << "AMS::subscriber not found" << endl;
                Http::send404Response(socket);
            }
        }
        else
        {
            EV << "AMS::Bad Request" << endl;
            Http::send400Response(socket);
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
        RegistrationInfo* r = new RegistrationInfo();//registrationResources_->buildRegistrationInfoFromJson(request);
        bool res = r->fromJson(request);
        if(!res)
        {
            EV << "AMS::Post request - bad request " << endl;
            Http::send400Response(socket);
            return;
        }
        r->setAppMobilityServiceId(std::to_string(applicationServiceIds));
        //request["appMobilityServiceId"] =  std::to_string(applicationServiceIds);
        registrationResources_->addRegistrationInfo(r);

        applicationServiceIds ++;
        nlohmann::ordered_json response = r->toJson();
        std::pair<std::string, std::string> p("Location: ", baseUriSerDer_);
        EV << "AMS::Correctly subscribed sending: " << response.dump() <<endl;
        Http::send201Response(socket, response.dump(2).c_str(), p);
    }
    else if(uri.compare(baseUriSubscriptions_) == 0)
    {
        // New subscriber
        nlohmann::ordered_json request = nlohmann::json::parse(currentRequestMessageServed->getBody());
        if(request.contains("subscriptionType"))
        {
            SubscriptionBase *subscription = nullptr;

            if(request["subscriptionType"] == "MobilityProcedureSubscription")
            {
                subscription = new MobilityProcedureSubscription(subscriptionId_, socket, baseSubscriptionLocation_, eNodeB_);
            }
            // Here should be added AdjacentAppInfoSubscription

            if(subscription == nullptr)
            {
                EV << "AMS::Subscription type not recognized" << endl;
                Http::send400Response(socket);
            }
            else
                handleSubscriptionRequest(subscription, socket, request); // This method helps to avoid repeated code for the other type of subscription


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
            handleNotificationCallback(request);
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
        EV << "AMS::received registration update from " << uri << endl;
        nlohmann::ordered_json request = nlohmann::json::parse(currentRequestMessageServed->getBody());
        // build registration info
        //RegistrationInfo *r = registrationResources_->buildRegistrationInfoFromJson(request);
        RegistrationInfo *r = new RegistrationInfo();
        bool res = r->fromJson(request);
        if(!res)
        {
            EV << "AMS::PUT- BAD REQUEST" << endl;
            Http::send400Response(socket);
            return;
        }
        res = registrationResources_->updateRegistrationInfo(uri, r);
        if(res)
        {
            EV << "AMS::Service Consumer " << uri << " correctly updated! " << endl;
            Http::send200Response(socket, request.dump().c_str());

            // this message is generated for each put: it allows to generate self-notification, useful for
            // app migrated from dynamic resource to local resouce
            cMessage *message = new cMessage("localMigration");
            if(!message->isScheduled())
            {

                double time = 0; //exponential(0.0005);
                EV << "AMS::localMigration in " << time << endl;
                scheduleAt(simTime() + time, message);
            }
        }
        else
        {
            EV << "AMS::Service Consumer " << uri << " not found! " << endl;
            Http::send404Response(socket);
        }
    }
    else if(uri.find(baseUriSubscriptions_) == 0)
    {

        uri.erase(0, baseUriSubscriptions_.length());
        EV << "AMS::Received subscriptions update from " << uri << endl;
        nlohmann::ordered_json request = nlohmann::json::parse(currentRequestMessageServed->getBody());
        SubscriptionBase *subscription = nullptr;

        if(request["subscriptionType"] == "MobilityProcedureSubscription")
        {
            int subscriptionId = std::stoi(uri.erase(0, std::string("sub").length()));
            subscription = new MobilityProcedureSubscription(subscriptionId, socket, baseSubscriptionLocation_, eNodeB_);
        }

        if(subscription != nullptr)
        {
            subscription->fromJson(request);

            auto sub = subscriptions_.find(subscription->getSubscriptionId());
            if(sub != subscriptions_.end())
            {
                subscription->set_links(baseSubscriptionLocation_);
                subscriptions_[sub->first] = subscription;
                EV << "AMS::Subscription " << subscription->toJson() << endl;
                EV << "AMS::Subscription Updated" << endl;
                Http::send200Response(socket, request.dump().c_str());
            }
            else
            {
                EV << "AMS::PUT request - subscription not found " << endl;
                Http::send404Response(socket);
            }
        }
        else
        {
            EV << "AMS::Subscription type not recognised!" << endl;
            Http::send400Response(socket);
        }

    }
    else
    {
        EV << "AMS::PUT request - bad uri " << endl;
        Http::send400Response(socket);
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
        if(!registrationResources_->removeRegistrationInfo(uri.c_str()))
        {
            EV << "AMS::Delete request - service consumer not found!" << endl;
            Http::send404Response(socket);
        }
    }
    else if(uri.find(baseUriSubscriptions_) == 0)
    {
        EV << "AMS::Delete subscription " <<  uri <<endl;
        uri.erase(0,uri.find(baseUriSubscriptions_+"sub") + baseUriSubscriptions_.length() + 3);
        EV << "AMS::Deleting " <<  uri << endl;
        auto it = subscriptions_.find(std::atoi(uri.c_str()));
        if(it == subscriptions_.end())
        {
            EV << "AMS:: delete subscription : subscriber " << uri << " not found" << endl;
            Http::send404Response(socket);
            return;
        }

        subscriptions_.erase(it);
        printAllSubscription();
    }
    else
    {
        EV << "AMS::DELETE request - bad uri " << endl;
        Http::send404Response(socket);
    }
}

void ApplicationMobilityService::handleSubscriptionRequest(SubscriptionBase *subscription, inet::TcpSocket *socket, const nlohmann::ordered_json &request)
{
    EV << "AMS::handleSubscriptionRequest" << endl;

    EV << "AMS::" << subscription->getSubscriptionType() << endl;
    bool res = subscription->fromJson(request);
    if(res)
    {

        subscription->set_links(baseSubscriptionLocation_);

        subscriptions_[subscriptionId_] = subscription;

        nlohmann::ordered_json response = subscription->toJson();
        response["subscriptionId"] = subscriptionId_;
        EV << "AMS::subscribed with id " << subscriptionId_ << "\n" << subscription->toJson() << endl;
        subscriptionId_ ++;
        Http::send201Response(socket, response.dump().c_str());

        // TODO Add new parameter in MobilityProcedureSubscription:
        // requestTestNotification - here you can start the test!

        // Debugging
        printAllSubscription();
        EV << serviceName_ << " - correct subscription created!" << endl;
    }
    else
    {
        EV << "AMS::an error occurred during request parsing!" << endl;
        Http::send400Response(socket);
    }
}

void ApplicationMobilityService::handleNotificationCallback(const nlohmann::ordered_json &request)
{
    /*
     * This method manages two types of notification
     * - MobilityProcedureNotification
     * - AdjacentAppInfoNotification
     * */
    NotificationBase *notification = nullptr;
    std::string subscriptionType = "";
    std::vector<int> ids;
    EV << "AMS::received notification request: " << request.dump(2) << endl;
    if(request["notificationType"]=="MobilityProcedureNotification")
    {
        notification = new MobilityProcedureNotification(registrationResources_);
        subscriptionType = "MobilityProcedureSubscription";
    }
    else if(request["notificationType"]=="AdjacentAppInfoNotification")
    {
        EV << "AMS::Notification type not implemented" << endl;
        //Http::send400Response(socket);
        return;
    }
    else
    {
        EV << "AMS::Notification type not managed " << endl;
//        Http::send400Response(socket);
        return;
    }
    EV << "AMS::Trigger - " << request["notificationType"] << " - received!" << endl;

    bool res = notification->fromJson(request);
    EV << "Notification processed: " << notification->toJson() << endl;
    bool removeChecks;
    if(res){
        std::vector<std::string> appInstanceId = registrationResources_->getAppInstanceIds(notification->getAssociateId());

        for(auto subscriber : subscriptions_)
        {
            EV << "AMS::processing subscriber: "<< subscriber.second->getSubscriptionId() << endl;
            EventNotification *event = nullptr;
            if(subscriptionType==subscriber.second->getSubscriptionType())
            {
                removeChecks = false;
                if(!appInstanceId.empty() && std::find(appInstanceId.begin(), appInstanceId.end(), subscriber.second->getFilterCriteria()->getAppInstanceId())
                        != appInstanceId.end())
                {
                    EV << "AMS::notification associated id checks removed" << endl;
                    removeChecks = true;
                }
                event = notification->handleNotification(subscriber.second->getFilterCriteria(), removeChecks);
                if(event != nullptr)
                {
                  event->setSubId(subscriber.second->getSubscriptionId());
                  EV << "AMS::next notification to -> " << subscriber.second->getSubscriptionId() << endl;
                  newSubscriptionEvent(event);
                }
            }

        }
    }
}

bool ApplicationMobilityService::manageSubscription()
{
    int subId = currentSubscriptionServed_->getSubId();
    if(subscriptions_.find(subId) != subscriptions_.end())
    {
        EV << "ApplicationMobilityService::manageSubscription() - subscription with id: " << subId << " found" << endl;
        SubscriptionBase * sub = subscriptions_[subId]; //upcasting (getSubscriptionType is in Subscriptionbase)
        sub->sendNotification(currentSubscriptionServed_);
        if(currentSubscriptionServed_!= nullptr)
            delete currentSubscriptionServed_;
        currentSubscriptionServed_ = nullptr;

        return true;
    }
    else
    {
        return false;
    }
}

void ApplicationMobilityService::printAllSubscription() {

    for(auto subscriber : subscriptions_)
    {
        subscriber.second->to_string();
    }
}
