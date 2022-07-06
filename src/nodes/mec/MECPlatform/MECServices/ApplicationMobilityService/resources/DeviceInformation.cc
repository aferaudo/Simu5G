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

#include "DeviceInformation.h"

DeviceInformation::DeviceInformation() {
    // TODO Auto-generated constructor stub

}

DeviceInformation::~DeviceInformation() {
    // TODO Auto-generated destructor stub
}

nlohmann::ordered_json DeviceInformation::toJson() const {

    nlohmann::ordered_json val;
    val["deviceInformation"]["associateId"] = associateId_.toJson();
    val["deviceInformation"]["appMobilityServiceLevel"] = getAppMobilityServiceLevelString();
    val["deviceInformation"]["contextTransferState"] = getContextTransferStateString();
    return val;
}

void DeviceInformation::setAppMobilityServiceLevelFromString(
        std::string appMobilityServiceLevel) {
    for(int i = 0; i < 3; i++) // very static for (should not be a problem since we are talking about a standard)
     {
         if(appMobilityServiceLevel == AppMobilityServiceLevelStrings[i]){
             setAppMobilityServiceLevel(static_cast<AppMobilityServiceLevel> (i));
             return;
         }
     }

     setAppMobilityServiceLevel(APP_MOBILITY_NOT_ALLOWED);
}

void DeviceInformation::setContextTransferStateFromString(
        std::string contextTransferState) {

    for(int i = 0; i < 2; i++) // very static for
   {
       if(contextTransferState == ContextTransferStateStrings[i]){
           setContextTransferState(static_cast<ContextTransferState> (i));
           return;
       }
   }

    setContextTransferState(NOT_TRANSFERRED);
}

bool DeviceInformation::fromJson(const nlohmann::ordered_json& object) {
   if(!object.contains("associateId"))
   {
       // This parameter is mandatory when device info is present
       EV << "DeviceInformation::required parameters: serviceConsumerID not specified" << endl;
       return false;

   }

   associateId_.setType(object["associateId"]["type"]);
   associateId_.setValue(object["associateId"]["value"]);


   if(object.contains("appMobilityServiceLevel"))
       setAppMobilityServiceLevelFromString(object["appMobilityServiceLevel"]);
   else
       appMobilityServiceLevel_ = APP_MOBILITY_NOT_ALLOWED; // default value

   //EV << "Here we are: " << appMobilityServiceLevel_ << endl;



   if(object.contains("contextTransferState"))
       setContextTransferStateFromString(object["contextTransferState"]);
   else
       contextTransferState_ = NOT_TRANSFERRED; // default value


   return true;
}
