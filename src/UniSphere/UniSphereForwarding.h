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

#ifndef UNISPHERE_UNISPHEREFORWARDING_H_
#define UNISPHERE_UNISPHEREFORWARDING_H_

#include <omnetpp.h>
#include "inet/networklayer/nexthop/NextHopForwarding.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/contract/IInterfaceTable.h"

using namespace omnetpp;

class UniSphereForwarding: public inet::NextHopForwarding {
  public:
    virtual void routePacket(inet::Packet *datagram, const inet::NetworkInterface *destIE, const inet::L3Address& nextHop, bool fromHL) override;
    virtual void encapsulate(inet::Packet *transportPacket, const inet::NetworkInterface *& destIE) override;
//    virtual void decapsulate(Packet *datagram) override;

    UniSphereForwarding();
    virtual ~UniSphereForwarding();
};

#endif /* UNISPHERE_UNISPHEREFORWARDING_H_ */
