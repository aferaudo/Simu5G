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
     */
    if(json.contains("callbackReference"))
    {
        callbackReference_ = json["callbackReference"];
    }

    if(json.contains("websockNotifConfig"))
    {
        result = result && websockNotifConfig.fromJson(json["websockNotifConfig"]);
    }

    if(json.contains("_links"))
        _links.setHref(json["_links"]["self"]["href"]);

    if(json.contains("epiryDeadline"))
    {
        expiryDeadline.setSeconds(json["epiryDeadline"]["seconds"]);
        expiryDeadline.setNanoSeconds(json["epiryDeadline"]["nanoSeconds"]);
    }

    result = result && filterCriteria_->fromJson(json["filterCriteria"]);

    return result;

}

nlohmann::ordered_json MobilityProcedureSubscription::toJson() const
{
    EV << "MobilityProcedureSubscription::toJson" << endl;
    nlohmann::ordered_json val;
    val["_links"] = _links.toJson();
    val["callbackReference"] = callbackReference_;
    val["requestTestNotification"] = requestTestNotification;
    val["websockNotifConfig"] = websockNotifConfig.toJson();
    val["filterCriteria"] = filterCriteria_->toJson();
    val["subscriptionType"] = subscriptionType_;

    return val;
}
