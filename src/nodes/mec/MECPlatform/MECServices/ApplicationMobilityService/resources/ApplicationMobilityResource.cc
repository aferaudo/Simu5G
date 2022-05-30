

#include "ApplicationMobilityResource.h"

ApplicationMobilityResource::ApplicationMobilityResource()
{
    migratedApps_.clear();
    serviceConsumers_.clear();
}

ApplicationMobilityResource::~ApplicationMobilityResource()
{
    serviceConsumers_.clear();
    migratedApps_.clear();
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
    // If app has been migrated change its state
    // remeber correspondence 1 to 1 (1 mecapp 1 ue)
    if(r->getDeviceInformation()[0].getContextTransferState() == USER_CONTEXT_TRANSFER_COMPLETED)
    {
        auto migratingApp = migratingApps_.find(r->getServiceConsumerId().appInstanceId);
        if(migratingApp == migratingApps_.end())
        {
            EV_ERROR << "ApplicationMobilityResource::app has not been migrated" << endl;
        }
        else
        {
           EV << "ApplicationMobilityResource::migration of app: " << migratingApp->second->getAppInstanceId() << " completed!" << endl;
           addMigratedApp(migratingApp->second);
           migratingApps_.erase(migratingApp);
        }

    }
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
                EV << "Comparing " << devInfo.getAssociateId().getValue() << " with " << id.getValue() << " and " << devInfo.getAssociateId().getType() << " with " << id.getType() << endl;
                if(devInfo.getAssociateId().getType() == id.getType()
                        && devInfo.getAssociateId().getValue() == id.getValue())
                {
                    EV << "ApplicationMobilityResource::an app has been found!" << endl;
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

bool ApplicationMobilityResource::addMigratedApp(TargetAppInfo* targetInfo)
{
    auto it = migratedApps_.find(targetInfo->getAppInstanceId());
    if(it != migratedApps_.end())
    {
        EV << "ApplicationMobilityResource::migrating app already present" << endl;

//        if(targetInfo->getCommInterface() == it->second->getCommInterface())
//        {
//            EV << "ApplicationMobilityResource::same event received...nothing to do" << endl;
//            return false;
//        }

        migratedApps_.erase(it);
    }
    migratedApps_.insert(std::pair<std::string, TargetAppInfo *>(targetInfo->getAppInstanceId(), targetInfo));

    EV << "ApplicationMobilityResource::migrating app updated correctly..." << endl;

    return true;
}

bool ApplicationMobilityResource::removingMigratedApp(std::string appInstanceId)
{
    auto it = migratedApps_.find(appInstanceId);
    if(it == migratedApps_.end())
    {
        EV << "ApplicationMobilityResource::migrated app not found!" << endl;
        return false;
    }

    migratedApps_.erase(it);
    EV << "ApplicationMobilityResource::app - " << appInstanceId << " - deleted!" << endl;

    return true;
}

void ApplicationMobilityResource::addMigratingApp(TargetAppInfo* target) {
    EV << "ApplicationMobilityResource:: app - " << target->getAppInstanceId() << " - migrating to " << target->getCommInterface()[0].str() << endl;

    auto itApp = migratingApps_.find(target->getAppInstanceId());

    if(itApp != migratingApps_.end())
    {
        EV << "ApplicationMobilityResource:: app - " << target->getAppInstanceId() << " - already added! Nothing to do..." <<endl;
        return;
    }

    migratingApps_.insert(std::pair<std::string, TargetAppInfo *>(target->getAppInstanceId(), target));
    EV << "ApplicationMobilityResource:: app - " << target->getAppInstanceId() << " - correctly added!" << endl;

}

bool ApplicationMobilityResource::removeMigratingApp(std::string migratingApp) {

    auto it = migratingApps_.find(migratingApp);
    if(it == migratingApps_.end())
    {
        EV << "ApplicationMobilityResource::migrating app not found!" << endl;
        return false;
    }

    migratingApps_.erase(it);
    EV << "ApplicationMobilityResource:: app - " << migratingApp << " - correctly deleted!" << endl;
    return true;
}

RegistrationInfo* ApplicationMobilityResource::getRegistrationInfoFromAppId(
        std::string appInstanceId) const {

    for(auto &el : serviceConsumers_)
    {
        if(el.second->getServiceConsumerId().appInstanceId.compare(appInstanceId) == 0)
            return el.second;
    }

    return nullptr;
}
