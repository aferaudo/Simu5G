//
//  MecPlatformManagerDyn.cc
//
//  @author Angelo Feraudo
//  @author Alessandro Calvio
//

// Imports
#include <sstream>

#include "nodes/mec/MECPlatformManager/Dynamic/MecPlatformManagerDyn.h"

Define_Module(MecPlatformManagerDyn);

MecPlatformManagerDyn::MecPlatformManagerDyn()
{
    vim = nullptr;
    serviceRegistry = nullptr;
}

void MecPlatformManagerDyn::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER-1){
        return;
    }

    EV << "MecPlatformManagerDyn::initialize - stage " << stage << endl;

    // Get other modules
    vim = check_and_cast<VirtualisationInfrastructureManagerDyn*>(getParentModule()->getSubmodule("app", 0));
    cModule* mecPlatform = getParentModule()->getSubmodule("mecPlatform");
    if(mecPlatform != nullptr && mecPlatform->findSubmodule("serviceRegistry") != -1)
    {
        serviceRegistry = check_and_cast<ServiceRegistry*>(mecPlatform->getSubmodule("serviceRegistry"));
    }else{
        EV << "MecPlatformManagerDyn::initialize - unable to find mecPlatform or serviceRegistry" << endl;
    }

    // Read parameters
    orchestratorAddress = inet::L3AddressResolver().resolve(par("mecOrchestratorAddress"));
    localAddress = inet::L3AddressResolver().resolve(par("localAddress"));

    orchestratorPort = par("mecOrchestratorPort");
    localPort = par("localPort");

    // Setup orchestrator communication
    if(localPort != -1){
        EV << "MecPlatformManagerDyn::initialize - binding orchestrator socket to port " << localPort << endl;
        orchestratorSocket.setOutputGate(gate("socketOut"));
        orchestratorSocket.bind(localPort);
    }
}

void MecPlatformManagerDyn::handleMessage(cMessage *msg)
{
    EV << "MecPlatformManagerDyn::handleMessage - message received - " << msg << endl;

}

MecAppInstanceInfo* MecPlatformManagerDyn::instantiateMEApp(CreateAppMessage* msg)
{
   MecAppInstanceInfo* res = vim->instantiateMEApp(msg);
   delete msg;
   return res;
}

bool MecPlatformManagerDyn::instantiateEmulatedMEApp(CreateAppMessage* msg)
{
    bool res = vim->instantiateEmulatedMEApp(msg);
    delete msg;
    return res;
}

bool MecPlatformManagerDyn::terminateEmulatedMEApp(DeleteAppMessage* msg)
{
    bool res = vim->terminateEmulatedMEApp(msg);
    delete msg;
    return res;
}


bool MecPlatformManagerDyn::terminateMEApp(DeleteAppMessage* msg)
{
    bool res = vim->terminateMEApp(msg);
    delete msg;
    return res;
}

const std::vector<ServiceInfo>* MecPlatformManagerDyn::getAvailableMecServices() const
{
    if(serviceRegistry == nullptr)
        return nullptr;
    else
    {
       return serviceRegistry->getAvailableMecServices();
    }
}

const std::set<std::string>* MecPlatformManagerDyn::getAvailableOmnetServices() const
{
    if(serviceRegistry == nullptr)
        return nullptr;
    else
    {
       return serviceRegistry->getAvailableOmnetServices();
    }
}

void MecPlatformManagerDyn::registerMecService(ServiceDescriptor&) const
{
    EV << "MecPlatformManagerDyn::registerMecService" << endl;
}



void MecPlatformManagerDyn::finish()
{
    EV << "MecPlatformManagerDyn::finish" << endl;

}


