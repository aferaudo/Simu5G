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
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h" //ServiceInfo struct


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
    vim = check_and_cast<VirtualisationInfrastructureManagerDyn*>(getParentModule()->getParentModule()->getSubmodule("vim")->getSubmodule("vimApp"));
    cModule* mecPlatform = getParentModule()->getParentModule()->getSubmodule("mecPlatform");
    if(mecPlatform != nullptr && mecPlatform->findSubmodule("serviceRegistry") != -1)
    {
        serviceRegistry = check_and_cast<ServiceRegistry*>(mecPlatform->getSubmodule("serviceRegistry"));
    }else{
        EV << "MecPlatformManagerDyn::initialize - unable to find mecPlatform or serviceRegistry" << endl;
    }

    // Read parameters
    meoAddress = inet::L3AddressResolver().resolve(par("meoAddress"));
    localAddress = inet::L3AddressResolver().resolve(par("localAddress"));
    vimAddress = inet::L3AddressResolver().resolve(par("vimAddress"));
    meoPort = par("meoPort");
    localPort = par("localPort");
    vimPort = par("vimPort");

    std::cout << getParentModule()->getSubmodule("interfaceTable") << endl;
    ifacetable = check_and_cast<inet::InterfaceTable*>(getParentModule()->getSubmodule("interfaceTable"));
    EV << "MecPlatformManagerDyn::initialize - ifacetable " << ifacetable->findInterfaceByName("pppIfRouter")->str() << endl;

    // Setup orchestrator communication
    if(localPort != -1){
        EV << "MecPlatformManagerDyn::initialize - binding orchestrator socket to port " << localPort << endl;
        socket.setOutputGate(gate("socketOut"));
        socket.bind(localPort);
    }

    scheduleAt(simTime()+0.01, new cMessage("register"));
}

void MecPlatformManagerDyn::handleMessage(cMessage *msg)
{
    EV << "MecPlatformManagerDyn::handleMessage - message received - " << msg << endl;

    if (msg->isSelfMessage()){
        EV << "MecPlatformManagerDyn::handleMessage - self message received - " << msg << endl;
        if(strcmp(msg->getName(), "register") == 0){
            // Register to MECOrchestrator
            std::cout << "MecPlatformManagerDyn::handleMessage - register to MEO " << msg << endl;
            sendMEORegistration();
        }

        delete msg;
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
        else if (!strcmp(msg->getName(), "ServiceRequest")){
            EV << "MecPlatformManagerDyn::handleMessage - TYPE: ServiceRequest" << endl;

            inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
            handleServiceRequest(pPacket);
            delete msg;
        }
        else if (!strcmp(msg->getName(), "instantiationApplicationRequest")){
            EV << "MecPlatformManagerDyn::handleMessage - TYPE: instantiationApplicationRequest" << endl;

            inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);
            handleInstantiationRequest(pPacket);
//            delete msg;
        }
        else if (!strcmp(msg->getName(), "instantiationApplicationResponse")){
            EV << "MecPlatformManagerDyn::handleMessage - TYPE: instantiationApplicationResponse" << endl;

            inet::Packet* pPacket = check_and_cast<inet::Packet*>(msg);

            inet::Packet* pktdup = new inet::Packet("instantiationApplicationResponse");
            auto responsemsg = inet::makeShared<InstantiationApplicationResponse>();
            auto data = pPacket->peekData<InstantiationApplicationResponse>();
            responsemsg = data.get()->dup();
            pktdup->insertAtBack(responsemsg);

            socket.sendTo(pktdup, meoAddress, meoPort);

//            delete msg;
        }

        else{
            EV << "MecPlatformManagerDyn::handleMessage - unknown package" << endl;
        }

//        std::cout << "IP received " << pPacket->getTag<inet::L3AddressInd>()->getSrcAddress() << endl;
//        std::cout << "NAME received " << msg->getName() << endl;

    }

//    delete msg;
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

void MecPlatformManagerDyn::registerMecService(ServiceDescriptor& servDescriptor) const
{
    EV << "MecPlatformManagerDyn::registerMecService" << endl;

    serviceRegistry->registerMecService(servDescriptor); //TODO Substitute with MecOrchestrator registring (?)
}



void MecPlatformManagerDyn::finish()
{
    EV << "MecPlatformManagerDyn::finish" << endl;

}

void MecPlatformManagerDyn::sendMEORegistration()
{
    inet::Packet* packet = new inet::Packet("Registration");
    auto registrationPkt = inet::makeShared<RegistrationPkt>();
    registrationPkt->setHostId(getParentModule()->getParentModule()->getId());
    registrationPkt->setType(MM3);
    registrationPkt->setSourcePort(localPort);
    registrationPkt->setChunkLength(inet::B(1000));
    packet->insertAtBack(registrationPkt);

    EV << "MecPlatformManagerDyn::sending custom packet to MEO " << endl;
    std::cout << "MecPlatformManagerDyn::sending custom packet to MEO " << endl;
    std::cout << "MecPlatformManagerDyn::sending custom packet to MEO " << meoAddress.str() << endl;
    std::cout << "MecPlatformManagerDyn::sending custom packet to MEO " << meoPort << endl;
    std::cout << "MecPlatformManagerDyn::sending custom packet to MEO " << packet->getFullName()<< endl;
    inet::PacketPrinter printer;
    printer.printPacket(std::cout, packet);
    socket.sendTo(packet, meoAddress, meoPort);

}

void MecPlatformManagerDyn::handleServiceRequest(inet::Packet* resourcePacket){

    auto data = resourcePacket->peekData<MeoMepmRequest>();
    const MeoMepmRequest *receivedData = data.get();

    const std::vector<ServiceInfo>* services = serviceRegistry->getAvailableMecServices();
    bool res = true;
    for(int i = 0; i < receivedData->getRequiredServiceNamesArraySize() && res; i++)
    {
        const char* serviceName = receivedData->getRequiredServiceNames(i);
        bool found = false;
        for(int j = 0; j < services->size() && !found; j++)
        {
            const char* availableServiceName = services->at(j).getName().c_str();
            if(std::strcmp(availableServiceName, serviceName)==0){
                found = true;
            }

        }

        res = res && found;
    }

    inet::Packet* packet = new inet::Packet("AvailabilityResponseMepm");
    auto responsePkt = inet::makeShared<MecHostResponse>();
    responsePkt->setDeviceAppId(receivedData->getDeviceAppId());
    responsePkt->setMecHostId(getParentModule()->getParentModule()->getId());
    responsePkt->setResult(res);
    responsePkt->setChunkLength(inet::B(1000));
    packet->insertAtBack(responsePkt);
    socket.sendTo(packet, meoAddress, meoPort);

    EV << "MecPlatformManagerDyn::handleResourceRequest - request result:  " << res << endl;
}

void MecPlatformManagerDyn::handleInstantiationRequest(inet::Packet* instantiationPacket){

    inet::Packet* pktdup = new inet::Packet("instantiationApplicationRequest");
    auto createAppMsg = inet::makeShared<CreateAppMessage>();
    auto data = instantiationPacket->peekData<CreateAppMessage>();
    createAppMsg = data.get()->dup();
    pktdup->insertAtBack(createAppMsg);
    pktdup->addTag<inet::InterfaceReq>()->setInterfaceId(ifacetable->findInterfaceByName("pppIfRouter")->getInterfaceId());

    EV << "MecPlatformManagerDyn::handleResourceRequest - sending:  " << vimAddress << ":" << vimPort << endl;
    socket.sendTo(pktdup, vimAddress, vimPort);
}


