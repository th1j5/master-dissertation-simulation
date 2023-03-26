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
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"
#include "inet/networklayer/common/L3AddressResolver.h"

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
        arp.reference(this, "arpModule", true);
    }
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
    EV_WARN << "Old Loc is reachable: " << checkReachabilityOldLoc() << endl; //FIXME: other place!!
}
bool AdjacencyManagerClient::checkReachabilityOldLoc() {
    NetworkInterface* ie = chooseInterface(par("oldLocInterface"));

    auto mobility = check_and_cast<physicallayer::IRadio *>(ie->getSubmodule("radio"))->getAntenna()->getMobility();
    auto clientPos = mobility->getCurrentPosition();

    L3Address gatewayLoc(getGateway(ie));
    if (gatewayLoc.isUnspecified())
        return false;
    MacAddress gatewayMAC = arp->resolveL3Address(gatewayLoc, nullptr);
    auto ieGW = L3AddressResolver().findInterfaceWithMacAddress(gatewayMAC);
    auto radioGW = check_and_cast<physicallayer::IRadio *>(ieGW->getSubmodule("radio"));
    auto gatewayPos = radioGW->getAntenna()->getMobility()->getCurrentPosition();
    auto gatewayRange = radioGW->getTransmitter()->getMaxCommunicationRange();

    auto gateway = L3AddressResolver().findHostWithAddress(gatewayLoc);
    auto gatewayRadio = gateway->getSubmodule("wlan")->getSubmodule("radio");
    auto * gatewayMob = check_and_cast<IMobility *>(gateway->getSubmodule("mobility"));
    auto oldGatewayPos = gatewayMob->getCurrentPosition();
    return clientPos.distance(gatewayPos) < 90; //FIXME: 90 -> antenna send distance
}
Ipv4Address AdjacencyManagerClient::getGateway(NetworkInterface* ie) {
    // Assumption: gateways is subnet + 1
    if(ie->getIpv4Address().isUnspecified())
        return Ipv4Address();
    auto subnet = ie->getIpv4Address().getInt() & ie->getIpv4Netmask().getInt();
    return Ipv4Address(subnet+1);
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
        ipv4DataOld->setIPAddress(Ipv4Address()); //empty to prevent conflicts in GlobalArp
        auto netmaskOld = ipv4Data->getNetmask();
        ipv4DataOld->setNetmask(Ipv4Address());

        ipv4Data->setIPAddress(ip.toIpv4()); // FIXED: leads to errors when oldIP is still assigned
        ipv4Data->setNetmask(subnetMask);

        std::string banner = "Got new Loc " + ip.str();
        host->bubble(banner.c_str());

        EV_INFO << host->getFullName() << " got the following Loc assigned: "
                << ip << "/" << subnetMask << "." << endl;
        ipv4DataOld->setIPAddress(ipOld);
        ipv4DataOld->setNetmask(netmaskOld);

    // TODO: fix routing table
        Ipv4Route *iroute = nullptr;
        for (int i = 0; i < irt->getNumRoutes(); i++) {
            Ipv4Route *e = irt->getRoute(i);
            if (routeMatches(e, Ipv4Address(), Ipv4Address(), msg->getSLoc().toIpv4(), 0, ie->getInterfaceName())) {
                iroute = e;
                break;
            }
        }
        if (iroute == nullptr) {
            // create gateway route
            Ipv4Route *route = new Ipv4Route();
            route->setDestination(Ipv4Address());
            route->setNetmask(Ipv4Address());
            route->setGateway(msg->getSLoc().toIpv4());
            route->setInterface(ie);
            route->setSourceType(Ipv4Route::MANUAL);
            irt->addRoute(route);
        }
    }

    seqRcvd = msg->getSeqNumber();
    EV_DEBUG << "Deleting " << packet << "." << endl; // Delete happens by caller
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
