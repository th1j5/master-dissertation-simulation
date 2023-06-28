/*
 * UniSphereRoute.cc
 *
 *  Created on: Jun 24, 2023
 *      Author: thijs
 */

#include "UniSphereRoute.h"

#include "../util.h"

using namespace inet; // more OK to use in .cc

UniSphereRoute::UniSphereRoute(Ptr<const PathAnnounce> ctrlMessage) {
//    auto pathSize = ctrlMessage->getForward_pathArraySize();
    auto pathSize = ctrlMessage->getForward_path().size();

    setDestination(ctrlMessage->getOrigin());

    // FIXME: correct?
    forwardPath = ctrlMessage->getForward_path();
    reversePath = ctrlMessage->getReverse_path(); // only landmarks

//    L3Address vport = ctrlMessage->getForward_path(pathSize-1); // neighbour who send it
    L3Address vport = forwardPath.front(); // neighbour who send it
    setNextHop(vport);
//    route->setMetric(pathSize-1);
    setMetric(forwardPath.size()-1);
    setLandmark(ctrlMessage->getLandmark());
}

UniSphereRoute::UniSphereRoute(L3Address neighbour) {
    // Install an incomplete route, used for getConnectedNodes (i.e. neighbour detection)
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
    RoutingPath prependedForwardPath = RoutingPath(forwardPath);
    prependedForwardPath.push_front(getHostID(host));

    payload->setForward_path(prependedForwardPath); //FIXME (check that @byValue made a copy)
    payload->setReverse_path(reversePath); // same
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

    out << NextHopRoute::str();

    return out.str();
}
