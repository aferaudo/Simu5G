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

#include "FilterCriteria.h"

FilterCriteria::FilterCriteria() {
    // TODO Auto-generated constructor stub

}

//FilterCriteria::FilterCriteria(std::string appInstanceId, MobilityStatus mobilityStatus)
//{
//    FilterCriteria(appInstanceId, std::vector<AssociateId>(), mobilityStatus);
//}
//FilterCriteria::FilterCriteria(std::string appInstanceId, std::vector<AssociateId> associateId,MobilityStatus mobilityStatus)
//{
//    appInstanceId_ = appInstanceId;
//    associateId_ = associateId;
//    mobilityStatus_ = mobilityStatus;
//}
FilterCriteria::~FilterCriteria()
{

}

bool FilterCriteria::fromJson(const nlohmann::ordered_json& json)
{
    EV << "FilterCriteria::Building FilterCriteria attribute from json" << endl;
    // FIXME do value checking
    appInstanceId_ = json["appInstanceId"];
    for(auto &val : json["associateId"].items()){
        nlohmann::ordered_json associateId = val.value();
        AssociateId a;
        a.setType(associateId["type"]);
        a.setValue(associateId["value"]);
        associateId_.push_back(a);
    }
    setMobilityStatusFromString(json["mobilityStatus"]);

    return true;
}

nlohmann::ordered_json FilterCriteria::toJson() const
{
    nlohmann::ordered_json val;
    val["appInstanceId"] = appInstanceId_;
    val["associateId"] = nlohmann::json::array();
    for(auto it = associateId_.begin(); it != associateId_.end(); ++it )
    {
        val["associateId"].push_back(it->toJson());
    }
    val["mobilityStatus"] = getMobilityStatusString();

    return val;
}

void FilterCriteria::setMobilityStatusFromString(std::string mobilityStatus)
{
    // FIXME static method
    for(int i = 0; i < 3; i++)
    {
        if(MobilityStatusString[i] == mobilityStatus){
            setMobilityStatus(static_cast<MobilityStatus> (i));
            return;
        }
    }
    setMobilityStatus(static_cast<MobilityStatus> (0));
}
