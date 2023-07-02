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
#include "../UniSphere/UniSphereRoute.h"
#include "../util.h"
#include "inet/networklayer/nexthop/NextHopRoute.h"
#include "inet/networklayer/nexthop/NextHopRoutingTable.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/mobility/contract/IMobility.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"

#include <list>
#include <queue>

using namespace inet; // more OK to use in .cc

Define_Module(AdjacencyManager);

/**
 * routerId (using it as an ID?)
 * NextHopRoutingTable: is highest interface address --> L3Address (but which type?)
 * IPv4RoutingTable:    ?
 */

void AdjacencyManager::initialize(int stage) {
    if (stage == INITSTAGE_LOCAL) {
        // get the routing table to update and subscribe it to the blackboard
        irt.reference(this, "routingTableModule", true);
        ift.reference(this, "interfaceTableModule", true);
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        // get the hostname
        host = getContainingNode(this);
        // for a wireless interface subscribe the association event to start the DHCP protocol
        //host->subscribe(l2AssociatedSignal, this); // TODO: only for client?
        //host->subscribe(interfaceDeletedSignal, this);
        host->subscribe(IMobility::mobilityStateChangedSignal, this);

        // add all neighbours
        // we assume L2 triggers, but
        // since ppp-connections don't do that, we iterate over all ppp
        if (auto* irt = dynamic_cast<NextHopRoutingTable*>(this->irt.get())) {
            for (int i=0; i < ift->getNumInterfaces(); i++) {
                NetworkInterface *iface = ift->getInterface(i);
                if (iface->isPointToPoint()) {
                    cGate* rxGate = iface->getRxTransmissionChannel()->getSourceGate();
//                    int outGateID = iface->getNodeOutputGateId();
//                    cGate* rxGate = gate(outGateID)->getPathEndGate();
                    cModule* rx = getContainingNode(rxGate->getOwnerModule());
                    connectNode(rx, iface);
                }
            }
        }
    }
}

// TODO what are the semantics of this?? does it connects IPCPs or Locs or ...?
void AdjacencyManager::connectNode(cModule* neighbour, NetworkInterface * iface) {
    //FIXME (will probably result in mayhem - or not, apparently, MODULEID doesn't even uses this, but IPvX & MODULEPATH does)
    // see routerId remark at start
    int longestPrefix = std::numeric_limits<int>::max();

    L3Address peerID = getHostID(neighbour);

    // check if already in RoutingTable
    bool routeAlreadyPresent = false;
    IRoute *e = irt->findBestMatchingRoute(peerID);
    if (e != nullptr
            && e->getDestinationAsGeneric() == peerID
            && e->getNextHopAsGeneric() == peerID
            && e->getInterface() == iface
            && e->getMetric() == 0)
        routeAlreadyPresent = true;

    if (isUniSphere() && !routeAlreadyPresent) {
        UniSphereRoute* route = new UniSphereRoute(peerID);
        route->setInterface(iface);
//        route->setLandmark()
//        route->vicinity = true;
//        route->forwardPath = ;
//                    route->setAdminDist(inet::IRoute::RouteAdminDist::dDirectlyConnected); // only IPv4
//        route->setPrefixLength(longestPrefix);
        irt->addRoute(route);
    }
    else if (!isUniSphere() && !routeAlreadyPresent) {
        // IPv4 case
        ASSERT(false);
    }
}
void AdjacencyManager::disconnectNode(cModule* neighbour) {
    L3Address peerID = getHostID(neighbour);

    IRoute *e = irt->findBestMatchingRoute(peerID);
    while (e != nullptr
                && e->getDestinationAsGeneric() == peerID
                && e->getNextHopAsGeneric() == peerID
                && e->getMetric() == 0)
    {
        irt->deleteRoute(e);
        e = irt->findBestMatchingRoute(peerID);
    }
}

