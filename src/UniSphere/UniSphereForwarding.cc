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

#include "UniSphereForwarding.h"
#include "UniSphereForwardingHeader_m.h"
#include "LocatorTag_m.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/common/socket/SocketTag_m.h"
#include "inet/common/stlutils.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/networklayer/common/HopLimitTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/L3Tools.h"
#include "inet/networklayer/common/NextHopAddressTag_m.h"
#include "inet/networklayer/contract/L3SocketCommand_m.h"
#include "inet/networklayer/nexthop/NextHopForwardingHeader_m.h"
#include "inet/networklayer/nexthop/NextHopInterfaceData.h"
#include "inet/networklayer/nexthop/NextHopRoute.h"
#include "inet/networklayer/nexthop/NextHopRoutingTable.h"

using namespace inet; // more OK to use in .cc
Define_Module(UniSphereForwarding);

void UniSphereForwarding::routePacket(Packet *datagram, const NetworkInterface *destIE, const L3Address& requestedNextHop, bool fromHL)
{
    // TODO add option handling code here

    auto header = datagram->peekAtFront<UniSphereForwardingHeader>();
    L3Address destAddr = header->getDestinationAddress();

    EV_INFO << "Routing datagram `" << datagram->getName() << "' with dest=" << destAddr << ": ";

    // check for local delivery
    if (routingTable->isLocalAddress(destAddr)) {
        EV_INFO << "local delivery\n";
        if (fromHL && header->getSourceAddress().isUnspecified()) {
            datagram->trimFront();
            const auto& newHeader = removeNetworkProtocolHeader<UniSphereForwardingHeader>(datagram);
            newHeader->setSourceAddress(destAddr); // allows two apps on the same host to communicate
            insertNetworkProtocolHeader(datagram, Protocol::nextHopForwarding, newHeader);
            header = newHeader;
        }
        numLocalDeliver++;

        if (datagramLocalInHook(datagram) != INetfilter::IHook::ACCEPT)
            return;

        sendDatagramToHL(datagram);
        return;
    }

    // if datagram arrived from input gate and forwarding is off, delete datagram
    if (!fromHL && !routingTable->isForwardingEnabled()) {
        EV_INFO << "forwarding off, dropping packet\n";
        numDropped++;
        delete datagram;
        return;
    }

    if (!fromHL) {
        datagram->trim();
    }

    // if output port was explicitly requested, use that, otherwise use NextHopForwarding routing
    // TODO see Ipv4, using destIE here leaves nextHope unspecified
    L3Address nextHop;
    if (destIE && !requestedNextHop.isUnspecified()) {
        EV_DETAIL << "using manually specified output interface " << destIE->getInterfaceName() << "\n";
        nextHop = requestedNextHop;
    }
    else {
        // use NextHopForwarding routing (lookup in routing table)
        const NextHopRoute *re = routingTable->findBestMatchingRoute(destAddr);

        /* U-Sphere */
        // error handling: destination address does not exist in routing table:
        // look at Locator
        if (re == nullptr && header->getPath().size() > 0) {
            // try to find landmark/next hop
            RoutingPath path = header->getPath();
            // landmark could be the sending node...
            if (routingTable->isLocalAddress(path.top())) {
                path.pop();
                // This should mean that this packets comes from this node && this node is a landmark
                EV_WARN << "NextHop in RoutingPath is local address" << endl;
            }
            if (path.size() > 0) {
                re = routingTable->findBestMatchingRoute(path.top()); //FIXME: check correctness
                // if it is the nexthop, pop it from the path
                if (re != nullptr && re->getNextHopAsGeneric() == path.top()) {
                    const auto& newHeader = removeNetworkProtocolHeader<UniSphereForwardingHeader>(datagram);
                    path.pop();
                    newHeader->setPath(path);
                    insertNetworkProtocolHeader(datagram, Protocol::nextHopForwarding, newHeader);
                    header = newHeader;
                }
            }
            // else: strange case: the node is a landmark && the pkt is send by this node && this node is receiver too?
        }
        if (re == nullptr) {
            // last
            EV_INFO << "unroutable, discarding packet\n";
            numUnroutable++;
            PacketDropDetails details;
            details.setReason(NO_ROUTE_FOUND);
            emit(packetDroppedSignal, datagram, &details);
            delete datagram;
            return;
        }

        // extract interface and next-hop address from routing table entry
        destIE = re->getInterface();
        nextHop = re->getNextHopAsGeneric();
    }

    if (!fromHL) {
        datagram->trimFront();
        const auto& newHeader = removeNetworkProtocolHeader<UniSphereForwardingHeader>(datagram);
        newHeader->setHopLimit(header->getHopLimit() - 1);
        insertNetworkProtocolHeader(datagram, Protocol::nextHopForwarding, newHeader);
        header = newHeader;
    }

    // set datagram source address if not yet set
    if (header->getSourceAddress().isUnspecified()) {
        datagram->trimFront();
        const auto& newHeader = removeNetworkProtocolHeader<UniSphereForwardingHeader>(datagram);
        newHeader->setSourceAddress(destIE->getProtocolData<NextHopInterfaceData>()->getAddress());
        insertNetworkProtocolHeader(datagram, Protocol::nextHopForwarding, newHeader);
        header = newHeader;
    }

    // default: send datagram to fragmentation
    EV_INFO << "output interface is " << destIE->getInterfaceName() << ", next-hop address: " << nextHop << "\n";
    numForwarded++;

    sendDatagramToOutput(datagram, destIE, nextHop);
}

