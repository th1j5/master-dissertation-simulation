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

#ifndef UNISPHERE_UNISPHEREROUTE_H_
#define UNISPHERE_UNISPHEREROUTE_H_

#include <omnetpp.h>

#include "inet/networklayer/nexthop/NextHopRoute.h"

#include "RoutingPath_m.h"
#include "PathAnnounce_m.h"

using namespace omnetpp;

class UniSphereRoute: public inet::NextHopRoute {
  private:
    bool landmark = false;
  public:
    /**
     * Unclear if this is useful
     * Can be used for differentiating between routes only used as placeholders for neighbours and ...
     * The only reason to keep non-active routes around is when a retraction happens
     */
    bool active = false; // U-Sphere semantics, not too much used here
    bool vicinity = false;
    /* For each *MN*, the RIB is increased with each added peer (to uphold the division between DTPMs and their RIB)
     * This is the easiest way (right now) to split the tables.
     */
    int RIB = 0; // to which DTPM/RIB does this belong?
    uint32_t seqno = 0;
    RoutingPath forwardPath;
    RoutingPath reversePath;
    virtual void setLandmark(bool isLandmark) { this->landmark = isLandmark; }
    virtual bool isLandmark() { return landmark; }

    virtual inet::Ptr<PathAnnounce> exportEntry();

    UniSphereRoute() {}
    UniSphereRoute(inet::Ptr<const PathAnnounce> pkt);
    UniSphereRoute(inet::L3Address neighbour);
    virtual ~UniSphereRoute() {}
    virtual std::string str() const override;
};

static std::ostream& operator<<(std::ostream& os, const UniSphereRoute& e) {
    // https://github.com/omnetpp/omnetpp/issues/1000
    os << "LOLLLLLLLL";
    return os;
};

#endif /* UNISPHERE_UNISPHEREROUTE_H_ */
