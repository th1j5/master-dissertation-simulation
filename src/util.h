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
#include "inet/networklayer/ipv4/Ipv4Route.h"

#include "UdpBasicConnectionApp/NodeIDToLocatorResolver.h"
#include "UniSphere/UniSphereRoute.h"

static inet::L3Address getHostID(cModule* host) {
    // FIXME problematic code
    auto* peerRt = dynamic_cast<inet::IRoutingTable*>(host->findModuleByPath(".generic.routingTable"));
    if (peerRt == nullptr)
        peerRt = dynamic_cast<inet::IRoutingTable*>(host->findModuleByPath(".ipv4.routingTable"));
    ASSERT(peerRt != nullptr);
    // get neighbour ID (when unisphere)
    inet::L3Address peerID = peerRt->getRouterIdAsGeneric();
    ASSERT(!peerID.isUnspecified());
    return peerID;
}
static bool isUniSphere() {
    static enum {
        UNKNOWN,
        IPV4,
        UNISPHERE
    } simulationType = { UNKNOWN };

    // enable/disable check for each run or not...
    if (true || simulationType == UNKNOWN) {
        cObject *mod = cSimulation::getActiveSimulation()->findObject("unisphere");
        if (mod)
            simulationType = UNISPHERE;
        else
            simulationType = IPV4;
    }
    switch (simulationType) {
        case IPV4:
            return false;
            break;
        case UNISPHERE:
            return true;
            break;
        default:
            throw cRuntimeError("SimulationType unknown or unsupported");
            break;
    }
}

namespace {
static bool isNeighbourRoute(const inet::IRoute *entry) {
    if (isUniSphere()) {
        if (entry->getMetric() == 0) { // neighbour
            ASSERT(entry->getDestinationAsGeneric() == entry->getNextHopAsGeneric());
            return true;
        }
        else
            return false;
    }
    else {
        auto *e = check_and_cast<const inet::Ipv4Route*>(entry);
        // only selects 'special' neigh routes (inserted by us, not the Ipv4NetworkConfigurator - probably wireless)
        return e->getMetric() == 0
                && !e->getDestinationAsGeneric().isUnspecified()
                && e->getPrefixLength() == 32;
                //&& entry->getNextHopAsGeneric() == Ipv4;
    }
}
static bool isUnitializedNeighbour(const UniSphereRoute *entry) {
    // Is this an entry inserted by the adjManager or not?
    EV_WARN << entry->getFullPath() << endl;
    EV_WARN << entry->getMetric() << endl;
    return isNeighbourRoute(entry)
            && !entry->active
            && !entry->vicinity
            && entry->seqno == 0
            && entry->forwardPath.size() == 0
            && entry->reversePath.size() == 0;
}
}

static std::vector<cModule*> getConnectedNeigh(inet::ModuleRefByPar<inet::IRoutingTable> irt) {
    std::vector<cModule*> nodes;
    for (int i=0; i<irt->getNumRoutes(); i++) {
        inet::IRoute *e = irt->getRoute(i);
        if(isNeighbourRoute(e)) {
            nodes.push_back(NodeIDToLocatorResolver().findHostWithAddress(e->getDestinationAsGeneric()));
            ASSERT(nodes.back());
        }
    }
    return nodes;
}

// copy from 'inet/common/Topology.cc'
static bool selectByProperty(cModule *mod, void *data)
{
    struct ParamData {
        const char *name;
        const char *value;
    };
    ParamData *d = (ParamData *)data;
    cProperty *prop = mod->getProperties()->get(d->name);
    if (!prop)
        return false;
    const char *value = prop->getValue(cProperty::DEFAULTKEY, 0);
    if (d->value)
        return opp_strcmp(value, d->value) == 0;
    else
        return opp_strcmp(value, "false") != 0;
}

static int getNetworkSize() {
    // Oracle network size estimator (see U-Sphere paper)
    bool (*predicate)(cModule *, void *);
    predicate = selectByProperty;
    struct {
        const char *name;
        const char *value;
    } data = {
        "networkNode", nullptr
    };
    int networkSize = 0;

    for (int modId = 0; modId <= getSimulation()->getLastComponentId(); modId++) {
        cModule *module = getSimulation()->getModule(modId);
        if (module && predicate(module, (void*)&data)) {
            networkSize++;
        }
    }
    EV_DEBUG << "Network estimate: " << networkSize << endl;
    return networkSize;
}

// https://stackoverflow.com/a/51217509
template <typename It>
struct iterator_pair {
    It b, e;

    It begin() const { return b; }
    It end() const { return e; }
};

// Drop last element of vector/... when iterating over it
// Used to enforce policy on the amount of connections kept by a MN
template <typename Range>
static auto drop_last(Range& r)
    -> iterator_pair<decltype(r.begin())>
{
    return {r.begin(), r.begin() == r.end() ? r.end() : std::prev(r.end())};
}

#endif /* UTIL_H_ */
