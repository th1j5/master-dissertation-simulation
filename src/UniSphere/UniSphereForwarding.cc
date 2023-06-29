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

#include "inet/networklayer/common/L3Tools.h"
#include "inet/networklayer/nexthop/NextHopInterfaceData.h"

using namespace inet; // more OK to use in .cc
Define_Module(UniSphereForwarding);

void UniSphereForwarding::routePacket(Packet *datagram, const NetworkInterface *destIE, const L3Address& requestedNextHop, bool fromHL)
{
    // TODO add option handling code here

    auto header = datagram->peekAtFront<NextHopForwardingHeader>();
    L3Address destAddr = header->getDestinationAddress();

    EV_INFO << "Routing datagram `" << datagram->getName() << "' with dest=" << destAddr << ": ";

    // check for local delivery
    if (routingTable->isLocalAddress(destAddr)) {
        EV_INFO << "local delivery\n";
        if (fromHL && header->getSourceAddress().isUnspecified()) {
            datagram->trimFront();
            const auto& newHeader = removeNetworkProtocolHeader<NextHopForwardingHeader>(datagram);
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

        // error handling: destination address does not exist in routing table:
        // throw packet away and continue
        if (re == nullptr) {
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
        const auto& newHeader = removeNetworkProtocolHeader<NextHopForwardingHeader>(datagram);
        newHeader->setHopLimit(header->getHopLimit() - 1);
        insertNetworkProtocolHeader(datagram, Protocol::nextHopForwarding, newHeader);
        header = newHeader;
    }

    // set datagram source address if not yet set
    if (header->getSourceAddress().isUnspecified()) {
        datagram->trimFront();
        const auto& newHeader = removeNetworkProtocolHeader<NextHopForwardingHeader>(datagram);
        newHeader->setSourceAddress(destIE->getProtocolData<NextHopInterfaceData>()->getAddress());
        insertNetworkProtocolHeader(datagram, Protocol::nextHopForwarding, newHeader);
        header = newHeader;
    }

    // default: send datagram to fragmentation
    EV_INFO << "output interface is " << destIE->getInterfaceName() << ", next-hop address: " << nextHop << "\n";
    numForwarded++;

    sendDatagramToOutput(datagram, destIE, nextHop);
}

UniSphereForwarding::UniSphereForwarding() {
    // TODO Auto-generated constructor stub

}

UniSphereForwarding::~UniSphereForwarding() {
    // TODO Auto-generated destructor stub
}
