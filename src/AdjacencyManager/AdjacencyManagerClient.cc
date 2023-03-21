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

#include "AdjacencyManagerClient.h"

#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"

using namespace inet; // more OK to use in .cc

Define_Module(AdjacencyManagerClient);

AdjacencyManagerClient::~AdjacencyManagerClient() {
    cancelAndDelete(selfMsg);
}

void AdjacencyManagerClient::initialize(int stage) {
    AdjacencyManager::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
            // timers
            selfMsg = new cMessage("getLocTimer", SEND);
    }
}
void AdjacencyManagerClient::handleSelfMessages(cMessage *msg) {
    switch (msg->getKind()) {
    case SEND:
        processGetLoc();
        // handleTimer(msg); --> initClient() --> sendDiscover
        break;
    default:
        throw cRuntimeError("Invalid kind %d in self message", (int)msg->getKind());
        break;
    }
}
void AdjacencyManagerClient::processGetLoc() {
    sendGetLocPacket();
    simtime_t d = par("getLocInterval");
    selfMsg->setKind(SEND);
    scheduleAfter(d, selfMsg);
}
void AdjacencyManagerClient::sendGetLocPacket()
{
    std::ostringstream str;
    str << "getLoc" << "-" << seqSend;
    Packet *packet = new Packet(str.str().c_str());
    const auto& getLoc = makeShared<AdjMgmtMessage>();
    uint16_t length = 100; // packet size without the options field
    getLoc->setOp(LOCREQUEST);
    getLoc->setSeqNumber(seqSend);
    getLoc->setCID(macAddress); // my mac address
    getLoc->setChunkLength(B(length));

    packet->insertAtBack(getLoc);

    EV_INFO << "Sending getLoc." << endl;
    sendToUdp(packet, clientPort, Ipv4Address::ALLONES_ADDRESS, serverPort);
    seqSend++;
}

void AdjacencyManagerClient::handleAdjMgmtMessage(inet::Packet *packet) {
    // Handles incoming LocAssignment messages
    const auto& msg = packet->peekAtFront<AdjMgmtMessage>();
    if (msg->getOp() != LOCREPLY) {
        EV_WARN << "Client received a non-LOCREPLY message, dropping." << endl;
        return;
    }
    ASSERT(msg->getSeqNumber() <= seqSend);
    if (msg->getSeqNumber() <= seqRcvd) {
        EV_WARN << "Message sequence number is not recent enough, dropping." << endl;
        return;
    }
    if (msg->getCID() != macAddress) {
        EV_WARN << "Loc update not intended for this client, dropping." << endl;
        return;
    }
    // assign Loc
    auto ipv4Data = ie->getProtocolDataForUpdate<Ipv4InterfaceData>();
    const L3Address& ip = msg->getAssignedLoc();
    const Ipv4Address& subnetMask = msg->getSubnetMask();

    if (ipv4Data->getIPAddress() != ip.toIpv4()) {
        NetworkInterface* ieOld = chooseInterface(par("oldLocInterface"));
        auto ipv4DataOld = ieOld->getProtocolDataForUpdate<Ipv4InterfaceData>();
        auto ipOld = ipv4Data->getIPAddress();
        auto netmaskOld = ipv4Data->getNetmask();

        ipv4Data->setIPAddress(ip.toIpv4());
        ipv4Data->setNetmask(subnetMask);

        std::string banner = "Got new Loc " + ip.str();
        host->bubble(banner.c_str());

        EV_INFO << host->getFullName() << " got the following Loc assigned: "
                << ip << "/" << subnetMask << "." << endl;
        ipv4DataOld->setIPAddress(ipOld);
        ipv4DataOld->setNetmask(netmaskOld);

    // TODO: fix routing table
    }

    seqRcvd = msg->getSeqNumber();
    EV_DEBUG << "Deleting " << packet << "." << endl;
    delete packet;
}

void AdjacencyManagerClient::openSocket()
{
    socket.bind(clientPort);
    socket.setBroadcast(true);
    EV_INFO << "AdjacencyManager client bound to port " << clientPort << "." << endl;
}
void AdjacencyManagerClient::handleStartOperation(LifecycleOperation *operation)
{
    AdjacencyManager::handleStartOperation(operation);

    simtime_t start = simTime(); // std::max(startTime
    scheduleAt(start, selfMsg);
}

void AdjacencyManagerClient::handleStopOperation(LifecycleOperation *operation)
{
    AdjacencyManager::handleStopOperation(operation);
    cancelEvent(selfMsg);
}

void AdjacencyManagerClient::handleCrashOperation(LifecycleOperation *operation)
{
    AdjacencyManager::handleCrashOperation(operation);
    cancelEvent(selfMsg);
}
