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

static inet::L3Address getHostID(cModule* host) {
    // FIXME problematic code
    auto* peerRt = dynamic_cast<inet::IRoutingTable*>(host->findModuleByPath(".generic.routingTable"));
    // get neighbour ID (when unisphere)
    inet::L3Address peerID = peerRt->getRouterIdAsGeneric();
    ASSERT(!peerID.isUnspecified());
    return peerID;
}

#endif /* UTIL_H_ */
