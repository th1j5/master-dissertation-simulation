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

#include "inet/networklayer/contract/NetworkHeaderBase_m.h"
#include "inet/networklayer/common/L3Tools.h"
#include "inet/transportlayer/udp/UdpHeader_m.h"

#include "MultiplexerPacket_m.h"
#include "util.h"
#include "UniSphere/UniSphereForwardingHeader_m.h"

using namespace inet; // more OK to use in .cc

Define_Module(Ttr);

Ttr::~Ttr() {
    for (const auto& e: ttrEntries) {
        cancelAndDelete(e.second.destructMsg);
    }
}

void Ttr::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        networkProtocol.reference(this, "networkProtocolModule", true);
        networkProtocol->registerHook(0, this);
    }
    else if (stage == INITSTAGE_NETWORK_LAYER) {
//        auto text = std::to_string(natEntries.size()) + " entries";
//        getDisplayString().setTagArg("t", 0, text.c_str());
    }
}

std::function< void(const Ptr< MultiplexerPacket > &)> f_increaseReroute = increaseReroute;
INetfilter::IHook::Result Ttr::datagramPreRoutingHook(Packet *datagram) {
    //put logic to reroute in flight here
//    pk->peekAtFront<Ipv4Header>().; // TODO optimise (peek for address, only remove when needed)
    auto& networkHeader = removeNetworkProtocolHeader<NetworkHeaderBase>(datagram);
    L3Address oldDestName = networkHeader->getDestinationAddress();
    auto rerouteEntry = ttrEntries.find(oldDestName);
    if (rerouteEntry != ttrEntries.end() && rerouteEntry->second.active) {
        if (isUniSphere()) {
            dynamicPtrCast<UniSphereForwardingHeader>(networkHeader)->setPath(rerouteEntry->second.newLoc.getPath());
        }
        else {
            networkHeader->setDestinationAddress(rerouteEntry->second.newLoc.getFinalDestination());
        }
        // reroute++
        auto& header = datagram->peekAtFront<UdpHeader>();
        b offset = header->getChunkLength(); // start from the beginning
        datagram->updateDataAt<MultiplexerPacket>(f_increaseReroute, offset);
    }
    if (isUniSphere()) {
        insertNetworkProtocolHeader(datagram, Protocol::nextHopForwarding, networkHeader);
    } else {
        insertNetworkProtocolHeader(datagram, Protocol::ipv4, networkHeader);
    }

    return ACCEPT;
}

void Ttr::addTTREntry(Locator newLoc, L3Address oldDestName, simtime_t TTL) {
    Enter_Method_Silent("Add a TTR Entry");
    cMessage* msg = new cMessage("remove TTR entry");
    scheduleAfter(TTL, msg);
    if (isUniSphere()) {
        if (oldDestName != newLoc.getFinalDestination())
            throw cRuntimeError("The TTR entry key and value don't correspond for U-Sphere");
    }
    bool succes = ttrEntries.insert({oldDestName, {newLoc, false, TTL, msg}}).second;
    if (!succes) {
        cancelAndDelete(msg);
        throw cRuntimeError("The old locator %s was already inserted in the TTR table for some reason...", oldDestName.str().c_str());
    }
}
void Ttr::activateEntry(L3Address oldDestName) {
    auto entry = ttrEntries.find(oldDestName);
    if (entry != ttrEntries.end()) {
        if (!entry->second.active)
            entry->second.active = true;
        else
            throw cRuntimeError("TTR entry with oldDestName %s was already active?!", oldDestName.str().c_str());
    }
    else {
        EV_WARN << "TTR entry with oldDestName %s was not found while trying to activate" << endl;
        EV_WARN << "This means that the TTL expired BEFORE the link was severed" << endl;
//        throw cRuntimeError("TTR entry with oldDestName %s not found!", oldDestName.str().c_str());
    }
}

void Ttr::handleMessage(cMessage *msg) {
    ASSERT(msg->isSelfMessage());
    /* Clean up TTR table */
    auto itr = ttrEntries.begin();
    while (itr != ttrEntries.end() && msg != itr->second.destructMsg) {
        ++itr;
    }
    if (itr == ttrEntries.end())
        throw cRuntimeError("Wanted to destroy TTR entry, but it wasn't found");
    else {
        ttrEntries.erase(itr);
        delete msg;
    }
}
// TODO: Do we need a timer?? see mechanism 3
//const auto& msg = pk->peekAtFront<LocatorUpdatePacket>();
