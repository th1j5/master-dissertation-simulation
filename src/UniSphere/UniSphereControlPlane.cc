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
#include "util.h"
#include "AdjacencyManager/AdjacencyManager.h"

#include "inet/common/ProtocolTag_m.h"
#include "inet/common/ProtocolGroup.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/HopLimitTag_m.h"
#include "inet/networklayer/nexthop/NextHopRoutingTable.h"

using namespace inet; // more OK to use in .cc
Define_Module(UniSphereControlPlane);

const simsignal_t UniSphereControlPlane::isLandmarkSignal = cComponent::registerSignal("isLandmark");

UniSphereControlPlane::UniSphereControlPlane() {}

UniSphereControlPlane::~UniSphereControlPlane() {
    cancelAndDelete(selfMsg);
    delete selfAnnounce;
    if (host != nullptr && host->isSubscribed(AdjacencyManager::newNeighbourConnectedSignal, this))
        host->unsubscribe(AdjacencyManager::newNeighbourConnectedSignal, this);
    if (host != nullptr && host->isSubscribed(AdjacencyManager::oldNeighbourDisconnectedSignal, this))
        host->unsubscribe(AdjacencyManager::oldNeighbourDisconnectedSignal, this);
//    if (irtOld) {
//        auto *mod = check_and_cast<cModule*>(irtOld.get());
//        mod->deleteModule();
//    }
}

