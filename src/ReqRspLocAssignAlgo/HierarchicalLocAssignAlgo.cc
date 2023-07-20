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

#include "HierarchicalLocAssignAlgo.h"

#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/ipv4/Ipv4Route.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/networklayer/ipv4/Ipv4RoutingTable.h"
#include "inet/networklayer/common/HopLimitTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/common/IProtocolRegistrationListener.h"

#include "util.h"

using namespace inet; // more OK to use in .cc
Define_Module(HierarchicalLocAssignAlgo);
// oldLocUnr signal???
// newlocAssigned??
// neighLocSend??

/**
 * routerId should remain stable in the IPv4 world (first IPv4 is chosen as ID)
 * But in very limited cases multiple MNs could get the same ID (if they get both the same IPv4 at different times for the first time)
 * Or it could go horribly wrong... because MN ifaces aren't yet assigned at initialization
 */
HierarchicalLocAssignAlgo::HierarchicalLocAssignAlgo() {
    // TODO Auto-generated constructor stub

}

HierarchicalLocAssignAlgo::~HierarchicalLocAssignAlgo() {
    // TODO Auto-generated destructor stub
    leased.clear();
    if (host != nullptr && host->isSubscribed(AdjacencyManager::newNeighbourConnectedSignal, this))
        host->unsubscribe(AdjacencyManager::newNeighbourConnectedSignal, this);
}
namespace {
static bool routeMatches(const Ipv4Route *entry, const Ipv4Address& target, const Ipv4Address& nmask,
        const Ipv4Address& gw, int metric, const char *dev)
{
    if (!target.equals(entry->getDestination()))
        return false;
    if (!nmask.equals(entry->getNetmask()))
        return false;
    if (!gw.equals(entry->getGateway()))
        return false;
    if (metric && metric != entry->getMetric())
        return false;
    if (strcmp(dev, entry->getInterfaceName()))
        return false;

    return true;
}
}

void HierarchicalLocAssignAlgo::initialize(int stage) {
    RoutingProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        // get the routing table to update and subscribe it to the blackboard
        irt.reference(this, "routingTableModule", true);
        ift.reference(this, "interfaceTableModule", true);
        // client
//        arp.reference(this, "arpModule", true);
        // server
//        ttr.reference(this, "transientTriangularRoutingModule", false); // false, when tested

        peerIn = gate("networkLayerIn");
        peerOut = gate("networkLayerOut");
        server = par("server");
        client = par("client");
//        forwarding = par("forwarding");

        hierLocAssignAlgo = ProtocolGroup::getIpProtocolGroup()->findProtocol(protocolId);
        if (!hierLocAssignAlgo) { // one-shot execution
            hierLocAssignAlgo = new Protocol("hierLocAssignAlgo", "Hierarchical Locator Assignment Algorithm");
            ProtocolGroup::getIpProtocolGroup()->addProtocol(protocolId, hierLocAssignAlgo);
        }

        if (server) {
            WATCH_MAP(leased);
        }
    }
    else if (stage == INITSTAGE_ROUTING_PROTOCOLS) {
        registerProtocol(*hierLocAssignAlgo, gate("networkLayerOut"), gate("networkLayerIn"));
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        // get the hostname
        host = getContainingNode(this);
        // subscribe
        host->subscribe(AdjacencyManager::newNeighbourConnectedSignal, this);
        host->subscribe(AdjacencyManager::oldNeighbourDisconnectedSignal, this);

        // fix routerID to be usable
        auto * ipv4RT = check_and_cast<Ipv4RoutingTable*>(irt.get());
        Ipv4Address myID = Ipv4Address(uint32_t(host->getId()));
        ipv4RT->setRouterId(myID);
        ASSERT(!irt->getRouterIdAsGeneric().isUnspecified());

        // other signals? like: interfaceDeletedSignal, l2AssociatedSignal?
    }
}

