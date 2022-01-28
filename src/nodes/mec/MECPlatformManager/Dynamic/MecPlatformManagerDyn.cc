//
//  MecPlatformManagerDyn.cc
//
//  @author Angelo Feraudo
//  @author Alessandro Calvio
//

// Imports
#include <sstream>

#include "nodes/mec/MECPlatformManager/Dynamic/MecPlatformManagerDyn.h"
#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"

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
    vim = check_and_cast<VirtualisationInfrastructureManagerDyn*>(getParentModule()->getParentModule()->getSubmodule("vim")->getSubmodule("app", 0));
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

    if (msg->isSelfMessage()){
        EV << "MecPlatformManagerDyn::handleMessage - self message received - " << msg << endl;

    }else{
        EV << "MecPlatformManagerDyn::handleMessage - other message received - " << msg << endl;

        inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
        if (pPacket == 0)
            throw cRuntimeError("MecPlatformManagerDyn::handleMessage - FATAL! Error when casting to inet packet");

        if (!strcmp(msg->getName(), "Instantiation")){
            auto data = pPacket->peekData<CreateAppMessage>();
            inet::PacketPrinter printer;
            printer.printPacket(std::cout, pPacket);

            // Ricostruzione pacchetto TODO cambiare
            CreateAppMessage * createAppMsg = new CreateAppMessage();
            createAppMsg->setUeAppID(data->getUeAppID());
            createAppMsg->setMEModuleName(data->getMEModuleName());
            createAppMsg->setMEModuleType(data->getMEModuleType());

            createAppMsg->setRequiredCpu(data->getRequiredCpu());
            createAppMsg->setRequiredRam(data->getRequiredRam());
            createAppMsg->setRequiredDisk(data->getRequiredDisk());

            createAppMsg->setRequiredService(data->getRequiredService());
            createAppMsg->setContextId(data->getContextId());

            instantiateMEApp(createAppMsg);

            getParentModule()->bubble("Richiesta di instanziazione ricevuta");
        }
        else if(!strcmp(msg->getName(), "Termination")){

            auto data = pPacket->peekData<DeleteAppMessage>();
//            inet::PacketPrinter printer;
//            printer.printPacket(std::cout, pPacket);

            // Ricostruzione pacchetto TODO cambiare
            DeleteAppMessage * deleteAppMsg = new DeleteAppMessage();
            deleteAppMsg->setUeAppID(data->getUeAppID());

            terminateMEApp(deleteAppMsg);
        }
        else{
            EV << "MecPlatformManagerDyn::handleMessage - unknown package" << endl;
        }

//        std::cout << "IP received " << pPacket->getTag<inet::L3AddressInd>()->getSrcAddress() << endl;
//        std::cout << "NAME received " << msg->getName() << endl;

    }

    delete msg;
}

MecAppInstanceInfo* MecPlatformManagerDyn::instantiateMEApp(CreateAppMessage* msg)
{
    EV << "MecPlatformManagerDyn::instantiateMEApp" << endl;
    MecAppInstanceInfo* res = vim->instantiateMEApp(msg);
    delete msg;
    return res;
}

bool MecPlatformManagerDyn::instantiateEmulatedMEApp(CreateAppMessage* msg)
{
    EV << "MecPlatformManagerDyn::instantiateEmulatedMEApp" << endl;
    bool res = vim->instantiateEmulatedMEApp(msg);
    delete msg;
    return res;
}

bool MecPlatformManagerDyn::terminateEmulatedMEApp(DeleteAppMessage* msg)
{
    EV << "MecPlatformManagerDyn::terminateEmulatedMEApp" << endl;
    bool res = vim->terminateEmulatedMEApp(msg);
    delete msg;
    return res;
}


bool MecPlatformManagerDyn::terminateMEApp(DeleteAppMessage* msg)
{
    EV << "MecPlatformManagerDyn::terminateMEApp" << endl;
    bool res = vim->terminateMEApp(msg);
    delete msg;
    return res;
}

const std::vector<ServiceInfo>* MecPlatformManagerDyn::getAvailableMecServices() const
{
    EV << "MecPlatformManagerDyn::getAvailableMecServices" << endl;
    if(serviceRegistry == nullptr)
        return nullptr;
    else
    {
       return serviceRegistry->getAvailableMecServices();
    }
}

const std::set<std::string>* MecPlatformManagerDyn::getAvailableOmnetServices() const
{
    EV << "MecPlatformManagerDyn::getAvailableOmnetServices" << endl;
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


