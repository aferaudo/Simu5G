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

#include "TargetAppInfo.h"

TargetAppInfo::TargetAppInfo() {
    // TODO Auto-generated constructor stub

}

TargetAppInfo::~TargetAppInfo() {
    // TODO Auto-generated destructor stub
}

nlohmann::ordered_json TargetAppInfo::toJson() const {
    nlohmann::ordered_json val;
    val["appInstanceId"] = appInstanceId_;
    val["commInterface"]["ipAddresses"] = nlohmann::json::array();
    auto it = commInterface_.begin();

    for(; it!=commInterface_.end(); ++it)
    {
        nlohmann::ordered_json arrayVal;
        arrayVal["host"] = it->addr.str();
        arrayVal["port"] = it->port;
        val["commonInterface"]["ipAddresses"].push_back(arrayVal);
    }

    return val;
}

bool TargetAppInfo::fromJson(const nlohmann::ordered_json &json)
{
    EV << "TargetAppInfo::processing json" << endl;
    if(!json.contains("appInstanceId"))
        return false;


    appInstanceId_ = json["appInstanceId"];

    if(json["commInterface"]["ipAddresses"].size() != 0)
    {
        for(auto it=json["commInterface"]["ipAddresses"].begin(); it != json["commInterface"]["ipAddresses"].end(); ++it)
        {
            SockAddr host;
            if(it.key() == "host")
                host.addr = inet::L3AddressResolver().resolve(std::string(it.value()).c_str());
            if(it.key() == "port")
                host.port = it.value();
            commInterface_.push_back(host);
        }
    }
    return true;

}
