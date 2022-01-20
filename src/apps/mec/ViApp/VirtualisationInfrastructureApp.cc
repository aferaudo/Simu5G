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

// Imports
#include <sstream>

#include "VirtualisationInfrastructureApp.h"
#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"

Define_Module(VirtualisationInfrastructureApp);

VirtualisationInfrastructureApp::VirtualisationInfrastructureApp()
{

}

void VirtualisationInfrastructureApp::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER-1){
        return;
    }

    EV << "VirtualisationInfrastructureApp::initialize - stage " << stage << endl;

    // Get parameters
    localPort = par("localPort");
    vimPort = par("vimPort");

    localAddress = inet::L3AddressResolver().resolve(par("localAddress"));
    vimAddress = inet::L3AddressResolver().resolve(par("vimAddress"));

    if (localPort != -1)
    {
        socket.setOutputGate(gate("socketOut"));
        socket.bind(localPort);
    }
    EV << simTime() << "VirtualisationInfrastructureApp::initialize - binding to port: local:" << localPort << " , dest: " << vimAddress.str() << ":" << vimPort << endl;

}

void VirtualisationInfrastructureApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        EV << "VirtualisationInfrastructureApp::handleMessage - self message received!" << endl;

    }else{
        EV << "VirtualisationInfrastructureApp::handleMessage - other message received!" << endl;
    }
}
