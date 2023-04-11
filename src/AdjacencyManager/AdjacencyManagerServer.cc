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

#include "AdjacencyManagerServer.h"
#include "AdjacencyManagerClient.h"
#include "cleanAdjMgmtMessage_m.h"

#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/common/stlutils.h"
#include "inet/networklayer/common/L3AddressResolver.h"

using namespace inet; // more OK to use in .cc

Define_Module(AdjacencyManagerServer);
simsignal_t neighLocUpdateRcvdSignal = cComponent::registerSignal("neighLocatorUpdateReceived");

AdjacencyManagerServer::~AdjacencyManagerServer() {
    // TODO Auto-generated destructor stub
    leased.clear();
}

void AdjacencyManagerServer::initialize(int stage) {
    AdjacencyManager::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        selfMsg = new CleanAdjMgmtMessage("unsubscribe resources please", CLEAN);
        ttr.reference(this, "transientTriangularRoutingModule", false); // false, when tested
    }
    WATCH_MAP(leased);
}
void AdjacencyManagerServer::handleSelfMessages(cMessage *msg) {
    cModule* MNhost;
    switch (msg->getKind()) {
    case CLEAN:
        MNhost = static_cast<CleanAdjMgmtMessage*>(msg)->getMNhostForUpdate();
        MNhost->unsubscribe(oldLocRemovedSignal, this);
        break;
    case SEND:
    default:
        throw cRuntimeError("Invalid kind %d in self message", (int)msg->getKind());
        break;
    }
}

bool AdjacencyManagerServer::isFilteredMessage(inet::Packet *packet) {
    const auto& msg = packet->peekAtFront<AdjMgmtMessage>();
    if (msg->getOp() != LOCREQUEST) {
        EV_WARN << "Server received a non-LOCREQUEST message, dropping." << endl;
        return true;
    }
    // check that the packet arrived on the interface we are supposed to serve
    int inputInterfaceId = packet->getTag<InterfaceInd>()->getInterfaceId();
    if (inputInterfaceId != ie->getInterfaceId()) {
        EV_WARN << "AdjMessage arrived on a different interface, dropping\n";
        return true;
    }
    return false;
}

void AdjacencyManagerServer::handleAdjMgmtMessage(inet::Packet *packet) {
    /**
     *  Enrollment (cfr Ouroboros)
     *  Handles incoming Ask Loc messages
     */
    if (isFilteredMessage(packet))
        return;

    const auto& msg = packet->peekAtFront<AdjMgmtMessage>();
    // insert neighbouring node into the networkgraph, by assigning it a Loc + linking to it
    L3Address assignedLoc = assignLoc(msg->getCID());
    sendAssignLocPacket(msg, assignedLoc);

    EV_DEBUG << "Deleting " << packet << "." << endl; // happens by caller
    numReceived++;
}

//void AdjacencyManagerServer::insertRoute(L3Address client) {
    /**
     *  We assume that all destinations are included in the default subnet route
     *  Furthermore, when the destination is not found in the Global ARP table, the packet will be simply dropped.
     *  This simplifies the state needed to remember
     */
//}

void AdjacencyManagerServer::handleNeighMessage(inet::Packet *pk) {
    /**
     *  Disenrollment (<-> enrollment)
     *  Remove neighbouring node from networkgraph by removing Loc + removing routes to it
     *  BUT, we can install a Transient Triangular Routing escape hatch for late packets
     */
    // adjust routing table + rerouting mechanisms
    const auto& msg = pk->peekAtFront<LocatorUpdatePacket>();
    auto oldLoc = msg->getOldAddress();
    // Warn Ttr
    if (ttr != nullptr) {
        ttr->addTTREntry(msg->getNewAddress(), oldLoc);
    }

    // Very unorthodox adjmgmt: subscribe with the client (==*other host*)
    auto MNhost = L3AddressResolver().findHostWithAddress(msg->getNewAddress());
    auto MNhostAdjMgmt = dynamic_cast<AdjacencyManagerClient*>(MNhost->getSubmodule("adjacencyManager"));
    if (MNhostAdjMgmt == nullptr)
        throw cRuntimeError("Didn't find AdjMgmt module of the MN");
    // check that the link with MN is still active
    if (oldLoc == MNhostAdjMgmt->getIeOld()->getNetworkAddress())
        MNhost->subscribe(oldLocRemovedSignal, this);
    else
        linkSevered(MNhostAdjMgmt->getIeNew()->getMacAddress()); // clientID

    emit(neighLocUpdateRcvdSignal, pk);
    // TODO
}