void HierarchicalLocAssignAlgo::handleMessageWhenUp(cMessage *msg) {
    ASSERT2(msg->arrivedOn("networkLayerIn"), "Unknown message origin.");
    // handle incoming message
    Packet *pkt = check_and_cast<Packet *>(msg);
    auto protocol = pkt->getTag<PacketProtocolTag>()->getProtocol();
    if (protocol != hierLocAssignAlgo)
        throw cRuntimeError("received unknown packet '%s (%s)' from the network layer.", msg->getName(), msg->getClassName());
    EV_WARN << "Received hierLocAssign packet from network: " << msg->getName() << " (" << msg->getClassName() << ")" << endl;

    ASSERT(dynamicPtrCast<const ReqRspLocMessage>(pkt->peekAtFront()));
    if (server) {
        // TODO: TTR packets (see AdjMngmtOld.cc -> socketDataArrived --> handleNeighMessage)
        handleLocReqMessage(pkt);
    }
    if (client) {
        handleLocRspMessage(pkt);
    }
    delete pkt;
}

void HierarchicalLocAssignAlgo::receiveSignal(cComponent *source, simsignal_t signalID, cObject *neighbour, cObject *details) {
    Enter_Method("%s", cComponent::getSignalName(signalID));
    cModule* neigh = check_and_cast<cModule*>(neighbour);
    if (client && signalID == AdjacencyManager::newNeighbourConnectedSignal) {
        // send Loc request
        auto payload = createLocReqPayload();
        payload->setSID(getHostID(neigh));
        sendToNeighbour(getHostID(neigh), payload);
        numNewNeighConnected++; // also serves as sequence number of send messages
    }
    if (client && signalID == AdjacencyManager::oldNeighbourDisconnectedSignal) {
        // cfr. fixDynamicRoutesClient - not part of locAssigning...
        auto &locToRemove = leased.at(getHostID(neigh));
        auto *ieLocToRemove = ift->findInterfaceByAddress(locToRemove);
        ASSERT(ieLocToRemove);
        // remove Loc
        removeLocClient(ieLocToRemove);
        // if we removed the active Loc
        if (chooseInterface(par("newLocInterface")) == ieLocToRemove) {
            //TODO: throw cRuntimeError("not yet implemented");
            // make old Loc active again
            // should also remove default gateway if it's the last...
        }
    }
    if (server && signalID == AdjacencyManager::newNeighbourConnectedSignal) {
        // Do nothing, client has to initiate Loc Request
    }
    if (server && signalID == AdjacencyManager::oldNeighbourDisconnectedSignal) {
        // TODO: clear lease or something
        // TODO: something with TTR, see AdjMgmtServer.cc
        // linkSevered(MNhostAdjMgmt->getIeNew()->getMacAddress()); // clientID always MAC of new interface
    }
    if (signalID != AdjacencyManager::newNeighbourConnectedSignal && signalID != AdjacencyManager::oldNeighbourDisconnectedSignal)
        throw cRuntimeError("Unexpected signal: %s", getSignalName(signalID));
}

