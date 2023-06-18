/*
 * util.h
 *
 *  Created on: Jun 18, 2023
 *      Author: thijs
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <omnetpp.h>
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

static inet::L3Address getHostID(cModule* host) {
    // FIXME problematic code
    auto* peerRt = dynamic_cast<inet::IRoutingTable*>(host->findModuleByPath(".generic.routingTable"));
    // get neighbour ID (when unisphere)
    inet::L3Address peerID = peerRt->getRouterIdAsGeneric();
    ASSERT(!peerID.isUnspecified());
    return peerID;
}

namespace {
static bool isNeighbourRoute(const inet::IRoute *entry) {
    if (entry->getMetric() == 0) { // neighbour
        ASSERT(entry->getDestinationAsGeneric() == entry->getNextHopAsGeneric());
        return true;
    }
    else
        return false;
}
}

static std::vector<cModule*> getConnectedNodes(inet::ModuleRefByPar<inet::IRoutingTable> irt) {
    std::vector<cModule*> nodes;
    for (int i=0; i<irt->getNumRoutes(); i++) {
        inet::IRoute *e = irt->getRoute(i);
        if(isNeighbourRoute(e))
            nodes.push_back(inet::L3AddressResolver().findHostWithAddress(e->getDestinationAsGeneric()));
    }
    return nodes;
}

#endif /* UTIL_H_ */
