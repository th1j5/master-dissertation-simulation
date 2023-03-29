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

#include "Ttr.h"

#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/common/L3Tools.h"

using namespace inet; // more OK to use in .cc

Define_Module(Ttr);

Ttr::~Ttr() {
    // FIXME delete table here
}

void Ttr::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        networkProtocol.reference(this, "networkProtocolModule", true);
    }
    else if (stage == INITSTAGE_NETWORK_LAYER) {
//        auto text = std::to_string(natEntries.size()) + " entries";
//        getDisplayString().setTagArg("t", 0, text.c_str());
    }
}

INetfilter::IHook::Result Ttr::datagramPreRoutingHook(Packet *datagram) {
    //put logic to reroute in flight here
//    pk->peekAtFront<Ipv4Header>().; // TODO optimise (peek for address, only remove when needed)
    auto& ipv4Header = removeNetworkProtocolHeader<Ipv4Header>(datagram);
    auto oldLoc = ipv4Header->getDestinationAddress();
    auto rerouteEntry = ttrEntries.find(oldLoc);
    if (rerouteEntry != ttrEntries.end()) {
        ipv4Header->setDestinationAddress(rerouteEntry->second);
        // FIXME: make sure to recognize re-routed packets
    }
    insertNetworkProtocolHeader(datagram, Protocol::ipv4, ipv4Header);

    return ACCEPT;
}

void Ttr::addTTREntry(L3Address newLoc, L3Address oldLoc) {
    if (ttrEntries.size() == 0) // Register only 1 time, when needed. Most efficient check.
        networkProtocol->registerHook(0, this);
    bool succes = ttrEntries.insert({oldLoc, newLoc}).second;
    if (!succes)
        throw cRuntimeError("The old locator %s was already inserted in the TTR table for some reason...", oldLoc.str().c_str());
}
// TODO: Do we need a timer?? see mechanism 3
//const auto& msg = pk->peekAtFront<LocatorUpdatePacket>();
