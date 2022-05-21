//
//
// Authors:
//  Alessandro Calvio
//  Angelo Feraudo
// 

#include "MobilityProcedureNotification.h"
#include "nodes/mec/MECPlatform/EventNotification/MobilityProcedureEvent.h"

MobilityProcedureNotification::MobilityProcedureNotification() {
    notificationType_ = "MobilityProcedureNotification";
    appInstanceId_ = "";
}

//MobilityProcedureNotification::MobilityProcedureNotification(MobilityStatus ms, std::vector<AssociateId> ids)
//{
//    mobilityStatus = ms;
//    associateId = ids;
//}

MobilityProcedureNotification::~MobilityProcedureNotification() {
    associateId_.clear();

}


bool MobilityProcedureNotification::fromJson(const nlohmann::ordered_json& json)
{
    EV << "MobilityProcedureNotification::processing json" << endl;
    if(!json.contains("notificationType") || !json.contains("associateId")
            || !json.contains("mobilityStatus") || !json.contains("_links"))
    {
        EV << "MobilityProcedureNotification::Some required parameters is missing!" << endl;
        return false;
    }
    if(json["notificationType"]!= notificationType_)
    {
        EV << "MobilityProcedureNotification::notificationType not valid!" << endl;
        return false;
    }
    if(json.contains("timeStamp"))
    {
        timestamp_.setSeconds(json["timeStamp"]["seconds"]);
        timestamp_.setNanoSeconds(json["timeStamp"]["nanoSeconds"]);
    }

    for(auto it=json["associateId"].begin(); it != json["associateId"].end(); ++it)
    {
        AssociateId a;
        if(it.key() == "type")
            a.setType(json["type"]);
        if(it.key() == "port")
            a.setValue(json["value"]);
        associateId_.push_back(a);
    }

    // FIXME use a global method without static bounds
    for(int i = 0; i < 3; i++)
    {
        if(MobilityStatusString[i] == json["mobilityStatus"]){
            mobilityStatus = static_cast<MobilityStatus> (i);
            break;
        }
    }
    if(json.contains("appInstanceId"))
    {
        appInstanceId_ = json["appInstanceId"];
    }


    if(json.contains("targetAppInfo") && !targetAppInfo.fromJson(json["targetAppInfo"]))
    {
        return false;
    }


    links_ = json["_links"]["href"];

    return true;
}

nlohmann::ordered_json MobilityProcedureNotification::toJson() const
{
    nlohmann::ordered_json object;
    object["notificationType"] = notificationType_;
    object["timeStamp"] = timestamp_.toJson();
    object["associateId"] = nlohmann::json::array();
    for(auto it = associateId_.begin(); it != associateId_.end(); it++)
    {
        object["associateId"].push_back(it->toJson());
    }
    object["mobilityStatus"] = MobilityStatusString[mobilityStatus];


    // todo add checking that target app info exists
    object["targetAppInfo"] = targetAppInfo.toJson();

    object["_links"]["href"] = links_;

    if(!appInstanceId_.empty())
        object["appInstanceId"] = appInstanceId_;

    return object;
}

EventNotification* MobilityProcedureNotification::handleNotification(FilterCriteriaBase *filters, bool noCheck)
{
    MobilityProcedureEvent *event = nullptr;

    FilterCriteria* filterCriteria = static_cast<FilterCriteria*>(filters);
    bool found = false;

    if(filterCriteria->getMobilityStatus() == mobilityStatus)
    {
        if(!noCheck)
            for(auto &ids : associateId_)
            {

                for(auto &assId : filterCriteria->getAssociateId())
                {
                    if(assId.getType() == ids.getType() && assId.getValue()==ids.getValue())
                    {
                        found = true;
                        break;
                    }
                }
                if(found)
                {
                    break;
                }
            }

        if(found || (!appInstanceId_.empty() && filterCriteria->getAppInstanceId() == appInstanceId_))
        {
            EV << "MobilityProcedureNotification::An event has been created" << endl;
            event = new MobilityProcedureEvent(notificationType_);
        }

        if(event != nullptr)
            event->setMobilityProcedureNotification(this);


    }

    return event;
}
