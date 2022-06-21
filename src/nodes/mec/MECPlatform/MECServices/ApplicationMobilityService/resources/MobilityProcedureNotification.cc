//
//
// Authors:
//  Alessandro Calvio
//  Angelo Feraudo
// 

#include "MobilityProcedureNotification.h"
#include "nodes/mec/MECPlatform/EventNotification/MobilityProcedureEvent.h"


MobilityProcedureNotification::MobilityProcedureNotification():MobilityProcedureNotification(nullptr) {
}
MobilityProcedureNotification::MobilityProcedureNotification(ApplicationMobilityResource* resources) {
    resources_ = resources;
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
    EV << "MobilityProcedureNotification: notificationType: " << notificationType_ << endl;
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
    for(auto& it : json["associateId"].items())
    {
        AssociateId a;

        nlohmann::ordered_json val = it.value();

        a.setType(val["type"]);
        a.setValue(val["value"]);
        associateId_.push_back(a);
    }
    EV << "AssociateId::FromJSON " << associateId_.size() << endl;
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
    bool found = true;

    std::vector<MobilityStatus> status = filterCriteria->getMobilityStatus();
    bool condition = std::find(status.begin(), status.end(), mobilityStatus) != status.end();
    if(condition)
    {
        //taking trace of an app that starts its migration-phase
        if(resources_ != nullptr && mobilityStatus == INTERHOST_MOVEOUT_TRIGGERED
                && targetAppInfo.getCommInterface().size() != 0)
        {
            EV << "MobilityProcedureNotification::an app has started its migration" << endl;
            resources_->addMigratingApp(&targetAppInfo);
        }

        if(!noCheck)
        {
            found = false;
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


