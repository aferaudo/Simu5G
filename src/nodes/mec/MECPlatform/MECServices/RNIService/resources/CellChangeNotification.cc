//
//      CellChangeNotification.cc -  Implementation of CellChangeNotification 
//
// Author: Angelo Feraudo (University of Bologna)
//

#include "CellChangeNotification.h"

CellChangeNotification::CellChangeNotification() {
    notificationType_ = "CellChangeNotification";

}

CellChangeNotification::~CellChangeNotification() {
    // TODO Auto-generated destructor stub
}

nlohmann::ordered_json CellChangeNotification::toJson() const
{
    nlohmann::ordered_json val;
    val["notificationType"] = notificationType_;
    val["associateId"] = nlohmann::json::array();
    for(auto it = associateId_.begin(); it != associateId_.end(); it++)
    {
        val["associateId"].push_back(it->toJson());
    }
    val["hoStatus"] = FilterCriteriaAssocHo::getHoStatusString(hoStatus_);

    val["srcEcgi"]["cellId"] = srcEcgi_.getCellId();
    val["srcEcgi"]["plmnId"]["mcc"] = srcEcgi_.getPlmn().getMcc();
    val["srcEcgi"]["plmnId"]["mnc"] = srcEcgi_.getPlmn().getMnc();
    val["trgEcgi"]["cellId"] = trgEcgi_.getCellId();
    val["trgEcgi"]["plmnId"]["mcc"] = trgEcgi_.getPlmn().getMcc();
    val["trgEcgi"]["plmnId"]["mnc"] = trgEcgi_.getPlmn().getMnc();

    val["_links"]["href"] = links_;
    return val;
}

bool CellChangeNotification::fromJson(const nlohmann::ordered_json& json)
{
    EV << "CellChangeNotification::Building CellChangeNotification attribute from json" << endl;
    bool result = NotificationBase::fromJson(json);

    if(notificationType_ != json["notificationType"])
    {
        EV << "CellChangeNotification::fromJson - notificationType mismatch" << endl;
        return false;
    }

    if (!json.contains("hoStatus") || !json.contains("srcEcgi") || !json.contains("trgEcgi"))
    {
        EV << "CellChangeNotification::fromJson - missing mandatory fields" << endl;
        return false;
    }

    hoStatus_ = FilterCriteriaAssocHo::getHoStatusFromString(json["hoStatus"]);

    // FIXME plmn from json
    srcEcgi_.setCellId(json["srcEcgi"]["cellId"]);
    mec::Plmn plmn;
    plmn.mcc = json["srcEcgi"]["plmnId"]["mcc"];
    plmn.mnc = json["srcEcgi"]["plmnId"]["mnc"];
    srcEcgi_.setPlmn(plmn);
    trgEcgi_.setCellId(json["trgEcgi"]["cellId"]);
    plmn.mcc = json["trgEcgi"]["plmnId"]["mcc"];
    plmn.mnc = json["trgEcgi"]["plmnId"]["mnc"];
    trgEcgi_.setPlmn(plmn);
    return result;
}

EventNotification* CellChangeNotification::handleNotification(FilterCriteriaBase *filters, bool noCheck)
{
    return nullptr;
}