void AdjacencyManager::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) {
    Enter_Method("%s", cComponent::getSignalName(signalID));
    if (signalID == IMobility::mobilityStateChangedSignal) {
        // make decisions to adjust graph
        ASSERT((std::string) host->getNedTypeName() == "prototype.LocNodes.MobileNode");

        auto nodesInRangeSorted = getNodesInRangeSorted();
        auto connectedNodes = getConnectedNodes(irt);
        EV_WARN << "Connected to: "; print(connectedNodes); EV_WARN << endl;
        if (!nodesInRangeSorted.empty()) {
            EV_WARN << "Connecting to:" << nodesInRangeSorted.back() << endl;
            // connect
            connectNode(nodesInRangeSorted.back(), ift->findInterfaceByName("wlan0"));
        }
        // disconnect
        // (when out-of-reach)
        for (auto n: connectedNodes) {
            if (!contains(nodesInRangeSorted, n))
                disconnectNode(n);
        }
    }
    else throw cRuntimeError("Unexpected signal: %s", getSignalName(signalID));
}

AdjacencyManager::SortedDistanceList AdjacencyManager::getNodesInRangeSorted() {
    auto cmp = [this](cModule* const& left, cModule* const& right) { return this->getDistance(left) > this->getDistance(right); };
//    std::priority_queue<cModule*, std::vector<cModule*>, decltype(cmp)> nodes(cmp);
    std::vector<cModule*> nodes;
    // Loop through all modules and find those that satisfy the criteria
    for (int modId = 0; modId <= getSimulation()->getLastComponentId(); modId++) {
        cModule *module = getSimulation()->getModule(modId);
        if (module && isWirelessNodeAndInRange(module)) {
            nodes.push_back(module);
        }
    }
    std::sort(nodes.begin(), nodes.end(), cmp);
    return nodes;
//    for (EV_WARN << "Sorted" << ": \t"; !nodes.empty(); nodes.pop_back())
//        EV_WARN << nodes.back() << ' ';
//    EV_WARN << endl;
}
//bool AdjacencyManager::cmp(cModule* left, cModule* right) {
//    return getDistance(left) > getDistance(right);
//}

bool AdjacencyManager::isWirelessNodeAndInRange(cModule *module) {
    // filter out this.host if "prototype.LocNodes.MobileNode" is also allowed
    const std::vector<std::string>& v {(std::string)"prototype.LocNodes.NeighbourNode"};
    if (!contains(v, module->getNedTypeName()))
        return false;

    ASSERT(true); // ASSERT is Node && has wireless interface
    auto otherIft = check_and_cast<IInterfaceTable *>(module->getSubmodule("interfaceTable"));
    ASSERT(otherIft->findInterfaceByName("wlan0") != nullptr);
    auto otherRadio = check_and_cast<physicallayer::IRadio *>(otherIft->findInterfaceByName("wlan0")->getSubmodule("radio"));
    auto otherRange = otherRadio->getTransmitter()->getMaxCommunicationRange();

    return getDistance(module) < otherRange;
}

inet::units::values::m AdjacencyManager::getDistance(cModule *otherNode) {
    auto mobility = check_and_cast<physicallayer::IRadio *>(ift->findInterfaceByName("wlan0")->getSubmodule("radio"))->getAntenna()->getMobility();
    auto selfPos = mobility->getCurrentPosition();

    auto otherIft = check_and_cast<IInterfaceTable *>(otherNode->getSubmodule("interfaceTable"));
    ASSERT(otherIft->findInterfaceByName("wlan0") != nullptr);
    auto otherRadio = check_and_cast<physicallayer::IRadio *>(otherIft->findInterfaceByName("wlan0")->getSubmodule("radio"));
    auto otherPos = otherRadio->getAntenna()->getMobility()->getCurrentPosition();

    return (units::values::m) selfPos.distance(otherPos);
}

AdjacencyManager::AdjacencyManager() {
    // TODO Auto-generated constructor stub

}

AdjacencyManager::~AdjacencyManager() {
    if (host != nullptr && host->isSubscribed(IMobility::mobilityStateChangedSignal, this))
            host->unsubscribe(IMobility::mobilityStateChangedSignal, this);
}

