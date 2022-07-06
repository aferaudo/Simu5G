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

#include "RegistrationInfo.h"


RegistrationInfo::RegistrationInfo() {
    // TODO Auto-generated constructor stub

}

RegistrationInfo::RegistrationInfo(ServiceConsumerId scId)
{

    RegistrationInfo(scId, "", std::vector<DeviceInformation>(), 0);
}

RegistrationInfo::RegistrationInfo(ServiceConsumerId scId, std::string appMobSId, std::vector<DeviceInformation>devInfo, int exTime)
{
    serviceConsumerId = scId;
    appMobilityServiceId = appMobSId;
    deviceInformation = devInfo;
    expiryTime = exTime;
}

RegistrationInfo::~RegistrationInfo() {
    // TODO Auto-generated destructor stub
}


void RegistrationInfo::setAppMobilityServiceId(std::string appMobSId)
{
    appMobilityServiceId = appMobSId;
}

nlohmann::ordered_json RegistrationInfo::toJson() const
{
    nlohmann::ordered_json val;
    val["appMobilityServiceId"] = appMobilityServiceId;
    val["deviceInformation"] = nlohmann::json::array();
    for(auto devInfo : deviceInformation)
        val["deviceInformation"].push_back(devInfo.toJson());

    val["expireTime"] = expiryTime;
    val["serviceConsumerId"]["appInstanceId"] = serviceConsumerId.appInstanceId;
    val["serviceConsumerId"]["mepId"] = serviceConsumerId.mepId;
    return val;
}

bool RegistrationInfo::fromJson(const nlohmann::ordered_json& object)
{
    /*
     * Required parameters are in accordance with the standard
     * ETSI 021 v2.2.1
     */
    // serviceConsumerId mandatory parameter

    EV << "RegisterInfo analysing object: " << object.dump().c_str()<<endl;
    if(!object.contains("serviceConsumerId") || !object["serviceConsumerId"].contains("appInstanceId")
           || !object["serviceConsumerId"].contains("mepId"))
    {
       // These two parameters are mandatory
      EV << "RegistrationInfo::required parameters: serviceConsumerID not specified" << endl;
      return false;
    }

    // Device information is not mandatory
    // from ETSI 021 v2.2.1:
    // If present, it specifies the device served by the application instance which is registering the Application Mobility Service
    // This may be due to the fact that apps may run in background without interacting with an UE App
    if(object.contains("deviceInformation"))
    {
        EV << "RegistrationInfo::Adding device information! " << object["deviceInformation"].dump().c_str() << endl;
        for(auto& item : object["deviceInformation"].items())
        {
            DeviceInformation val;
            val.fromJson(item.value());
            deviceInformation.push_back(val);
        }

    }
    else
    {
        EV << "RegistrationInfo::deviceInformation not specified"<< endl;
        DeviceInformation devInfo;
        AssociateId a;
        a.setType("");
        a.setValue("");
        devInfo.setAssociateId(a);
        devInfo.setAppMobilityServiceLevel(APP_MOBILITY_NOT_ALLOWED);
        devInfo.setContextTransferState(NOT_TRANSFERRED);
        deviceInformation.push_back(devInfo);
    }



    // ExpiryTime not mandatory
    expiryTime = 0;
    if(object.contains("expireTime"))
       expiryTime = object["expireTime"];



    serviceConsumerId.appInstanceId = object["serviceConsumerId"]["appInstanceId"];
    serviceConsumerId.mepId = object["serviceConsumerId"]["mepId"];
    return true;
}



void RegistrationInfo::printRegistrationInfo()
{
    EV << "RegistrationInfo: " << endl;
    EV << "     AppMobilityServiceId: " << getAppMobilityServiceId() << endl;
    for(auto devInfo : deviceInformation)
    {
        EV << "     DeviceInformation: " << endl;
        EV << "         associateIdType: "<< devInfo.getAssociateId().getType() << endl;
        EV << "         associateIdValue: "<< devInfo.getAssociateId().getValue() << endl;
        EV << "         appMobilityServiceLevel: "<< devInfo.getAppMobilityServiceLevelString() << endl;
        EV << "         contextTransferState: "<< devInfo.getContextTransferStateString() << endl;
    }
    EV << "     serviceConsumerAppId: "<< getServiceConsumerId().appInstanceId << endl;
    EV << "     serviceConsumerMepId: "<< getServiceConsumerId().mepId << endl;
}