void UniSphereForwarding::encapsulate(Packet *transportPacket, const NetworkInterface *& destIE)
{
    auto header = makeShared<UniSphereForwardingHeader>();
    header->setChunkLength(B(par("headerLength")));
    auto& l3AddressReq = transportPacket->removeTag<L3AddressReq>();
    L3Address src = l3AddressReq->getSrcAddress();
    L3Address dest = l3AddressReq->getDestAddress();
    auto& locReq = transportPacket->removeTagIfPresent<LocatorReq>();

    header->setProtocol(transportPacket->getTag<PacketProtocolTag>()->getProtocol());

    auto& hopLimitReq = transportPacket->removeTagIfPresent<HopLimitReq>();
    short ttl = (hopLimitReq != nullptr) ? hopLimitReq->getHopLimit() : -1;

    // set source and destination address
    header->setDestinationAddress(dest);
    if (locReq) {
        Locator destLoc = locReq->getDestLoc();
        ASSERT(dest == destLoc.getFinalDestination());
        header->setPath(destLoc.getPath());
    }

    // multicast interface option, but allow interface selection for unicast packets as well
    const auto& ifTag = transportPacket->findTag<InterfaceReq>();
    destIE = ifTag ? interfaceTable->getInterfaceById(ifTag->getInterfaceId()) : nullptr;

    // when source address was given, use it; otherwise it'll get the address
    // of the outgoing interface after routing
    if (!src.isUnspecified()) {
        // if interface parameter does not match existing interface, do not send datagram
        if (routingTable->getInterfaceByAddress(src) == nullptr)
            throw cRuntimeError("Wrong source address %s in (%s)%s: no interface with such address",
                    src.str().c_str(), transportPacket->getClassName(), transportPacket->getFullName());
        header->setSourceAddress(src);
    }

    // set other fields
    if (ttl != -1) {
        ASSERT(ttl > 0);
    }
    else if (false) // TODO datagram->getDestinationAddress().isLinkLocalMulticast())
        ttl = 1;
    else
        ttl = defaultHopLimit;

    header->setHopLimit(ttl);

    // setting NextHopForwarding options is currently not supported

    delete transportPacket->removeControlInfo();
    header->setPayloadLengthField(transportPacket->getDataLength());

    insertNetworkProtocolHeader(transportPacket, Protocol::nextHopForwarding, header);
}

UniSphereForwarding::UniSphereForwarding() {
    // TODO Auto-generated constructor stub

}

UniSphereForwarding::~UniSphereForwarding() {
    // TODO Auto-generated destructor stub
}
