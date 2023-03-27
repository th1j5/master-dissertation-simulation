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

#include "AdjacencyManager.h"

#include "inet/common/ModuleAccess.h"
#include "inet/linklayer/common/InterfaceTag_m.h"

using namespace inet; // more OK to use in .cc

Register_Abstract_Class(AdjacencyManager);

void AdjacencyManager::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        // TODO: choose better place
        // disable animations for the toplevel module
        cModule *network = getSimulation()->getSystemModule();
        network->setBuiltinAnimationsAllowed(false);
        // UDP ports
        clientPort = 71; // FIXME: does this work?
        serverPort = 70;
        // get the routing table to update and subscribe it to the blackboard
        irt.reference(this, "routingTableModule", true);
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        // get the hostname
        host = getContainingNode(this);
        // for a wireless interface subscribe the association event to start the DHCP protocol
        host->subscribe(l2AssociatedSignal, this); // TODO: only for client?
        host->subscribe(interfaceDeletedSignal, this);
        socket.setCallback(this);
        socket.setOutputGate(gate("socketOut"));
        openSocket();
    }
}

void AdjacencyManager::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        handleSelfMessages(msg);
    }
    else if (msg->arrivedOn("socketIn")) {
        socket.processMessage(msg);
    }
    else
        throw cRuntimeError("Unknown incoming gate: '%s'", msg->getArrivalGate()->getFullName());
}

void AdjacencyManager::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    // process incoming packet
    handleAdjMgmtMessage(packet);
    delete packet;
}
void AdjacencyManager::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "Ignoring UDP error report " << indication->getName() << endl;
    delete indication;
}
void AdjacencyManager::socketClosed(UdpSocket *socket_) {}

void AdjacencyManager::sendToUdp(Packet *msg, int srcPort, const L3Address& destAddr, int destPort)
{
    EV_INFO << "Sending packet " << msg << endl;
    msg->addTagIfAbsent<InterfaceReq>()->setInterfaceId(ie->getInterfaceId());
    socket.sendTo(msg, destAddr, destPort);
}

void AdjacencyManager::handleStartOperation(LifecycleOperation *operation)
{
    ie = chooseInterface();
    macAddress = ie->getMacAddress();
}

void AdjacencyManager::handleStopOperation(LifecycleOperation *operation)
{
    ie = nullptr;

    socket.close();
}

void AdjacencyManager::handleCrashOperation(LifecycleOperation *operation)
{
    ie = nullptr;

    if (operation->getRootModule() != getContainingNode(this)) // closes socket when the application crashed only
        socket.destroy(); // TODO  in real operating systems, program crash detected by OS and OS closes sockets of crashed programs.
}

NetworkInterface *AdjacencyManager::chooseInterface(const char *interfaceName)
{
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    if (interfaceName == nullptr)
        interfaceName = par("newLocInterface");
    NetworkInterface *ie = nullptr;

    if (strlen(interfaceName) > 0) {
        ie = ift->findInterfaceByName(interfaceName);
        if (ie == nullptr)
            throw cRuntimeError("Interface \"%s\" does not exist", interfaceName);
    }
    else {
        // there should be exactly one non-loopback interface that we want to configure
        for (int i = 0; i < ift->getNumInterfaces(); i++) {
            NetworkInterface *current = ift->getInterface(i);
            if (!current->isLoopback()) {
                if (ie)
                    throw cRuntimeError("Multiple non-loopback interfaces found, please select explicitly which one you want to configure via DHCP");
                ie = current;
            }
        }
        if (!ie)
            throw cRuntimeError("No non-loopback interface found to be configured via DHCP");
    }
    return ie;
}
