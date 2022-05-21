

#include "ApplicationMobilityResource.h"

ApplicationMobilityResource::ApplicationMobilityResource()
{
    serviceConsumers_.clear();
}

ApplicationMobilityResource::~ApplicationMobilityResource()
{
    serviceConsumers_.clear();
}

nlohmann::ordered_json ApplicationMobilityResource::toJson() const
{
    EV << "ApplicationMobilityResource::toJson - returning all registered service consumers" << endl;
    nlohmann::ordered_json registeredInfo;
    nlohmann::ordered_json val;

    auto it = serviceConsumers_.begin();

    if(it == serviceConsumers_.end())
    {
        EV << "ApplicationMobilityResource::No value to return!" << endl;
        // TODO Do something
        return registeredInfo; // should be empty
    }

    for(;it != serviceConsumers_.end(); ++it)
    {
        val = it->second->toJson();
        registeredInfo.push_back(val);
    }

    return registeredInfo;

}

nlohmann::ordered_json ApplicationMobilityResource::toJsonFromId(std::string appMobilityServiceId)
{
    nlohmann::ordered_json toReturn;
    auto it = serviceConsumers_.find(appMobilityServiceId);
    if(it == serviceConsumers_.end())
    {
        EV << "ApplicationMobilityResource::no service consumer found with id: " << appMobilityServiceId << endl;
        return toReturn;
    }
    toReturn = it->second->toJson();
    return toReturn;
}

void ApplicationMobilityResource::addRegistrationInfo(RegistrationInfo* r)
{
    EV << "ApplicationMobilityResource::Registering new service consumer" << endl;
    serviceConsumers_.insert(std::pair<std::string,RegistrationInfo*> (r->getAppMobilityServiceId(), r));
    printRegisteredServiceConsumers();
}

bool ApplicationMobilityResource::updateRegistrationInfo(std::string appMobilityServiceId, RegistrationInfo *r)
{
    EV << "ApplicationMobilityResource::updating registration info"<< endl;
    auto it = serviceConsumers_.find(appMobilityServiceId);
    if(it == serviceConsumers_.end())
    {
        EV << "ApplicationMobilityResource::no service consumer found - put" << endl;
        return false;
    }
    r->setAppMobilityServiceId(appMobilityServiceId);
    it->second = r;
    printRegisteredServiceConsumers();
    return true;
}

bool ApplicationMobilityResource::removeRegistrationInfo(std::string appMobilityServiceID)
{
    EV << "ApplicationMobilityResource::Removing registration info" << endl;
    auto it = serviceConsumers_.find(appMobilityServiceID);
    if(it == serviceConsumers_.end())
    {
        EV << "ApplicationMobilityResource::no service consumer found - delete" << endl;
        return false;
    }
    serviceConsumers_.erase(it);
    printRegisteredServiceConsumers();
    return true;
}


void ApplicationMobilityResource::printRegisteredServiceConsumers()
{
    EV << "ApplicationMobilityResource::Service Consumers updated" << endl;
    auto it = serviceConsumers_.begin();

    for(; it != serviceConsumers_.end(); ++it)
    {
        it->second->printRegistrationInfo();

    }
    EV << "--------------------------------------------------------" << endl;
}

std::vector<std::string> ApplicationMobilityResource::getAppInstanceIds(
        std::vector<AssociateId> associateId) {
    std::vector<std::string> appInstanceIds;
    bool found = false;
    if(associateId.size() == 0)
    {
        EV << "ApplicationMobilityResource::getAppInstanceIds - no associateId specified - skipping..." << endl;
        return appInstanceIds;
    }
    for(auto serviceConsumer : serviceConsumers_)
    {
        for(auto devInfo : serviceConsumer.second->getDeviceInformation())
        {
            for(auto id : associateId)
            {
                if(devInfo.getAssociateId().getType() == id.getType()
                        && devInfo.getAssociateId().getValue() == id.getValue())
                {
                    appInstanceIds.push_back(serviceConsumer.second->getServiceConsumerId().appInstanceId);
                    found = true;
                    break;
                }

            }
            if(found)
                break;
        }
        found = false;
    }
    EV << "ApplicationMobilityResource::getAppInstanceIds - result: " << found << endl;

    return appInstanceIds;
}
