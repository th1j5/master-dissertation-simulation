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

#include "UdpBasicConnectionApp.h"

#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/FragmentationTag_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer//ipv4/Ipv4InterfaceData.h"
#include "inet/queueing/common/LabelsTag_m.h" // Alternative: inet/common/FlowTag.msg
// #include "inet/InterfaceTableAccess.h"
#include "AdjacencyManager/AdjacencyManagerClient.h"

using namespace inet;

Define_Module(UdpBasicConnectionApp);

simsignal_t locUpdateSentSignal = cComponent::registerSignal("locatorUpdateSent");
simsignal_t locUpdateRcvdSignal = cComponent::registerSignal("locatorUpdateReceived");

void UdpBasicConnectionApp::initialize(int stage)
{
    UdpBasicApp::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        host = getContainingNode(this);
        adjMgmt.reference(this, "adjacencyMgmt", false);
    }
}
void UdpBasicConnectionApp::processStart() {
    corrID_MN = std::vector<double>(1, -1.0); // processStart already sends a packet... FIXME
    UdpBasicApp::processStart();
    if (destAddresses.size() > 1)
        throw cRuntimeError("We cannot handle more than 1 communicating entity per app instantiation");
    if (!destAddresses.empty()) {
        host->subscribe(AdjacencyManagerClient::newLocAssignedSignal, this);
    } // only subscribe (=sending LocUpdates) if it has corresponding node
};
void UdpBasicConnectionApp::processStop() {
    UdpBasicApp::processStop();
    // assume unsubscribing doesn't error if not really subscribed...
    host->unsubscribe(AdjacencyManagerClient::newLocAssignedSignal, this);
};
void UdpBasicConnectionApp::receiveSignal(cComponent *source, simsignal_t signalID, intval_t numLocUpdates, cObject *details) {
    Enter_Method("%s", cComponent::getSignalName(signalID));
    const NetworkInterface *ie;

    if (signalID == AdjacencyManagerClient::newLocAssignedSignal) {
        ie = check_and_cast<const NetworkInterface *>(details);
        if (strcmp(ie->getInterfaceName(), par("newLocInterface")) != 0)
            throw cRuntimeError("new Loc Assigned signal is assigned to other interface than newLocInterface");
        L3Address newLocator = ie->getNetworkAddress();
        if (newLocator.isUnspecified())
            throw cRuntimeError("new Loc is unspecified");
        sendLocUpdate(newLocator, numLocUpdates);
    }
    else
        throw cRuntimeError("Unexpected signal: %s", getSignalName(signalID));
};

UdpBasicConnectionApp::~UdpBasicConnectionApp() {
    // Also calls parent destructor for a reason
}

//L3Address UdpBasicConnectionApp::getCurrentLoc()
//{
//    // IInterfaceTable* ift = check_and_cast<IInterfaceTable *>(getParentModule()->getSubmodule("interfaceTable"));
//    return;
//    L3Address localLoc;
//
//    if (ift->getInterface(1) != NULL) { // FIXME: brittle approach to get the correct Address
//        localLoc = ift->getInterface(1)->getNetworkAddress();
//    }
//    return localLoc;
//}
void UdpBasicConnectionApp::sendPacket()
{
    std::ostringstream str;
    str << packetName << "-" << numSent;
    Packet *packet = new Packet(str.str().c_str());
    if (dontFragment)
        packet->addTag<FragmentationReq>()->setDontFragment(true);
    const auto& payload = makeShared<MultiplexerPacket>();
    payload->setChunkLength(B(par("messageLength")));
    payload->setSequenceNumber(numSent);
    payload->addTag<CreationTimeTag>()->setCreationTime(simTime());
    payload->setMultiplexerDestination("app");
    L3Address destAddr = chooseDestAddr();
    double corrID = corrID_MN[0];
    payload->setLocUpdateCorrelationID(corrID);
    packet->insertAtBack(payload);
    emit(packetSentSignal, packet);
    socket.sendTo(packet, destAddr, destPort);
    numSent++;
}

void UdpBasicConnectionApp::sendLocUpdate(L3Address newLoc, int numLocUpdates)
{
    double corrID = dynamic_cast<AdjacencyManagerClient *>(adjMgmt.get())->getCorrID(numLocUpdates);
    std::ostringstream str;
    str << locUpdateName << "-" << numLocUpdates;
    Packet *packet = new Packet(str.str().c_str());
    if (dontFragment)
        packet->addTag<FragmentationReq>()->setDontFragment(true);
    const auto& payload = makeShared<LocatorUpdatePacket>();
    payload->setChunkLength(B(10)); // FIXME: hardcoded
    payload->setSequenceNumber(numSent);
    payload->setSequenceNumLocUpdate(numLocUpdates);
    payload->setLocUpdateCorrelationID(corrID);
    payload->setOldAddress(L3Address()); // TODO: update
    payload->setNewAddress(newLoc);
    payload->addTag<CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(payload);
    if (destAddresses.empty()) throw cRuntimeError("No peer to send the update to...");
    L3Address destAddr = chooseDestAddr();
    emit(locUpdateSentSignal, packet);
    socket.sendTo(packet, destAddr, destPort);
    numLocUpdateSend++;
}

void UdpBasicConnectionApp::processPacket(Packet *pk)
{
    EV_INFO << "Received packet: " << UdpSocket::getReceivedPacketInfo(pk) << endl;
    auto data = pk->peekData<MultiplexerPacket>();
    auto multiplexerDest = data->getMultiplexerDestination();
    if (strcasecmp(multiplexerDest, "app") == 0) {
        emit(packetReceivedSignal, pk); // Data packet
        numReceived++;
    }
    else if (strcasecmp(multiplexerDest, "flow_allocator") == 0) {
        emit(locUpdateRcvdSignal, pk);
        numLocUpdateReceived++;
        // FIXME: update destination location (brittle)
        auto locUpdateData = pk->peekData<LocatorUpdatePacket>();
        destAddresses[0] = locUpdateData->getNewAddress();
        corrID_MN[0] = locUpdateData->getLocUpdateCorrelationID();
    }
    else {
        throw cRuntimeError("unrecognized multiplexer destination");
    }
    delete pk;
}
