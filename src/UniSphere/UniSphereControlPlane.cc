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
const simsignal_t UniSphereControlPlane::newNeighbourConnectedSignal = cComponent::registerSignal("newNeighConnected");

UniSphereControlPlane::UniSphereControlPlane() {
    // TODO Auto-generated constructor stub
}

UniSphereControlPlane::~UniSphereControlPlane() {
    cancelAndDelete(selfMsg);
    delete selfAnnounce;
    if (host != nullptr && host->isSubscribed(newNeighbourConnectedSignal, this))
        host->unsubscribe(newNeighbourConnectedSignal, this);
//    if (irtOld) {
//        auto *mod = check_and_cast<cModule*>(irtOld.get());
//        mod->deleteModule();
//    }
}

void UniSphereControlPlane::initialize(int stage) {
    RoutingProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        selfMsg = new cMessage("announceTimer");
        locator = UniSphereLocator(); // empty Locator

        // get the routing table to update and subscribe it to the blackboard
        irt.reference(this, "routingTableModule", true);
        ift.reference(this, "interfaceTableModule", true);
        peerIn = gate("networkLayerIn");
        peerOut = gate("networkLayerOut");
        forwarding = par("forwarding");

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
//        cModuleType *moduleType = cModuleType::get("inet.networklayer.nexthop.NextHopRoutingTable");
//        cModule *mod = moduleType->createScheduleInit("irtOld", host);
//        irtOld = opp_component_ptr<IRoutingTable>(check_and_cast<IRoutingTable*>(mod));

        // subscribe
        host->subscribe(newNeighbourConnectedSignal, this);

        // init ourselves
        selfAnnounce->setDestination(getHostID(host));
        selfAnnounce->setLandmark(false); // see networkSizeEstimateChanged
        // grant access to this host. Only 1 selfAnnounce is used for consistency, even if the RT could be logically split (!= DTPMs)
        selfAnnounce->setRoutingTable(check_and_cast<NextHopRoutingTable *>(irt.get()));
        WATCH(selfAnnounce);
        WATCH_PTR(selfAnnounce);
        WATCH(locator);

        // Compute whether we should become a landmark or not
        networkSizeEstimateChanged(getNetworkSize());
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
//        selfAnnounce->seqno++; //FIXME: seqno not updated in U-Sphere???
        announceOurselves(peer);
    }
    scheduleAfter(interval_announce, selfMsg);
}
void UniSphereControlPlane::announceOurselves(cModule* peer) {
    auto payload = selfAnnounce->exportEntry();
    ASSERT(payload->getOrigin() == payload->getForward_path().top());
    sendToNeighbour(getHostID(peer), payload);

    // Send full routing table to neighbor
    fullUpdate(getHostID(peer));
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
    // in U-Sphere, a route can be imported, even if it's not better TODO
    bool isImported = importRoute(route);
    if (!isImported)
        delete route;

    /* If import results in better route, export this route to all neighbours */
    //TODO
    if (isImported) {

        for (auto peer: getConnectedNodes(irt)) {
//            auto payload = staticPtrCast<PathAnnounce>(ctrlMessage->dupShared()); //FIXME dupShared()??
//            payload->setChunkLength(payload->getChunkLength()+B(1)); // FIXME
//            payload->appendForward_path(getHostID(host)); // add ourselves to forward path
            sendToNeighbourProtected(getHostID(peer), route);
        }
    }

    delete pkt;
}

