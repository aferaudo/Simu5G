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

#include "NotificationBase.h"

NotificationBase::NotificationBase() {
    // TODO Auto-generated constructor stub

}

NotificationBase::~NotificationBase() {
    // TODO Auto-generated destructor stub
}



bool NotificationBase::fromJson(const nlohmann::ordered_json& json)
{
    EV << "NotificationBase::Building NotificationBase attribute from json" << endl;
    if(!json.contains("notificationType") || !json.contains("_links"))
    {
        EV << "NotificationBase::Some required parameters is missing!" << endl;
        return false;
    }

    if(json.contains("timeStamp"))
    {
        timestamp_.setSeconds(json["timeStamp"]["seconds"]);
        timestamp_.setNanoSeconds(json["timeStamp"]["nanoSeconds"]);
    }
    if(json.contains("associateId"))
    
    {
        for(auto& it : json["associateId"].items())
        {
            AssociateId a;

            nlohmann::ordered_json val = it.value();

            a.setType(val["type"]);
            a.setValue(val["value"]);
            associateId_.push_back(a);
        }
    }

    // This should be checked inside the notification class
    // notificationType_ = json["notificationType"];

    links_ = json["_links"]["href"];
}