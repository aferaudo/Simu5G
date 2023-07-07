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

#include "AddressChangeNotification.h"

AddressChangeNotification::AddressChangeNotification() {
    notificationType = "AddressChangeNotification";

}

AddressChangeNotification::~AddressChangeNotification() {
    // TODO Auto-generated destructor stub
}

nlohmann::ordered_json AddressChangeNotification::toJson() const{

    nlohmann::ordered_json object;
    object["notificationType"] = notificationType;
    object["contextId"] = contextId;
    object["appInstanceId"] = appInstanceId;
    object["referenceURI"] = referenceURI;
    return object;
}

bool AddressChangeNotification::fromJson(const nlohmann::ordered_json &json)
{
    if(json.contains("notificationType"))
    {
        notificationType = json["notificationType"] ;
        if(json.contains("contextId"))
        {
            contextId = json["contextId"];
            if(json.contains("appInstanceId"))
            {
                appInstanceId = json["appInstanceId"];
                if(json.contains("referenceURI"))
                {
                    referenceURI = json["referenceURI"];
                    return true;
                }
                else
                    return false;
            }
            else
            {
                return false;
            }
        }
        else
            return false;
    }
    else
        return false;




}
