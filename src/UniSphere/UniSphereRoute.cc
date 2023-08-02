/*
 * UniSphereRoute.cc
 *
 *  Created on: Jun 24, 2023
 *      Author: thijs
 */

#include "UniSphereRoute.h"

#include "util.h"

using namespace inet; // more OK to use in .cc

UniSphereRoute::UniSphereRoute(Ptr<const PathAnnounce> ctrlMessage) {
    setDestination(ctrlMessage->getOrigin());

    // FIXME: correct?
    forwardPath = ctrlMessage->getForward_path();
    reversePath = ctrlMessage->getReverse_path(); // only landmarks

    L3Address vport = forwardPath.top(); // neighbour who send it
    setNextHop(vport);
    setMetric(forwardPath.size()-1);
    setLandmark(ctrlMessage->getLandmark());
}

UniSphereRoute::UniSphereRoute(L3Address neighbour) {
    // Install an incomplete route, used for getConnectedNeigh (i.e. neighbour detection)
    // Don't make it active, because we don't know some import properties
    setDestination(neighbour);
    setNextHop(neighbour);
    setMetric(0);
    ASSERT(!active);
}

/**
 * Returns a PathAnnounce payload for this entry
 * Caller should still check if this need to send to a certain neighbour or not
 * AND add neighbour specific information if needed
 */
Ptr<PathAnnounce> UniSphereRoute::exportEntry() {
    auto payload = makeShared<PathAnnounce>();
    payload->setOrigin(getDestinationAsGeneric()); // == set_public_key in U-Sphere
    payload->setLandmark(landmark);
    payload->setSeqno(seqno); // FIXME

    //FIXME: check that paths are correctly constructed
    // add ourselves to forward path
    cModule *host = getContainingNode(check_and_cast<cModule*>(getRoutingTableAsGeneric()));
    RoutingPath appendedForwardPath = RoutingPath(forwardPath);
    appendedForwardPath.push(getHostID(host));

    payload->setForward_path(appendedForwardPath); //FIXME (check that @byValue made a copy)
    // reverse path == inverse(forwardPath) (in our simplified implementation)
    payload->setReverse_path(reversePath);
    // no vport needed in reverse path, due to our adaptations to use IDs instead

    payload->setChunkLength(B(10)); //FIXME

    return payload;
}

std::string UniSphereRoute::str() const {
    std::stringstream out;

    out << (landmark ? "L" : "..");
    out << (active   ? "A" : "..");
    out << (vicinity ? "V" : "..") << " ";
    out << "seq:" << seqno << " ";
    out << "RIB:" << RIB << " ";
    out << "R:" << reversePath << " ";
    out << "F:" << forwardPath << " ";

    out << NextHopRoute::str();

    return out.str();
}
