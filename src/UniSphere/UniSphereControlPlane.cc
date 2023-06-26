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

#include "UniSphereControlPlane.h"
#include "PathAnnounce_m.h"
#include "../util.h"

#include "inet/common/ProtocolTag_m.h"
#include "inet/common/ProtocolGroup.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/HopLimitTag_m.h"
#include "inet/networklayer/nexthop/NextHopRoutingTable.h"

using namespace inet; // more OK to use in .cc
Define_Module(UniSphereControlPlane);

const inet::Protocol *UniSphereControlPlane::unisphere = new Protocol("unisphere", "U-Sphere");

UniSphereControlPlane::UniSphereControlPlane() {
    // TODO Auto-generated constructor stub

}

UniSphereControlPlane::~UniSphereControlPlane() {
    cancelAndDelete(selfMsg);
    delete selfAnnounce;
}

void UniSphereControlPlane::initialize(int stage) {
    RoutingProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        selfMsg = new cMessage("announceTimer");

        // get the routing table to update and subscribe it to the blackboard
        irt.reference(this, "routingTableModule", true);
        ift.reference(this, "interfaceTableModule", true);
        peerIn = gate("networkLayerIn");
        peerOut = gate("networkLayerOut");

        if (!ProtocolGroup::getIpProtocolGroup()->findProtocol(protocolId)) { // one-shot execution
            ProtocolGroup::getIpProtocolGroup()->addProtocol(protocolId, unisphere);
        }
    }
    else if (stage == INITSTAGE_ROUTING_PROTOCOLS) {
        registerProtocol(*unisphere, gate("networkLayerOut"), gate("networkLayerIn"));
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        // get the hostname
        host = getContainingNode(this);
        selfAnnounce = new UniSphereRoute();
        selfAnnounce->setDestination(getHostID(host));
        selfAnnounce->setLandmark(false); // FIXME: adjust to U-Sphere specs
        selfAnnounce->setRoutingTable(check_and_cast<NextHopRoutingTable *>(irt.get())); // grant access to this host
        // Other default values??
    }
}

void UniSphereControlPlane::handleMessageWhenUp(cMessage *msg) {
    if (msg->isSelfMessage()) {
        // handle self message (announce timer)
        announceOurselves();
    }
    else if (msg->arrivedOn("networkLayerIn")) {
        // handle incoming message
        Packet *pkt = check_and_cast<Packet *>(msg);
        auto protocol = pkt->getTag<PacketProtocolTag>()->getProtocol();
        if (protocol == unisphere) {
            EV_WARN << "Received unisphere packet from network: " << msg->getName() << " (" << msg->getClassName() << ")" << endl;
            processPacket(pkt);
        }
        else
            throw cRuntimeError("U-SphereControlPlane: received unknown packet '%s (%s)' from the network layer.", msg->getName(), msg->getClassName());
    }
    else
        throw cRuntimeError("Unknown message origin.");
}

void UniSphereControlPlane::announceOurselves() {
    // Announce ourselves to all neighbours and send them routing updates
    for (auto peer: getConnectedNodes(irt)) {
//        auto payload = makeShared<PathAnnounce>();
//        selfAnnounce->seqno++; //FIXME: seqno not updated in U-Sphere???
//        payload->setChunkLength(B(10)); // FIXME
//        payload->setLandmark(false);
//        payload->setSeqno(0);
//        payload->setOrigin(getHostID(host));
        // add ourselves to forward path
//        payload->appendForward_path(getHostID(host));
        auto payload = selfAnnounce->exportEntry();
        ASSERT(payload->getOrigin() == payload->getForward_path().back());
        sendToNeighbour(getHostID(peer), payload);

        // Send full routing table to neighbor
        //fullUpdate(getHostID(peer));
    }
    scheduleAfter(interval_announce, selfMsg);
}

void UniSphereControlPlane::processPacket(Packet *pkt) {
    auto ctrlMessage = dynamicPtrCast<const PathAnnounce>(pkt->popAtFront());
    EV_WARN << "Received message" << ctrlMessage << endl;
    /* social/compact_router.cpp:663 */
    // in U-Sphere, this is actually an aggregation of announcements -\_(")_/-

    /* Prepare routing entry */
    UniSphereRoute *route = new UniSphereRoute(ctrlMessage);
    route->setInterface(getSourceInterfaceFrom(pkt)); //TODO: does this mean something for our thesissimulation?? Is it a restriction??

    /* attempt to import if better route */
    bool isImported = importRoute(route);
    if (!isImported)
        delete route;

    /* If import results in better route, export this route to all neighbours */
    //TODO

    delete pkt;
}