void UniSphereControlPlane::initialize(int stage) {
    RoutingProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        ASSERT(strcmp(par("locChangingStrategy"), "end2end") == 0);

        selfMsg = new cMessage("announceTimer");
        locator = UniSphereLocator(); // empty Locator

        // get the routing table to update and subscribe it to the blackboard
        irt.reference(this, "routingTableModule", true);
        ift.reference(this, "interfaceTableModule", true);
        peerIn = gate("networkLayerIn");
        peerOut = gate("networkLayerOut");
        forwarding = par("forwarding");

        unisphere = ProtocolGroup::getIpProtocolGroup()->findProtocol(protocolId);
        if (!unisphere) { // one-shot execution
            unisphere = new Protocol("unisphere", "U-Sphere");
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
        host->subscribe(AdjacencyManager::newNeighbourConnectedSignal, this);
        host->subscribe(AdjacencyManager::oldNeighbourDisconnectedSignal, this);

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
    for (auto peer: getConnectedNeigh(irt)) {
//        selfAnnounce->seqno++; //FIXME: seqno not updated in U-Sphere???
        announceOurselves(peer);
    }
    scheduleAfter(interval_announce, selfMsg);
}
void UniSphereControlPlane::announceOurselves(cModule* peer) {
    auto payload = selfAnnounce->exportEntry();
    ASSERT(payload->getOrigin() == payload->getForward_path().top());
    sendToNeighbour(getHostID(peer), payload);

    // Send full routing table to neighbor (if not MN)
    fullUpdate(getHostID(peer));
}

void UniSphereControlPlane::processPacket(Packet *pkt) {
    if (auto ctrlMessage = dynamicPtrCast<const PathAnnounce>(pkt->peekAtFront())) {
        processPathAnnounce(pkt);
    }
    else if (auto ctrlMessage = dynamicPtrCast<const PathRetract>(pkt->peekAtFront())) {
        processPathRetract(ctrlMessage);
    }
    else
        throw cRuntimeError("Unrecognized control packet");
    delete pkt;
}
void UniSphereControlPlane::processPathRetract(Ptr<const PathRetract> ctrlMessage) {
    L3Address getSendingNeighbour = ctrlMessage->getOriginNeigh();
    retract(getSendingNeighbour, ctrlMessage->getDestination());
}
void UniSphereControlPlane::processPathAnnounce(Packet *pkt) {
    auto ctrlMessage = dynamicPtrCast<const PathAnnounce>(pkt->popAtFront());
    EV_WARN << "Received message" << ctrlMessage << endl;
    /* social/compact_router.cpp:663 */
    // in U-Sphere, this is actually an aggregation of announcements -\_(")_/-

    /* Prepare routing entry */
    UniSphereRoute *route = new UniSphereRoute(ctrlMessage);
    route->setInterface(getSourceInterfaceFrom(pkt)); //TODO: does this mean something for our thesissimulation?? Is it a restriction??
    if (!forwarding)
        route->RIB = numNewNeighConnected;

    /* attempt to import if better route */
    // in U-Sphere, a route can be imported, even if it's not better TODO
    bool isImported = importRoute(route);
    if (!isImported)
        delete route;

    /* If import results in better route, export this route to all neighbours */
    //TODO
    if (isImported) {

        for (auto peer: getConnectedNeigh(irt)) {
//            auto payload = staticPtrCast<PathAnnounce>(ctrlMessage->dupShared()); //FIXME dupShared()??
//            payload->setChunkLength(payload->getChunkLength()+B(1)); // FIXME
//            payload->appendForward_path(getHostID(host)); // add ourselves to forward path
            sendToNeighbourProtected(getHostID(peer), route);
        }
    }
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
        if(!(oldRoute->active || isUnitializedNeighbour(oldRoute)))
            throw cRuntimeError("non-active routes are updated");
        newRoute->active = oldRoute->active;
        newRoute->vicinity = oldRoute->vicinity;
        if (isUnitializedNeighbour(oldRoute)) {
            //FIXME: might violate some primitives, because the procedure for route insertion is not respected.
            //SOLUTION: let even the AdjManager import routes using this procedure?
            // Even without some needed information (like landmark status?)
            newRoute->active = true;
            newRoute->vicinity = true;
        }
        irt->deleteRoute(oldRoute); // oldRoute no longer valid
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
                    throw cRuntimeError("should not be possible to be both neighbour & new, due to our approach in OMNeT");
                }
            }
            if (vicinity.maxHopEntry->getMetric() > newRoute->getMetric()) {
                // In vicinity, but we might need to retract an entry if it isn't in any bucket
                isVicinity = true;
                if(vicinity.maxHopEntry->isLandmark()) {
                    vicinity.maxHopEntry->vicinity = false;
                } else {
                    retract(vicinity.maxHopEntry->getDestinationAsGeneric());
                    // oldRoute no longer valid
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
    bool importedNewRoute = keepBestRoute(newRoute);
    if (true && landmarkChangedType)
        // Landmark type of the currently active route has changed
        /*TODO?*/;

    // Determine whether local address has changed
    if (newRoute->isLandmark() || landmarkChangedType) {
        if(selectLocalAddress()) {
            // TODO!! removeOldLoc
            if (firstLocUpdateAfterTopologyChange) {
                numLocUpdates++;
                firstLocUpdateAfterTopologyChange = false;
            }
            emit(newLocAssignedSignal, numLocUpdates, new Locator(locator));
        }
    }

    return importedNewRoute;
}

bool UniSphereControlPlane::keepBestRoute(UniSphereRoute* newRoute) {
    // TODO: look at RIB information?
    // if there is no oldRoute OR if the newRoute is better, add newRoute
    L3Address origin = newRoute->getDestinationAsGeneric();
    auto *oldRoute = check_and_cast_nullable<UniSphereRoute*>(irt->findBestMatchingRoute(origin));
    if (oldRoute == nullptr || newRoute->getMetric() < oldRoute->getMetric()) {
        if (oldRoute) {
            irt->deleteRoute(oldRoute);
            // only ever keep 1 route to a destination... (!= U-Sphere)
            ASSERT(nullptr == irt->findBestMatchingRoute(origin));
        }
        newRoute->active = true;
        irt->addRoute(newRoute);
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
bool UniSphereControlPlane::retract(L3Address neighSource, L3Address dest) {
    // irt mutates in the loop, we know it's a vector, so we cheat with this information...
    for (int i=irt->getNumRoutes()-1; i>=0; i--) {
        auto *e = check_and_cast<UniSphereRoute*>(irt->getRoute(i));
        if (neighSource == e->getNextHopAsGeneric()
                && (dest.isUnspecified() || dest == e->getDestinationAsGeneric())) {
            // U-Sphere: determine new active route, only send if changed
            if (e->active && true)
                sendRetractToAllNeighbours(e);
            irt->deleteRoute(e);
            //TODO: U-Sphere
            //select better route (is hard with our scheme, because we don't keep any non-active routes)
            // U-Sphere is an extension of DVR, where multiple advertisements for the same dest are kept
            // Also a bit like Babel (there, the feasibility condition decides, what we have here is starvation)
        }
    }
    return false;
}
// ribRetractEntry() in U-Sphere
void UniSphereControlPlane::sendRetractToAllNeighbours(UniSphereRoute* route) {
    //Protected
    if (!forwarding)
        return;
    // Announce ourselves to all neighbours and send them routing updates
    for (auto peer: getConnectedNeigh(irt)) {
        auto peerID = getHostID(peer);
//        selfAnnounce->seqno++; //FIXME: seqno not updated in U-Sphere???
        auto payload = makeShared<PathRetract>();
        payload->setDestination(route->getDestinationAsGeneric());
        payload->setOriginNeigh(getHostID(host));
        payload->setChunkLength(B(1));
        //Protected
        if (peerID == route->getNextHopAsGeneric())
            continue;

        sendToNeighbour(peerID, payload);
    }
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
    if (signalID == AdjacencyManager::newNeighbourConnectedSignal) {
        announceOurselves(check_and_cast<cModule*>(neigh));
        numNewNeighConnected++;
        firstLocUpdateAfterTopologyChange = true;
        if (!forwarding) {
            // Update RIB separation IF I'm a MN - done by numNewNeighConnected
        }
    }
    /*case peerRemoved*/
    else if (signalID == AdjacencyManager::oldNeighbourDisconnectedSignal) {
        L3Address peerID = getHostID(check_and_cast<cModule*>(neigh));
        retract(peerID, L3Address());
        /* since new locator is proactively chosen, we can assume this
         * means that the old Locator isn't reachable anymore when a peer disconnects (old neigh)
         * packets could however by chance still reach the node, because the ID can be enough to route. */
        emit(oldLocRemovedSignal, numLocUpdates);
    }
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
void UniSphereControlPlane::sendToNeighbour(L3Address neighbour, inet::Ptr<inet::FieldsChunk> payload) {
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
    emit(isLandmarkSignal, selfAnnounce->isLandmark());
}

bool UniSphereControlPlane::selectLocalAddress() {
    /**
     * only select from the latest DTPM or RIB
     * This means that we proactively choose the landmark to change (disable forwarding if you don't want this)
     */
    auto selfID = getHostID(host);
    if (selfAnnounce->isLandmark()) {
        if (locator.ID == selfID && locator.path.size() == 0)
            return false;
        locator.ID = selfID;
        locator.path = RoutingPath();
    }
    else {
        // If not self landmark
        UniSphereRoute* bestL = nullptr; // Best landmark
        for (int i = 0; i < irt->getNumRoutes(); ++i) {
            UniSphereRoute* re = check_and_cast<UniSphereRoute*>(irt->getRoute(i));
            /* Only select candidate if:
             * - Landmark && ( no candidate yet || newer RIB || better metric && same RIB )
             * There is no way the landmark will be from the old RIB (landmarks everywhere reachable)
             */
            if (re->isLandmark() &&
                    (bestL == nullptr
                     || re->RIB > bestL->RIB
                     || (re->getMetric() < bestL->getMetric() && re->RIB == bestL->RIB)))
            {
                bestL = re;
            }
        }
        if (!bestL)
            throw cRuntimeError("there is no known landmark??");
        if (locator.ID == selfID
                && locator.path.size() > 0
                && locator.path.top() == bestL->getDestinationAsGeneric()) // FIXME: correct way?
            return false;
        locator.ID = getHostID(host);
        locator.path = RoutingPath();
        // reverse_copy
        auto temp = bestL->forwardPath;
        while(!temp.empty()) {
            locator.path.push(temp.top());
            temp.pop();
        }
    }
    return true;
}

void UniSphereControlPlane::handleStartOperation(LifecycleOperation *operation) {
    simtime_t start = simTime(); // std::max(startTime
    scheduleAt(start, selfMsg);
}
