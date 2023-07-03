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
#include "inet/networklayer//ipv4/Ipv4InterfaceData.h"
#include "inet/queueing/common/LabelsTag_m.h" // Alternative: inet/common/FlowTag.msg

#include "NodeIDToLocatorResolver.h"
#include "UniSphere/LocatorTag_m.h"
// #include "inet/InterfaceTableAccess.h"
//#include "AdjacencyManager/AdjacencyManagerClient.h"

using namespace inet;

Define_Module(UdpBasicConnectionApp);

simsignal_t locUpdateSentSignal = cComponent::registerSignal("locatorUpdateSent");
simsignal_t locUpdateRcvdSignal = cComponent::registerSignal("locatorUpdateReceived");

void UdpBasicConnectionApp::initialize(int stage)
{
    UdpBasicApp::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        host = getContainingNode(this);
        //adjMgmt.reference(this, "adjacencyMgmt", false);
        if (isUniSphere())
            controlPlane = check_and_cast<LocUpdatable*>(host->getSubmodule("unisphere"));
        else;
//            controlPlane = host->getSubmodule("adjacencyManager"); // TODO: get member which represents the IPv4 control plane
        ASSERT(controlPlane);
    }
}
void UdpBasicConnectionApp::processStart() {
    corrID_MN = std::vector<double>(1, -1.0); // processStart already sends a packet... FIXME

    /* start UdpBasicApp::processStart */
    socket.setOutputGate(gate("socketOut"));
    const char *localAddress = par("localAddress");
    socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
    setSocketOptions();

    const char *destAddrs = par("destAddresses");
    cStringTokenizer tokenizer(destAddrs);
    const char *token;
    {
        UniSphereLocator b = UniSphereLocator();
        EV_WARN << b.ID << endl;
    }

    while ((token = tokenizer.nextToken()) != nullptr) {
        destAddressStr.push_back(token);
        Locator result;
        NodeIDToLocatorResolver().tryResolve(token, result);
        if (result.isUnspecified())
            EV_ERROR << "cannot resolve destination address: " << token << endl;
        destAddresses.push_back(result);
    }

    if (!destAddresses.empty()) {
        selfMsg->setKind(SEND);
        processSend();
    }
    else {
        if (stopTime >= CLOCKTIME_ZERO) {
            selfMsg->setKind(STOP);
            scheduleClockEventAt(stopTime, selfMsg);
        }
    }
    /* end UdpBasicApp::processStart */

    if (destAddresses.size() > 1)
        throw cRuntimeError("We cannot handle more than 1 communicating entity per app instantiation");
    if (!destAddresses.empty()) {
        host->subscribe(LocUpdatable::newLocAssignedSignal, this);
    } // only subscribe (=sending LocUpdates) if it has corresponding node
};
void UdpBasicConnectionApp::processStop() {
    UdpBasicApp::processStop();
    // assume unsubscribing doesn't error if not really subscribed...
    if (host->isSubscribed(LocUpdatable::newLocAssignedSignal, this))
        host->unsubscribe(LocUpdatable::newLocAssignedSignal, this);
};
void UdpBasicConnectionApp::receiveSignal(cComponent *source, simsignal_t signalID, intval_t numLocUpdates, cObject *details) {
    Enter_Method("%s", cComponent::getSignalName(signalID));
//    const NetworkInterface *ie;
    const Locator *newLocator;

    if (signalID == LocUpdatable::newLocAssignedSignal) {
        newLocator = check_and_cast<const Locator*>(details);
//        ie = check_and_cast<const NetworkInterface *>(details);
//        if (strcmp(ie->getInterfaceName(), par("newLocInterface")) != 0)
//            throw cRuntimeError("new Loc Assigned signal is assigned to other interface than newLocInterface");
//        L3Address newLocator = ie->getNetworkAddress();
        if (newLocator->isUnspecified())
            throw cRuntimeError("new Loc is unspecified");
        sendLocUpdate(*newLocator, numLocUpdates);
        delete newLocator;
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
    const auto& payload = makeShared<MultiplexerPacket>();
    payload->setChunkLength(B(par("messageLength")));
    payload->setMultiplexerDestination("app");
    double corrID = corrID_MN[0];
    payload->setLocUpdateCorrelationID(corrID);

    if (sendPayload(payload, str, packetSentSignal))
        numSent++;
}

void UdpBasicConnectionApp::sendLocUpdate(Locator newLoc, int numLocUpdates)
{
    //double corrID = dynamic_cast<AdjacencyManagerClient *>(adjMgmt.get())->getCorrID(numLocUpdates);

    double corrID = controlPlane->getCorrID(numLocUpdates);

    std::ostringstream str;
    str << locUpdateName << "-" << numLocUpdates;
    const auto& payload = makeShared<LocatorUpdatePacket>();
    payload->setChunkLength(B(10)); // FIXME: hardcoded
    payload->setSequenceNumLocUpdate(numLocUpdates);
    payload->setLocUpdateCorrelationID(corrID);
//    payload->setOldAddress(); // TODO: update
    payload->setNewAddress(newLoc);

    if (sendPayload(payload, str, locUpdateSentSignal))
        numLocUpdateSend++;
}
bool UdpBasicConnectionApp::sendPayload(const Ptr<MultiplexerPacket>& payload, std::ostringstream& str, simsignal_t signal) {
    if (destAddresses.empty())
        throw cRuntimeError("No peer to send the update to...");
    // don't send packets if addr is still isUnspecified()
    Locator destAddr = chooseDestLoc();
    if (destAddr.isUnspecified())
        return false;

    payload->setSequenceNumber(numSent);
    payload->addTag<CreationTimeTag>()->setCreationTime(simTime());

    Packet *packet = new Packet(str.str().c_str(), payload);
//    packet->insertAtBack(payload);
    if (dontFragment)
        packet->addTag<FragmentationReq>()->setDontFragment(true);

    auto& locReq = packet->addTagIfAbsent<LocatorReq>();
    locReq->setDestLoc(destAddr);

    emit(signal, packet);
    socket.sendTo(packet, destAddr.getFinalDestination(), destPort); // Uni-Sphere also uses destAddr
    return true;
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
        destAddresses[0] = locUpdateData->getNewAddress(); //FIXME!
        corrID_MN[0] = locUpdateData->getLocUpdateCorrelationID();
    }
    else {
        throw cRuntimeError("unrecognized multiplexer destination");
    }
    delete pk;
}

Locator UdpBasicConnectionApp::chooseDestLoc()
{
    int k = intrand(destAddresses.size());
    if (destAddresses[k].isUnspecified() || (!isUniSphere() && destAddresses[k].getFinalDestination().isLinkLocal())) {
        NodeIDToLocatorResolver().tryResolve(destAddressStr[k].c_str(), destAddresses[k]);
    }
    return destAddresses[k];
}