bool UniSphereControlPlane::importRoute(UniSphereRoute *newRoute) {
    L3Address origin = newRoute->getDestinationAsGeneric();
    if (origin == getHostID(host))
        return false;

    // check if route update has same dest & vport/neighbour
    // We can put multiple routes in the Table, lowest metric will be chosen
    // But the question is: do we need/want this? U-Sphere implementation seems to think so, but unclear when actually used
    NextHopRoute *oldRoute = check_and_cast<NextHopRoute*>(irt->findBestMatchingRoute(origin));
    if (oldRoute != nullptr && oldRoute->getNextHopAsGeneric() == newRoute->getNextHopAsGeneric()) {
        ASSERT(oldRoute->getDestinationAsGeneric() == origin);
        // Ignore import when the existing entry is ... (not applicable here)
        // (return false) ->
        // Update certain attributes of the routing entry
        oldRoute->setMetric(newRoute->getMetric());
        //TODO Might change bestRoute & returns true in U-Sphere
        return false; //FIXME
    }
    else if (oldRoute != nullptr) {
        // has different NextHop, what to do?
        return false; //FIXME
    }
    else {
        // An entry should be inserted if it represents a landmark or if it falls into the vicinity
        // or the extended vicinity (based on sloppy group membership) of the current node
        bool isVicinity = false;
        CurrentVicinity vicinity = getCurrentVicinity();

        if (vicinity.size >= getMaximumVicinitySize()) {
            if (vicinity.maxHopEntry->getMetric() > newRoute->getMetric()) {
                // In vicinity, but we might need to retract an entry if it isn't in any bucket
                isVicinity = true;
                //FIXME!!!
//                vicinity.maxHopEntry->setLandmark();
            }
            // Else: not part of vicinity, but might be part of sloppy group
            // Unimplemented
        } else {
            // Inside vicinity as it is not yet full
            isVicinity = true;
        }

        if (!isVicinity && !newRoute->isLandmark())
            return false;
        irt->addRoute(newRoute);
        return true;
    }
    //FIXME mem-leaks?
}

bool UniSphereControlPlane::retract(L3Address dest) {
    int numDeleted = 0;
    while (IRoute* e = irt->findBestMatchingRoute(dest)) {
        ASSERT(e->getDestinationAsGeneric() == dest);
        irt->deleteRoute(e);
        numDeleted++;
    }
    // TODO: send retract entry signals
    return numDeleted > 0;
}

size_t UniSphereControlPlane::getMaximumVicinitySize() const
{
    // TODO: This is probably not the best way (int -> double -> sqrt -> int)
    int networkSize = getNetworkSize();
    double n = static_cast<double>(networkSize);
    size_t maxVicinitySize = static_cast<size_t>(std::sqrt(n * std::log(n)));
    EV_WARN << "Vicinity estimate: " << maxVicinitySize << endl;
    return maxVicinitySize;
}

UniSphereControlPlane::CurrentVicinity UniSphereControlPlane::getCurrentVicinity() const {
    // get entries which are: active && inVicinity (&& !extendedVicinity ?)
    // ASSUMPTION: all entries in RT are active && inVicinity (and no extendedVicinity is used)
    CurrentVicinity vicinity;
    vicinity.size = irt->getNumRoutes();

    for (int i = 0; i < vicinity.size; ++i) {
        IRoute *e = irt->getRoute(i);
        if (vicinity.maxHopEntry != nullptr && e->getMetric() > vicinity.maxHopEntry->getMetric())
            vicinity.maxHopEntry = check_and_cast<NextHopRoute*>(e);
    }

    return vicinity;
}

void UniSphereControlPlane::sendToNeighbour(L3Address neighbour, inet::Ptr<PathAnnounce> payload) {
    // could buffer before sending (see U-Sphere, CompactRouterPrivate::ribExportQueueAnnounce)
    short ttl = 1;
    Packet *pkt = new Packet("PathAnnounce", payload);
    // TODO - see routing/pim/modes/PimSM.cc/sendToIP
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(unisphere);
    pkt->addTagIfAbsent<DispatchProtocolInd>()->setProtocol(unisphere);
    pkt->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::nextHopForwarding);
    pkt->addTagIfAbsent<L3AddressReq>()->setDestAddress(neighbour);
    pkt->addTagIfAbsent<HopLimitReq>()->setHopLimit(ttl);
    send(pkt, peerOut);
}

void UniSphereControlPlane::handleStartOperation(LifecycleOperation *operation) {
    simtime_t start = simTime(); // std::max(startTime
    scheduleAt(start, selfMsg);
}
