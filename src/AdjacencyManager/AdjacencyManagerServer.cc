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

#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/common/stlutils.h"

using namespace inet; // more OK to use in .cc

Define_Module(AdjacencyManagerServer);

AdjacencyManagerServer::~AdjacencyManagerServer() {
    // TODO Auto-generated destructor stub
}

void AdjacencyManagerServer::initialize(int stage) {
    AdjacencyManager::initialize(stage);
    WATCH_MAP(leased);
}
void AdjacencyManagerServer::handleSelfMessages(cMessage *msg) {
    throw cRuntimeError("Unknown selfmessage type!");
}

void AdjacencyManagerServer::handleAdjMgmtMessage(inet::Packet *packet) {
    // Handles incoming Ask Loc messages
    const auto& msg = packet->peekAtFront<AdjMgmtMessage>();
    if (msg->getOp() != LOCREQUEST) {
        EV_WARN << "Server received a non-LOCREQUEST message, dropping." << endl;
        return;
    }
    // check that the packet arrived on the interface we are supposed to serve
    int inputInterfaceId = packet->getTag<InterfaceInd>()->getInterfaceId();
    if (inputInterfaceId != ie->getInterfaceId()) {
        EV_WARN << "AdjMessage arrived on a different interface, dropping\n";
        return;
    }
    L3Address assignedLoc = assignLoc(msg->getCID());
    sendAssignLocPacket(msg, assignedLoc);

    EV_DEBUG << "Deleting " << packet << "." << endl; // happens by caller
    numReceived++;
}

void AdjacencyManagerServer::handleNeighMessage(inet::Packet *pk) {
    // adjust routing table + rerouting mechanisms
    // Warn Ttr...
    // TODO: Do we need a timer?? see mechanism 3
    //const auto& msg = pk->peekAtFront<LocatorUpdatePacket>();
    //    Ipv4Route *reroute = new Ipv4Route();
    //    reroute->setDestination(msg->getOldAddress());
    //    reroute->setNetmask(Ipv4Address::ALLONES_ADDRESS);
    //    reroute->setGateway(Ipv4Address::LOOPBACK_ADDRESS);
    //    reroute->setInterface(chooseInterface("lo0"));
    //    reroute->setSourceType(IRoute::MANUAL);
    //    irt->addRoute(reroute);
    // TODO
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