void AdjacencyManagerServer::linkSevered(MacAddress clientID) {
    if (ttr != nullptr) {
        ttr->activateEntry(leased[clientID]);
    }
}

void AdjacencyManagerServer::receiveSignal(cComponent *source, simsignal_t signalID, intval_t numLocUpdate, cObject *details) {
    Enter_Method("%s", cComponent::getSignalName(signalID));
    // emulate signal from lower layer that the client is not reachable anymore
    if (signalID == oldLocRemovedSignal) {
        auto MNhostAdjMgmt = dynamic_cast<AdjacencyManagerClient*>(source);
        auto MNhost = MNhostAdjMgmt->getParentModule();
        if (MNhostAdjMgmt == nullptr)
            throw cRuntimeError("Didn't find AdjMgmt module of the MN");
        // compare numLocUpdate - not needed:
        // if this signal is received, we always can cut it loose (either we were too late or just on time)
        linkSevered(MNhostAdjMgmt->getIeNew()->getMacAddress()); // clientID always MAC of new interface
        static_cast<CleanAdjMgmtMessage*>(selfMsg)->setMNhost(MNhost);
        scheduleAfter(0, selfMsg); // unsubscribe
    }
    else throw cRuntimeError("Unexpected signal: %s", getSignalName(signalID));
}

L3Address AdjacencyManagerServer::assignLoc(MacAddress clientID) {
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
L3Address* AdjacencyManagerServer::getLocByID(MacAddress clientID) {
    auto it = leased.find(clientID);
    if (it == leased.end()) {
        EV_DETAIL << "Lease not found for MAC " << clientID << "." << endl;
        // lease does not exist
        return nullptr;
    }
    else {
        EV_DETAIL << "Found lease for MAC " << clientID << "." << endl;
        return &(it->second);
    }
}

void AdjacencyManagerServer::sendAssignLocPacket(const Ptr<const AdjMgmtMessage> msg, L3Address assignedLoc)
{
    int seqNum = msg->getSeqNumber();
    std::ostringstream str;
    str << "assignLoc" << "-" << seqNum;
    Packet *packet = new Packet(str.str().c_str());
    const auto& assignLoc = makeShared<AdjMgmtMessage>();
    uint16_t length = 100; // packet size without the options field
    assignLoc->setOp(LOCREPLY);
    assignLoc->setSeqNumber(seqNum);
    assignLoc->setSID(macAddress); // server mac address...
    assignLoc->setCID(msg->getCID()); // client mac address...
    assignLoc->setChunkLength(B(length));
    assignLoc->setAssignedLoc(assignedLoc);
    assignLoc->setSLoc(locator);
    assignLoc->setSubnetMask(subnetMask);

    packet->insertAtBack(assignLoc);

    EV_INFO << "Sending assignLoc." << endl;
    sendToUdp(packet, serverPort, Ipv4Address::ALLONES_ADDRESS, clientPort);
}

void AdjacencyManagerServer::openSocket()
{
    socket.bind(serverPort);
    socket.setBroadcast(true);
    EV_INFO << "AdjacencyManager server bound to port " << clientPort << "." << endl;
}

void AdjacencyManagerServer::handleStartOperation(LifecycleOperation *operation)
{
    AdjacencyManager::handleStartOperation(operation);
    maxNumOfClients = par("maxNumClients");
    long numReservedLocs = 2; // hardcode reserved addresses (net + server)

    auto ipv4data = ie->getProtocolData<Ipv4InterfaceData>();
    subnetMask = ipv4data->getNetmask();
    uint32_t networkStartAddress = ipv4data->getIPAddress().getInt() & ipv4data->getNetmask().getInt();
    locator = L3Address(ipv4data->getIPAddress());
    ipAddressStart = Ipv4Address(networkStartAddress + numReservedLocs);
    if (maxNumOfClients == -1) {
        maxNumOfClients = (~subnetMask.getInt()) - numReservedLocs; //also broadcast
    }
    if (!Ipv4Address::maskedAddrAreEqual(ipv4data->getIPAddress(), Ipv4Address(ipAddressStart.getInt() + maxNumOfClients - 1), subnetMask))
        throw cRuntimeError("Not enough IP addresses in subnet for %d clients", maxNumOfClients);
}
void AdjacencyManagerServer::handleStopOperation(LifecycleOperation *operation)
{
    leased.clear();
}

void AdjacencyManagerServer::handleCrashOperation(LifecycleOperation *operation)
{
    leased.clear();
}
