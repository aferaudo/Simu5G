//
//      FilterCriteriaAssocHo.cc -  Implementation of FilterCriteriaAssocHo
//
// Author: Angelo Feraudo (University of Bologna)
//


#include "FilterCriteriaAssocHo.h"

FilterCriteriaAssocHo::FilterCriteriaAssocHo() {

}

FilterCriteriaAssocHo::~FilterCriteriaAssocHo() {
}


nlohmann::ordered_json FilterCriteriaAssocHo::toJson() const
{
     
    nlohmann::ordered_json val;
    val["appInstanceId"] = appInstanceId_;
    val["associateId"] = nlohmann::ordered_json::array();
    for(auto it = associateId_.begin(); it != associateId_.end(); ++it )
    {
        val["associateId"].push_back(it->toJson());
    }
    
    val["hoStatus"] = nlohmann::ordered_json::array();
    for(auto it = hoStatus_.begin(); it != hoStatus_.end(); ++it )
    {
        val["hoStatus"].push_back(getHoStatusString(*it));
    }
    
    val["ecgi"] = nlohmann::ordered_json::array();
    for(auto it = ecgi_.begin(); it != ecgi_.end(); ++it )
    {
        val["ecgi"].push_back(it->toJson());
    }
    return val;
}

bool FilterCriteriaAssocHo::fromJson(const nlohmann::ordered_json& json)
{
    EV << "FilterCriteriaAssocHo::Building FilterCriteriaAssocHo attribute from json" << endl;

    appInstanceId_ = json["appInstanceId"];
    for(auto &val : json["associateId"].items()){
        nlohmann::ordered_json associateId = val.value();
        AssociateId a;
        a.setType(associateId["type"]);
        a.setValue(associateId["value"]);
        associateId_.push_back(a);
    }

    if(!json.contains("hoStatus"))
    {
       hoStatus_.push_back(getHoStatusFromString("COMPLETED")); // default value
    }
    else
    {
        for(auto &val : json["hoStatus"].items()){
           nlohmann::ordered_json hoStatusString = val.value();
           hoStatus_.push_back(getHoStatusFromString(hoStatusString));
        }
    }

    if(json.contains("ecgi"))
    {
        for(auto &val : json["ecgi"].items()){
           nlohmann::ordered_json ecgi = val.value();
           Ecgi e;
           e.setCellId(ecgi["cellId"]);
           mec::Plmn plmn;
           plmn.mcc = ecgi["plmnId"]["mcc"];
           plmn.mnc = ecgi["plmnId"]["mnc"];
           e.setPlmn(plmn);
           ecgi_.push_back(e);
        }
    }

    return true;
}


HoStatus FilterCriteriaAssocHo::getHoStatusFromString(std::string hoStatus)
{
        if(hoStatus == "COMPLETED")
            return COMPLETED;
        else if(hoStatus == "REJECTED")
            return REJECTED;
        else if(hoStatus == "CANCELLED")
            return CANCELLED;
        else if(hoStatus == "IN_PREPARATION")
            return IN_PREPARATION;
        else if(hoStatus == "IN_EXECUTION")
            return IN_EXECUTION;
        else
            return INVALID;
 }
 