/* @return bool is the new route imported?*/
bool UniSphereControlPlane::importRoute(UniSphereRoute *newRoute) {
    L3Address origin = newRoute->getDestinationAsGeneric();
    if (origin == getHostID(host)) {
        EV_WARN << "Got a message about myself (=loop), discarding" << newRoute << endl;
        return false;
    }

    bool landmarkChangedType = false;

    // check if route update has same dest & vport/neighbour
    // We can put multiple routes in the Table, lowest metric will be chosen
    // But the question is: do we need/want this? U-Sphere implementation seems to think so, but unclear when actually used
    UniSphereRoute *oldRoute = check_and_cast_nullable<UniSphereRoute*>(irt->findBestMatchingRoute(origin));
    if (oldRoute != nullptr && oldRoute->getNextHopAsGeneric() == newRoute->getNextHopAsGeneric()) {
        ASSERT(oldRoute->getDestinationAsGeneric() == origin);
        // Ignore import when the existing entry is ... (U-Sphere, not applicable here)
        //return false; // U-Sphere

        // Update certain attributes of the routing entry
        ASSERT2(!(oldRoute->isLandmark() && !newRoute->isLandmark()), "Landmark status has dropped, should not be possible when the network size doesn't change");
        if (oldRoute->isLandmark() != newRoute->isLandmark())
          landmarkChangedType = true;

        // TODO: In some cases modification can cause the entry to be retracted; for example
        // when it is no longer a landmark and it doesn't fall into the (extended) vicinity


        // EITHER: it was active
        // OR it is an uninitialized neighbour
        // IF NOT: then we have non-active routes in our system being updated...
        ASSERT(oldRoute->active || isUnitializedNeighbour(oldRoute));
        newRoute->active = oldRoute->active;
        newRoute->vicinity = oldRoute->vicinity;
        if (isUnitializedNeighbour(oldRoute)) {
            //FIXME: might violate some primitives, because the procedure for route insertion is not respected.
            //SOLUTION: let even the AdjManager import routes using this procedure?
            // Even without some needed information (like landmark status?)
            newRoute->active = true;
            newRoute->vicinity = true;
        }
        irt->deleteRoute(oldRoute);
        oldRoute = nullptr; // importNewRoute WILL succeed
        //TODO Might change bestRoute & returns true in U-Sphere
    }
    else {
        // An entry should be inserted if it represents a landmark or if it falls into the vicinity
        // or the extended vicinity (based on sloppy group membership) of the current node
        bool isVicinity = false;
        CurrentVicinity vicinity = getCurrentVicinity();

        if (vicinity.size >= getMaximumVicinitySize()) {
            if (vicinity.maxHopEntry->getMetric() == 0) {
                EV_WARN << "We have a local star-topology, where there are more neighbours than places in the RT." << endl;
                if (newRoute->getMetric() == 0) {
                    EV_WARN << "Even some neighbour entries are not accepted." << endl;
                    ASSERT(false); // should not be possible to be both neighbour & new, due to our approach in OMNeT
                }
            }
            if (vicinity.maxHopEntry->getMetric() > newRoute->getMetric()) {
                // In vicinity, but we might need to retract an entry if it isn't in any bucket
                isVicinity = true;
                if(vicinity.maxHopEntry->isLandmark()) {
                    vicinity.maxHopEntry->vicinity = false;
                } else {
                    retract(vicinity.maxHopEntry->getDestinationAsGeneric());
                }
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

        newRoute->vicinity = isVicinity;
        //irt->addRoute(newRoute); // do this here OR in keepBestRoute()
    }
    // TODO: very different in semantics from selectBestRoute
    // (sBR has triggers in itself AND keeps old entries, just deactivates them)
    bool importedNewRoute = keepBestRoute(newRoute, oldRoute);
    if (true && landmarkChangedType)
        // Landmark type of the currently active route has changed
        /*TODO?*/;

    // Determine whether local address has changed
    if (newRoute->isLandmark() || landmarkChangedType)
        selectLocalAddress();

    return importedNewRoute;
}

bool UniSphereControlPlane::keepBestRoute(UniSphereRoute* newRoute, UniSphereRoute* oldRoute) {
    // if there is no oldRoute OR if the newRoute is better, add newRoute
    if (!oldRoute || newRoute->getMetric() < oldRoute->getMetric()) {
        newRoute->active = true;
        irt->addRoute(newRoute);
        if (oldRoute)
            irt->deleteRoute(oldRoute);
        return true; // imported newRoute
    }
    else
        return false; // kept oldRoute
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

void UniSphereControlPlane::fullUpdate(L3Address neighbour) {
    for (int i = 0; i < irt->getNumRoutes(); ++i) {
        auto *e = check_and_cast<UniSphereRoute*>(irt->getRoute(i));
        if (e->active) {
            sendToNeighbourProtected(neighbour, e);
        }
    }
}

void UniSphereControlPlane::receiveSignal(cComponent *source, simsignal_t signalID, cObject *neigh, cObject *details) {
    Enter_Method("%s", cComponent::getSignalName(signalID));
    if (signalID == newNeighbourConnectedSignal) {
        announceOurselves(check_and_cast<cModule*>(neigh));
    }
    //case /*TODO: peerRemoved*/:
    /*case routingEntryExpired?*/
    else
        throw cRuntimeError("Unexpected signal: %s", getSignalName(signalID));
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
    // ASSUMPTION: all entries in RT are active. isVicinity is checked (and no extendedVicinity is used) TODO
    CurrentVicinity vicinity {};

    ASSERT(vicinity.size == 0);
    ASSERT(vicinity.maxHopEntry == nullptr);
    for (int i = 0; i < irt->getNumRoutes(); ++i) {
        UniSphereRoute *e = check_and_cast<UniSphereRoute*>(irt->getRoute(i));
        if (e->vicinity) {
            vicinity.size++;
            if (vicinity.maxHopEntry == nullptr || e->getMetric() > vicinity.maxHopEntry->getMetric())
                vicinity.maxHopEntry = e;
        }
    }
    ASSERT(vicinity.size == 0 || vicinity.maxHopEntry); //->if condition wrongly implemented

    return vicinity;
}

bool UniSphereControlPlane::sendToNeighbourProtected(L3Address neighbour, UniSphereRoute* entry) {
    auto payload = entry->exportEntry();
    // Retrieve ID(vport in U-Sphere) for given peer
    // & don't send if the entry is originally coming from that neighbour
    if (neighbour == entry->getNextHopAsGeneric())
        return false;
    // don't send entries when forwarding disabled (for MNs, most simple implementation of multiple end-point DTPMs)
    if (!forwarding)
        return false;

    sendToNeighbour(neighbour, payload);
    return true;
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

void UniSphereControlPlane::networkSizeEstimateChanged(int size) {
    // Re-evaluate network size and check if we should alter our landmark status
    // U-Sphere: std::generate_canonical<double, 32>(m_manager.context().basicRng());
    double x = dblrand();
    double n = static_cast<double>(size);

    // TODO: Only flip landmark status if size has changed by at least a factor 2
    if (x < std::sqrt(std::log(n) / n)) {
      EV_WARN << "Becoming a LANDMARK." << endl;
      selfAnnounce->setLandmark(true);
    }
}

bool UniSphereControlPlane::selectLocalAddress() {
    auto selfID = getHostID(host);
    if (selfAnnounce->isLandmark()) {
        if (locator.ID != selfID ||
                locator.path.size() > 0) {
            locator.ID = selfID;
            locator.path = RoutingPath();
        // TODO signal Loc change?
//            emit(newLocAssignedSignal, numLocUpdates, ie);
        return true;
      }
      return false;
    }
    // If not self landmark
    UniSphereRoute* bestLandmark = nullptr;
    for (int i = 0; i < irt->getNumRoutes(); ++i) {
        UniSphereRoute* re = check_and_cast<UniSphereRoute*>(irt->getRoute(i));
        if (re->isLandmark()
                && (bestLandmark == nullptr || re->getMetric() < bestLandmark->getMetric())) {
            bestLandmark = re;
        }
    }
    ASSERT(bestLandmark); // there is no known landmark??
    if (locator.ID == selfID
            && locator.path.size() > 0
            && locator.path.top() == bestLandmark->getDestinationAsGeneric()) // FIXME: correct way?
        return false;
    else {
        locator.ID = getHostID(host);
        locator.path = RoutingPath();
        // reverse_copy
        auto temp = bestLandmark->forwardPath;
        while(!temp.empty()) {
            locator.path.push(temp.top());
            temp.pop();
        }
        //TODO: emit();
        return true;
    }
}

void UniSphereControlPlane::handleStartOperation(LifecycleOperation *operation) {
    simtime_t start = simTime(); // std::max(startTime
    scheduleAt(start, selfMsg);
}