Ptr<ReqRspLocMessage> HierarchicalLocAssignAlgo::createLocReqPayload() {
    const auto& getLoc = makeShared<ReqRspLocMessage>();
    uint16_t length = 100; // packet size without the options field
    getLoc->setOp(LOCREQUEST);
    getLoc->setSeqNumber(numNewNeighConnected);
    getLoc->setCID(getHostID(host)); // my ID
    getLoc->setChunkLength(B(length));
    getLoc->setLocUpdateCorrelationID(getCorrID(numLocUpdates+1)); // Next newLoc event
    return getLoc;
}
Ptr<ReqRspLocMessage> HierarchicalLocAssignAlgo::createLocRspPayload(const Ptr<const ReqRspLocMessage> req) {
    int seqNum = req->getSeqNumber();
    const auto& assignLoc = makeShared<ReqRspLocMessage>();
    uint16_t length = 100; // packet size without the options field
    assignLoc->setOp(LOCREPLY);
    assignLoc->setSeqNumber(seqNum);
    assignLoc->setSID(getHostID(host)); // server ID
    assignLoc->setCID(req->getCID()); // client ID
    assignLoc->setChunkLength(B(length));

    assignLoc->setSubnetMask(subnetMask);
    assignLoc->setSIface(iface);
    return assignLoc;
}
// client
void HierarchicalLocAssignAlgo::handleLocRspMessage(Packet *packet) {
    // TODO assign etc, see client
    const auto& msg = packet->peekAtFront<ReqRspLocMessage>();
    if (isFilteredMessageClient(msg))
        return;

    // assign Loc
    if (updateLocator(msg)) {
        // In network mechanisms
        // TODO: enable
//        if (strcasecmp(par("locChangingStrategy"), "TTR") == 0)
//            sendNeighLocUpdate(L3Address(ip), oldLocData.loc, oldLocData.neigh);
//        else if (strcasecmp(par("locChangingStrategy"), "ID") == 0)
//            /* sendNeighLocUpdate, with ID's */;
//        else if (strcasecmp(par("locChangingStrategy"), "end2end") != 0)
//            throw cRuntimeError("Unrecognized locator changing strategy");
    }

    EV_DEBUG << "Deleting " << packet << "." << endl; // happens by caller

    fixDynamicRoutesClient(packet);
}
bool HierarchicalLocAssignAlgo::updateLocator(const inet::Ptr<const ReqRspLocMessage> & msg) {
    //
    Locator const& loc = msg->getAssignedLoc();
    const Ipv4Address & subnetMask = msg->getSubnetMask();
    const Ipv4Address& ip = loc.getFinalDestination().toIpv4();
    //const Ipv4Address & subnetMask = ;
    auto ipv4Data = chooseInterface()->getProtocolDataForUpdate<Ipv4InterfaceData>();

    // remember which neighbouring server gave us our Locator
    leased.insert({msg->getSID(), ip});
    // Decision to Update Locator to new locator
    if (ipv4Data->getIPAddress() == ip)
        return false;

    /* assignNewLoc */
    //empty to prevent conflicts in GlobalArp
    auto ipv4DataOlda = chooseInterface(par("oldLocInterface"))->getProtocolDataForUpdate<Ipv4InterfaceData>();
    ASSERT2(ipv4DataOlda->getIPAddress().isUnspecified(), "Loc should have been removed beforehand by removing a neigh through AdjMgmt");

    //- numLocUpdates++; // tactically placed such that all future calls to removeOldLoc will have the same corrID as the corrID which is send by newLocAssignedSignal

    // Save oldLoc
    oldLocData.loc = ipv4Data->getIPAddress();
    oldLocData.netmask = ipv4Data->getNetmask();
    // FIXME: needed for TTR. oldLocData.neigh = getGateway(ie);
    // Assign newLoc
    ASSERT(!ip.isUnspecified());
    ASSERT(ip != oldLocData.loc.toIpv4());
    ipv4Data->setIPAddress(ip);
    ipv4Data->setNetmask(subnetMask);
    //- emit();
    // Info
    std::string banner = "Got new Loc " + ip.str();
    host->bubble(banner.c_str());
    EV_INFO << host->getFullName() << " got the following Loc assigned: "
            << ip << "/" << subnetMask << "." << endl;
    // Restore oldLoc
    auto ipv4DataOld = chooseInterface(par("oldLocInterface"))->getProtocolDataForUpdate<Ipv4InterfaceData>();
    if (!oldLocData.loc.isUnspecified()) {
        ipv4DataOld->setIPAddress(oldLocData.loc.toIpv4());
        ipv4DataOld->setNetmask(oldLocData.netmask);
    }

    /* final */
    numLocUpdates++;
    emit(newLocAssignedSignal, numLocUpdates, new Locator(ip));
    return true;
}
void HierarchicalLocAssignAlgo::fixDynamicRoutesClient(Packet* piggybackPkt) {
    // This should be part of the routing protocol part of the ControlPlane
    // NOT the locator assigning algo
    // However, this does not exist here, thus this 'hook' fixes this small job
    const auto& piggybackMsg = piggybackPkt->peekAtFront<ReqRspLocMessage>();

    // TODO: fix routing table
    Ipv4Route *iroute = nullptr;
    for (int i = 0; i < irt->getNumRoutes(); i++) {
        Ipv4Route *e = irt->getRoute(i);
        if (routeMatches(e, Ipv4Address(), Ipv4Address(), piggybackMsg->getSIface(), 0, chooseInterface()->getInterfaceName())) {
            iroute = e;
            break;
        }
    }
    if (iroute != nullptr)
        throw cRuntimeError("Unknown conditions, please check.");

    // Default gateway need to be installed
    Ipv4Route *route = new Ipv4Route();
    route->setDestination(Ipv4Address());
    route->setNetmask(Ipv4Address());
    route->setNextHop(piggybackMsg->getSIface());
    route->setInterface(getSourceInterfaceFrom(piggybackPkt));
    route->setSourceType(Ipv4Route::MANUAL);
    irt->addRoute(route);
}
// server
void HierarchicalLocAssignAlgo::handleLocReqMessage(Packet *packet) {
    /**
     *  NOT Enrollment (cfr Ouroboros)
     *  Handles incoming request Loc messages
     */
    if (isFilteredMessageServer(packet))
        return;

    const auto& msg = packet->peekAtFront<ReqRspLocMessage>();

    // insert neighbouring node into the networkgraph, by assigning it a Loc + linking to it
    L3Address assignedLoc = assignLoc(msg->getCID());
    auto payload = createLocRspPayload(msg); // sendAssignLocPacket()
    payload->setAssignedLoc(assignedLoc);

    EV_INFO << "Sending assignLoc." << endl;
    // FIXME, neighbour doesn't have an IP-addr
    sendToNeighbour(msg->getCID(), payload);

    EV_DEBUG << "Deleting " << packet << "." << endl; // happens by caller
}
bool HierarchicalLocAssignAlgo::isFilteredMessageServer(Packet *packet) {
    const auto& msg = packet->peekAtFront<ReqRspLocMessage>();
    if (msg->getOp() != LOCREQUEST) {
        EV_WARN << "Server received a non-LOCREQUEST message, dropping." << endl;
        return true;
    }
    // check that the packet arrived on the interface we are supposed to serve
    int inputInterfaceId = packet->getTag<InterfaceInd>()->getInterfaceId();
    if (inputInterfaceId != chooseInterface()->getInterfaceId()) {
        EV_WARN << "AdjMessage arrived on a different interface, dropping\n";
        ASSERT(false);
        return true;
    }
    if (msg->getSID() != getHostID(host)) {
        EV_WARN << "LocAssignMessage intended for different server, dropping\n";
        return true;
    }
    return false;
}
bool HierarchicalLocAssignAlgo::isFilteredMessageClient(const Ptr<const ReqRspLocMessage> & msg) {
    if (msg->getOp() != LOCREPLY) {
        EV_WARN << "Client received a non-LOCREPLY message, dropping." << endl;
        return true;
    }
    ASSERT(msg->getSeqNumber() <= numNewNeighConnected);
    if (msg->getSeqNumber() <= seqRcvd) {
        EV_WARN << "Message sequence number is not recent enough, dropping." << endl;
        // happens also because both antennas receive the same message (p.-)
        return true;
    }
    if (msg->getCID() != getHostID(host)) {
        EV_WARN << "Loc update not intended for this client, dropping." << endl;
        ASSERT(false);
        return true;
    }

    seqRcvd = msg->getSeqNumber();
    return false;
}
L3Address HierarchicalLocAssignAlgo::assignLoc(L3Address clientID) {
    auto * loc = getLocByID(clientID);
    if (loc)
        return *loc;
    // Assign new Loc
    int beginAddr = ipAddressStart.getInt(); // the first address that we might use
    for (unsigned int i = 0; i < maxNumOfClients; i++) {
        L3Address newLoc(Ipv4Address(beginAddr + i));
        if (!containsValue(leased, newLoc)) {
            // there is no expired lease so we create a new one
            leased[clientID] = newLoc;
            return leased[clientID];
        }
    }
    throw cRuntimeError("No new Loc available");
}
L3Address* HierarchicalLocAssignAlgo::getLocByID(L3Address clientID) {
    auto it = leased.find(clientID);
    if (it == leased.end()) {
        EV_DETAIL << "Lease not found for ID/MAC " << clientID << "." << endl;
        // lease does not exist
        return nullptr;
    }
    else {
        EV_DETAIL << "Found lease for ID/MAC " << clientID << "." << endl;
        return &(it->second);
    }
}

