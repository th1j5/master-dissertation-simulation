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
#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/common/FragmentationTag_m.h"
#include "inet/common/TimeTag_m.h"

using namespace inet; // more OK to use in .cc

Define_Module(AdjacencyManagerClient);
simsignal_t neighLocUpdateSentSignal = cComponent::registerSignal("neighLocatorUpdateSent");
simsignal_t AdjacencyManagerClient::newLocAssignedSignal = cComponent::registerSignal("newLocatorAssigned");

AdjacencyManagerClient::~AdjacencyManagerClient() {
    if (host->isSubscribed(IMobility::mobilityStateChangedSignal, this))
        host->unsubscribe(IMobility::mobilityStateChangedSignal, this);
}

void AdjacencyManagerClient::initialize(int stage) {
    AdjacencyManager::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        // timers
        selfMsg = new cMessage("getLocTimer", SEND);
        arp.reference(this, "arpModule", true);
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        host->subscribe(IMobility::mobilityStateChangedSignal, this);
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
    getLoc->setLocUpdateCorrelationID(getCorrID(numLocUpdates+1)); // Next newLoc event

    packet->insertAtBack(getLoc);

    EV_INFO << "Sending getLoc." << endl;
    sendToUdp(packet, clientPort, Ipv4Address::ALLONES_ADDRESS, serverPort);
    seqSend++;
}

// Checks all links to determine if the flows to this locator are still working
bool AdjacencyManagerClient::isLocReachable(NetworkInterface* ie1) {
    auto mobility = check_and_cast<physicallayer::IRadio *>(ie1->getSubmodule("radio"))->getAntenna()->getMobility();
    auto clientPos = mobility->getCurrentPosition();

    // All link enpoint nodes (special case: only 1, namely the 'gateway')
    L3Address gatewayLoc = getGateway(ie1);
    if (gatewayLoc.isUnspecified())
        return false;
    MacAddress gatewayMAC = arp->resolveL3Address(gatewayLoc, nullptr);
    auto ieGW = L3AddressResolver().findInterfaceWithMacAddress(gatewayMAC);
    auto radioGW = check_and_cast<physicallayer::IRadio *>(ieGW->getSubmodule("radio"));
    auto gatewayPos = radioGW->getAntenna()->getMobility()->getCurrentPosition();
    auto gatewayRange = radioGW->getTransmitter()->getMaxCommunicationRange();

    return (units::values::m) clientPos.distance(gatewayPos) < gatewayRange;
}
Ipv4Address AdjacencyManagerClient::getGateway(NetworkInterface* ie) {
    // Assumption: gateways is subnet + 1
    if(ie->getIpv4Address().isUnspecified())
        return Ipv4Address();
    auto subnet = ie->getIpv4Address().getInt() & ie->getIpv4Netmask().getInt();
    return Ipv4Address(subnet+1);
}

bool AdjacencyManagerClient::isFilteredMessage(const Ptr<const AdjMgmtMessage> & msg) {
    if (msg->getOp() != LOCREPLY) {
        EV_WARN << "Client received a non-LOCREPLY message, dropping." << endl;
        return true;
    }
    ASSERT(msg->getSeqNumber() <= seqSend);
    if (msg->getSeqNumber() <= seqRcvd) {
        EV_WARN << "Message sequence number is not recent enough, dropping." << endl;
        return true;
    }
    if (msg->getCID() != macAddress) {
        EV_WARN << "Loc update not intended for this client, dropping." << endl;
        return true;
    }
    return false;
}

