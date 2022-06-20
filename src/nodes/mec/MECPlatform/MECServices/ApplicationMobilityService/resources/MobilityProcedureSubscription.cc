/*
 * MobilityProcedureSubscription.cc
 *
 *  Created on: May 10, 2022
 *      Author: Alessandro Calvio, Angelo Feraudo
 */

#include "MobilityProcedureSubscription.h"
#include "nodes/mec/MECPlatform/EventNotification/MobilityProcedureEvent.h"

MobilityProcedureSubscription::MobilityProcedureSubscription() {
    subscriptionType_ = "MobilityProcedureSubscription";
    filterCriteria_ = new FilterCriteria();
}

MobilityProcedureSubscription::MobilityProcedureSubscription(unsigned int subId, inet::TcpSocket *socket , const std::string& baseResLocation,  std::set<omnetpp::cModule*>& eNodeBs):
        SubscriptionBase(subId,socket,baseResLocation, eNodeBs)
{
    subscriptionType_ = "MobilityProcedureSubscription";
    filterCriteria_ = new FilterCriteria();
}
MobilityProcedureSubscription::~MobilityProcedureSubscription() {
    // TODO Auto-generated destructor stub
}

void MobilityProcedureSubscription::sendSubscriptionResponse()
{
    // What Should we do here?
}

void MobilityProcedureSubscription::sendNotification(EventNotification *event)
{
    EV << "MobilityProcedureSubscription::sendNotification" << endl;
    MobilityProcedureEvent *mobilityEvent = check_and_cast<MobilityProcedureEvent*>(event);

    // Create a MobilityProcedureNotification message
//    const std::string notificationType = "MobilityProcedureNotification"; //optional:
//    timestamp; //optional: yes
//    std::vector<AssociateId> associateId; //optional: no
//    mobilityStatus; //optional: no
//    targetAppInfo; //optional: yes
//    std::string _links; // optional: yes

    MobilityProcedureNotification *notification = mobilityEvent->getMobilityProcedureNotification();
    notification->setLinks(links_);
    std::cout << "AMS::sending notification to " <<  socket_->getRemoteAddress() << " " << socket_->getRemotePort() << notification->toJson().dump(2) << endl;
    EV << socket_->getRemoteAddress() << " " << socket_->getRemotePort() << " " << clientHost_ << " MobilityProcedureSubscription debug message " << endl;
    Http::sendPostRequest(socket_, notification->toJson().dump().c_str(), clientHost_.c_str(), clientUri_.c_str());

}

EventNotification* MobilityProcedureSubscription::handleSubscription()
{
    return nullptr;
}

bool MobilityProcedureSubscription::fromJson(const nlohmann::ordered_json& json)
{
    bool result = true;
    EV << "MobilityProcedureSubscription::from Json\n" << json << endl;
    if(!json.contains("subscriptionType") || !json.contains("filterCriteria"))
    {
        EV << "MobilityProcedureSubscription::required parameters not specified in the request!" << endl;
        return false;
    }

    if(json["subscriptionType"] != subscriptionType_)
    {
        EV << "MobilityProcedureSubscription::subscription type not valid!" << endl;
        return false;
    }


    if(!json.contains("callbackReference") && !json.contains("websockNotifConfig"))
    {
        EV << "MobilityProcedureSubscription::at least one of callbackReference and websockNotifConfig shall "
                "be provided by service consumer" << endl;

        return false;
    }

    /*
     * If both callbackReference and websockNotifConfig are provided, it is up to AMS to choose an alternative and return
     * only that alternative in the response
     *
     * In this case the AMS uses by default the callbackReference field if present.
     *
     */

    std::string notifyURL;
    if(json.contains("callbackReference"))
    {
        callbackReference_ = json["callbackReference"];
        notifyURL = callbackReference_;
    }
    else if(json.contains("websockNotifConfig") )
    {
        result = result && websockNotifConfig.fromJson(json["websockNotifConfig"]);
        notifyURL = websockNotifConfig.getWebSocketUri();
    }

    std::size_t found = notifyURL.find("/");
    if (found!=std::string::npos)
    {
         clientHost_ = notifyURL.substr(0, found);
         clientUri_ = notifyURL.substr(found);
         EV << "clientHost: " << clientHost_ << " clientUri: " << clientUri_ << endl;
    }

    if(json.contains("_links"))
        links_ = json["_links"]["self"]["href"];

    if(json.contains("epiryDeadline"))
    {
        expiryDeadline.setSeconds(json["epiryDeadline"]["seconds"]);
        expiryDeadline.setNanoSeconds(json["epiryDeadline"]["nanoSeconds"]);
        expiryDeadline.setValid(true);
    }
    else
    {
        expiryDeadline.setValid(false);
    }

    result = result && filterCriteria_->fromJson(json["filterCriteria"]);

    return result;

}

nlohmann::ordered_json MobilityProcedureSubscription::toJson() const
{
    EV << "MobilityProcedureSubscription::toJson" << endl;
    nlohmann::ordered_json val;
    val["_links"]["self"]["href"] = links_;
    val["callbackReference"] = callbackReference_;
    val["requestTestNotification"] = requestTestNotification;
    val["websockNotifConfig"] = websockNotifConfig.toJson();
    val["filterCriteria"] = filterCriteria_->toJson();
    val["subscriptionType"] = subscriptionType_;

    return val;
}

void MobilityProcedureSubscription::to_string() {
    EV << "MobilityProcedureSubscription: " <<endl;
    EV << "\tsubid: " << subscriptionId_ << endl;
    EV << "\tcallbackReferece: " << callbackReference_ << endl;
    EV << "\tfilterCriteria:" <<endl;
    EV << "\t\tappInstanceId:" << filterCriteria_->getAppInstanceId() <<endl;
    EV << "\t\tmobilityStatus:" << static_cast<FilterCriteria*>(filterCriteria_)->getMobilityStatusString() <<endl;
}
