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

#include "AdjacencyManagerOld_old.h"
#include "inet/common/ModuleAccess.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/nexthop/NextHopRoute.h"

using namespace inet; // more OK to use in .cc

Register_Abstract_Class(AdjacencyManagerOld);
simsignal_t AdjacencyManagerOld::oldLocRemovedSignal = cComponent::registerSignal("oldLocatorUnreachable");

AdjacencyManagerOld::~AdjacencyManagerOld() {
    cancelAndDelete(selfMsg);
}

void AdjacencyManagerOld::initialize(int stage)
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
        ift.reference(this, "interfaceTableModule", true);
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
    if (false) {
        // add all neighbours
        // we assume L2 triggers, but
        // since ppp-connections don't do that, we iterate over all ppp
        for (int i=0; i < ift->getNumInterfaces(); i++) {
            NetworkInterface *iface = ift->getInterface(i);
            if (iface->isPointToPoint()) {
                //iface->getRxTransmissionChannel()->getSourceGate();
                int oGateID = iface->getNodeOutputGateId();
                cGate* rxGate = gate(oGateID)->getPathEndGate();
                cModule* rx = getContainingNode(rxGate->getOwnerModule());
                auto* peerIft= dynamic_cast<InterfaceTable*>(rx->getSubmodule("interfaceTableModule"));
                // get neighbour ID (when unisphere)
                L3Address peerID = L3Address(peerIft->findFirstNonLoopbackInterface()->getModuleIdAddress());
                ASSERT(!peerID.isUnspecified());

                NextHopRoute* route = new NextHopRoute();
                route->setDestination(peerID);
                route->setInterface(iface);
                route->setNextHop(peerID);
                //route->setMetric(0);
                route->setAdminDist(inet::IRoute::RouteAdminDist::dDirectlyConnected);
                //route->setPrefixLength(l);
                irt->addRoute(route);
            }
        }
    }
}

void AdjacencyManagerOld::handleMessageWhenUp(cMessage *msg)
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

void AdjacencyManagerOld::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    // process incoming packet
    auto data = packet->peekData<MultiplexerPacket>();
    auto multiplexerDest = data->getMultiplexerDestination();
    if (strcasecmp(multiplexerDest, "adj_mgmt") == 0) {
        handleAdjMgmtMessage(packet);
    }
    else if (strcasecmp(multiplexerDest, "flow_allocator") == 0) {
        handleNeighMessage(packet);
    }
    else
        throw cRuntimeError("unrecognized multiplexer destination");
    delete packet;
}
void AdjacencyManagerOld::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "Ignoring UDP error report " << indication->getName() << endl;
    delete indication;
}
void AdjacencyManagerOld::socketClosed(UdpSocket *socket_) {}

void AdjacencyManagerOld::sendToUdp(Packet *msg, int srcPort, const L3Address& destAddr, int destPort)
{
    EV_INFO << "Sending packet " << msg << endl;
    msg->addTagIfAbsent<InterfaceReq>()->setInterfaceId(ie->getInterfaceId());
    socket.sendTo(msg, destAddr, destPort);
}

void AdjacencyManagerOld::handleStartOperation(LifecycleOperation *operation)
{
    ie = chooseInterface();
    macAddress = ie->getMacAddress();
}

void AdjacencyManagerOld::handleStopOperation(LifecycleOperation *operation)
{
    ie = nullptr;

    socket.close();
}

void AdjacencyManagerOld::handleCrashOperation(LifecycleOperation *operation)
{
    ie = nullptr;

    if (operation->getRootModule() != getContainingNode(this)) // closes socket when the application crashed only
        socket.destroy(); // TODO  in real operating systems, program crash detected by OS and OS closes sockets of crashed programs.
}

NetworkInterface *AdjacencyManagerOld::chooseInterface(const char *interfaceName)
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