void HierarchicalLocAssignAlgo::removeOldLocClient() {
    removeLocClient(chooseInterface(par("oldLocInterface")));
}
void HierarchicalLocAssignAlgo::removeLocClient(NetworkInterface* ie) {
    auto ipv4DataOld = ie->getProtocolDataForUpdate<Ipv4InterfaceData>();
    if(!ipv4DataOld->getIPAddress().isUnspecified()) { //assume IP & netmask always configured together
        EV_WARN << "Deleting a Loc" << endl;
        ipv4DataOld->setIPAddress(Ipv4Address());
        ipv4DataOld->setNetmask(Ipv4Address());
//        emit(oldLocRemovedSignal, numLocUpdates);
    }
}
void HierarchicalLocAssignAlgo::sendToNeighbour(L3Address neighbour, Ptr<ReqRspLocMessage> payload) {
    short ttl = 1;
    std::ostringstream str;
    int seqNum = payload->getSeqNumber();

    str << (payload->getOp() == LOCREQUEST ? "getLoc" : "assignLoc") << "-" << seqNum;
    Packet *pkt = new Packet(str.str().c_str(), payload);
    // TODO - see routing/pim/modes/PimSM.cc/sendToIP
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(hierLocAssignAlgo);
    pkt->addTagIfAbsent<DispatchProtocolInd>()->setProtocol(hierLocAssignAlgo);
    pkt->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    // neighbour is the neigh ID, which will not be recognized, thus we need to bcast, on correct iface
    pkt->addTagIfAbsent<L3AddressReq>()->setDestAddress(Ipv4Address::ALLONES_ADDRESS);
    int ifaceID = irt->findBestMatchingRoute(neighbour)->getInterface()->getInterfaceId();
    pkt->addTagIfAbsent<InterfaceReq>()->setInterfaceId(ifaceID);
    pkt->addTagIfAbsent<HopLimitReq>()->setHopLimit(ttl);
    //sendToUdp(packet, clientPort, Ipv4Address::ALLONES_ADDRESS, serverPort);
    EV_INFO << "Sending ReqRspMessage." << endl;
    // client & server? - sendToUdp
//    EV_INFO << "Sending packet " << msg << endl;
//    msg->addTagIfAbsent<InterfaceReq>()->setInterfaceId(ie->getInterfaceId());
    send(pkt, peerOut);
}
void HierarchicalLocAssignAlgo::handleStartOperation(LifecycleOperation *operation) {
    if (!(server || client))
        return;

    NetworkInterface* ie = chooseInterface();
//    macAddress = ie->getMacAddress();

    // client
    simtime_t start = simTime(); // std::max(startTime
//    ieOld = chooseInterface(par("oldLocInterface"));

    if (server) {
        maxNumOfClients = par("maxNumClients");
        long numReservedLocs = 2; // hardcode reserved addresses (net + server)

        auto ipv4data = ie->getProtocolData<Ipv4InterfaceData>();
        subnetMask = ipv4data->getNetmask();
        uint32_t networkStartAddress = ipv4data->getIPAddress().getInt() & ipv4data->getNetmask().getInt();
        iface = ipv4data->getIPAddress();
        ipAddressStart = Ipv4Address(networkStartAddress + numReservedLocs);
        if (maxNumOfClients == -1) {
            maxNumOfClients = (~subnetMask.getInt()) - numReservedLocs; //also broadcast
        }
        if (!Ipv4Address::maskedAddrAreEqual(ipv4data->getIPAddress(), Ipv4Address(ipAddressStart.getInt() + maxNumOfClients - 1), subnetMask))
            throw cRuntimeError("Not enough IP addresses in subnet for %d clients", maxNumOfClients);
    }
}
void HierarchicalLocAssignAlgo::handleStopOperation(LifecycleOperation *operation) {
//    ie = nullptr;
    // client
//    ieOld = nullptr;
    // server
    leased.clear();
}

void HierarchicalLocAssignAlgo::handleCrashOperation(LifecycleOperation *operation) {
//    ie = nullptr;
    // client
//    ieOld = nullptr;
    // server
    leased.clear();
}

// need it? server + client
NetworkInterface *HierarchicalLocAssignAlgo::chooseInterface(const char *interfaceName) {
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
//namespace { // In front! Declaration before usage...
//static bool routeMatches(const Ipv4Route *entry, const Ipv4Address& target, const Ipv4Address& nmask,
//        const Ipv4Address& gw, int metric, const char *dev)
//{
//    if (!target.equals(entry->getDestination()))
//        return false;
//    if (!nmask.equals(entry->getNetmask()))
//        return false;
//    if (!gw.equals(entry->getGateway()))
//        return false;
//    if (metric && metric != entry->getMetric())
//        return false;
//    if (strcmp(dev, entry->getInterfaceName()))
//        return false;
//
//    return true;
//}
//}