void AdjacencyManagerClient::handleAdjMgmtMessage(inet::Packet *packet) {
    // Handles incoming LocAssignment messages
    const auto& msg = packet->peekAtFront<AdjMgmtMessage>();
    if (isFilteredMessage(msg))
        return;

    // assign Loc
    const Ipv4Address& ip = msg->getAssignedLoc().toIpv4();
    auto ipv4Data = ie->getProtocolDataForUpdate<Ipv4InterfaceData>();

    // Decision to Update Locator to new locator
    if (ipv4Data->getIPAddress() != ip) {
        assignNewLoc(ip_netmask{ip, msg->getSubnetMask()});

        // In network mechanisms
        if (strcasecmp(par("locChangingStrategy"), "TTR") == 0)
            sendNeighLocUpdate(L3Address(ip), oldLocData.loc, oldLocData.neigh);
        else if (strcasecmp(par("locChangingStrategy"), "ID") == 0)
            /* sendNeighLocUpdate, with ID's */;
        else if (strcasecmp(par("locChangingStrategy"), "end2end") != 0)
            throw cRuntimeError("Unrecognized locator changing strategy");

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
            // create gateway route - replaces previous default route
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
    ieOld = chooseInterface(par("oldLocInterface"));
    scheduleAt(start, selfMsg);
}

void AdjacencyManagerClient::handleStopOperation(LifecycleOperation *operation)
{
    AdjacencyManager::handleStopOperation(operation);
    ieOld = nullptr;
    cancelEvent(selfMsg);
}

void AdjacencyManagerClient::handleCrashOperation(LifecycleOperation *operation)
{
    AdjacencyManager::handleCrashOperation(operation);
    ieOld = nullptr;
    cancelEvent(selfMsg);
}
void AdjacencyManagerClient::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) {
    Enter_Method("%s", cComponent::getSignalName(signalID));
    if (signalID == IMobility::mobilityStateChangedSignal) {
        // make decisions to adjust graph
        if (!isLocReachable(ieOld)) {
            EV << "Old Loc has become unreachable" << endl;
            removeOldLoc();
        }
        if (!isLocReachable(ie)) {
            // Decide to keep at least 1 loc (although it is unreachable)
            // But for metrics, it is interesting to know when this became unreachable,
            // thus use next corrID (because the next update lies in the future)
            emit(oldLocRemovedSignal, numLocUpdates+1);
        }
    }
    else throw cRuntimeError("Unexpected signal: %s", getSignalName(signalID));
}
void AdjacencyManagerClient::assignNewLoc(ip_netmask ip_net) {
    const Ipv4Address& ip = ip_net.ip;
    const Ipv4Address& subnetMask = ip_net.netmask;
    //empty to prevent conflicts in GlobalArp
    removeOldLoc(); // oldOld loc

    numLocUpdates++; // tactically placed such that all future calls to removeOldLoc will have the same corrID as the corrID which is send by newLocAssignedSignal

    // saveOldLoc();
    auto ipv4Data = ie->getProtocolDataForUpdate<Ipv4InterfaceData>();
    oldLocData.loc = ipv4Data->getIPAddress();
    oldLocData.netmask = ipv4Data->getNetmask();
    oldLocData.neigh = getGateway(ie);
    // assignNewLoc();
    ASSERT(!ip.isUnspecified());
    ASSERT(ip != oldLocData.loc.toIpv4());
    ipv4Data->setIPAddress(ip);
    ipv4Data->setNetmask(subnetMask);
    emit(newLocAssignedSignal, numLocUpdates, ie);
    // info
    std::string banner = "Got new Loc " + ip.str();
    host->bubble(banner.c_str());
    EV_INFO << host->getFullName() << " got the following Loc assigned: "
            << ip << "/" << subnetMask << "." << endl;
    // restoreOldLoc();
    auto ipv4DataOld = ieOld->getProtocolDataForUpdate<Ipv4InterfaceData>();
    if (!oldLocData.loc.isUnspecified()) {
        ipv4DataOld->setIPAddress(oldLocData.loc.toIpv4());
        ipv4DataOld->setNetmask(oldLocData.netmask);
    }
}
void AdjacencyManagerClient::removeOldLoc() {
    auto ipv4DataOld = ieOld->getProtocolDataForUpdate<Ipv4InterfaceData>();
    if(!ipv4DataOld->getIPAddress().isUnspecified()) { //assume IP & netmask always configured together
        EV_WARN << "Deleting old Loc" << endl;
        ipv4DataOld->setIPAddress(Ipv4Address());
        ipv4DataOld->setNetmask(Ipv4Address());
        emit(oldLocRemovedSignal, numLocUpdates);
    }
}

void AdjacencyManagerClient::sendNeighLocUpdate(L3Address newLoc, L3Address oldLoc, L3Address neigh)
{
    if (oldLoc.isUnspecified())
        return;
    std::ostringstream str;
    str << locUpdateName << "-" << numLocUpdates;
    Packet *packet = new Packet(str.str().c_str());
    packet->addTag<FragmentationReq>()->setDontFragment(true); //dontFragment
    const auto& payload = makeShared<LocatorUpdatePacket>();
    payload->setChunkLength(B(10)); // FIXME: hardcoded
    payload->setSequenceNumber(numSent);
    payload->setSequenceNumLocUpdate(numLocUpdates);
    payload->setLocUpdateCorrelationID(getCorrID(numLocUpdates));
    payload->setOldAddress(oldLoc);
    payload->setNewAddress(newLoc);
    payload->addTag<CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(payload);
    emit(neighLocUpdateSentSignal, packet);
    socket.sendTo(packet, neigh, serverPort);
    ASSERT(numLocUpdateSend+1 == numLocUpdates);
    numLocUpdateSend++;

}
