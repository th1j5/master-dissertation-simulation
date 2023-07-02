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

#ifndef UDPBASICCONNECTIONAPP_NODEIDTOLOCATORRESOLVER_H_
#define UDPBASICCONNECTIONAPP_NODEIDTOLOCATORRESOLVER_H_

#include "inet/networklayer/common/L3AddressResolver.h"

#include "Locator_m.h"

using namespace omnetpp;

 #define DEFAULT_ADDR_TYPE    (NodeIDToLocatorResolver::ADDR_IPv4 | NodeIDToLocatorResolver::ADDR_IPv6 | NodeIDToLocatorResolver::ADDR_MODULEPATH | NodeIDToLocatorResolver::ADDR_MODULEID)

class NodeIDToLocatorResolver: public inet::L3AddressResolver {
  public:
    virtual void routerIdOf(cModule *host, Locator& result);
    virtual void addressOf(cModule *host, Locator& result, int addrType = DEFAULT_ADDR_TYPE);
    virtual void addressOf(cModule *host, const char *ifname, Locator& result, int addrType = DEFAULT_ADDR_TYPE);
    virtual bool tryResolve(const char *str, Locator& result, int addrType = DEFAULT_ADDR_TYPE);
    virtual bool tryResolve(const char *str, inet::L3Address& result, int addrType = DEFAULT_ADDR_TYPE) override { throw cRuntimeError("Unjust usage..."); }

    NodeIDToLocatorResolver();
    virtual ~NodeIDToLocatorResolver();
};

#endif /* UDPBASICCONNECTIONAPP_NODEIDTOLOCATORRESOLVER_H_ */
