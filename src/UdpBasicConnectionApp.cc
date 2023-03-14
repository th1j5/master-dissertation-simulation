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

using namespace inet;

Define_Module(UdpBasicConnectionApp);

simsignal_t locUpdateSentSignal = cComponent::registerSignal("locatorUpdateSent");
simsignal_t locUpdateRcvdSignal = cComponent::registerSignal("locatorUpdateReceived");

void UdpBasicConnectionApp::initialize(int stage)
{
    UdpBasicApp::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        host = getContainingNode(this);
    }
}
void UdpBasicConnectionApp::processStart() {
    UdpBasicApp::processStart();
    if (!destAddresses.empty()) {
        host->subscribe(interfaceConfigChangedSignal, this);
        host->subscribe(interfaceIpv4ConfigChangedSignal, this);
        host->subscribe(interfaceStateChangedSignal, this);
    } // only subscribe (=sending LocUpdates) if it has corresponding node
};
void UdpBasicConnectionApp::processStop() {
    UdpBasicApp::processStop();
    // assume unsubscribing doesn't error if not really subscribed...
    host->unsubscribe(interfaceConfigChangedSignal, this);
    host->unsubscribe(interfaceIpv4ConfigChangedSignal, this);
    host->unsubscribe(interfaceStateChangedSignal, this);
};
void UdpBasicConnectionApp::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) {
    Enter_Method("%s", cComponent::getSignalName(signalID));

    const NetworkInterface *ie;
    const NetworkInterfaceChangeDetails *change;

    // interfaceStateChangedSignal
    // interfaceConfigChangedSignal
    if (signalID == interfaceIpv4ConfigChangedSignal) {
        change = check_and_cast<const NetworkInterfaceChangeDetails *>(obj);
        auto fieldId = change->getFieldId();
        if(fieldId == Ipv4InterfaceData::F_IP_ADDRESS) {
            // With DHCP leases, both the IP and Ipv4InterfaceData::F_NETMASK are changed, sequentially after each other
            EV_WARN << "Thijs: Config IPv4 changed signal" << change;
            ie = change->getNetworkInterface();
            // TODO: Check if IP address really changed
            // FIXME: don't send if the address becomes empty
            L3Address newLocator = ie->getNetworkAddress();
            if (!newLocator.isUnspecified()) sendLocUpdate(newLocator);
        }
    }
    else if (signalID == interfaceStateChangedSignal) {
        // Ignore
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
    packet->insertAtBack(payload);
    L3Address destAddr = chooseDestAddr();
    emit(packetSentSignal, packet);
    socket.sendTo(packet, destAddr, destPort);
    numSent++;
}

void UdpBasicConnectionApp::sendLocUpdate(L3Address newLoc)
{
    std::ostringstream str;
    str << locUpdateName << "-" << numLocUpdateSend;
    Packet *packet = new Packet(str.str().c_str());
    if (dontFragment)
        packet->addTag<FragmentationReq>()->setDontFragment(true);
    const auto& payload = makeShared<LocatorUpdatePacket>();
    payload->setChunkLength(B(10)); // FIXME: hardcoded
    payload->setSequenceNumber(numSent);
    payload->setSequenceNumLocUpdate(numLocUpdateSend);
    EV << L3Address();
    payload->setOldAddress(0); // TODO: update
    payload->setNewAddress(newLoc);
    payload->addTag<CreationTimeTag>()->setCreationTime(simTime());
    payload->setMultiplexerDestination("flow_allocator");
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
        emit(packetReceivedSignal, pk);
        numReceived++;
    }
    else if (strcasecmp(multiplexerDest, "flow_allocator") == 0) {
        emit(locUpdateRcvdSignal, pk);
        numLocUpdateReceived++;
        // FIXME: update destination location (brittle)
        auto locUpdateData = pk->peekData<LocatorUpdatePacket>();
        destAddresses[0] = locUpdateData->getNewAddress();
    }
    else {
        throw cRuntimeError("unrecognized multiplexer destination");
    }
    delete pk;
}
