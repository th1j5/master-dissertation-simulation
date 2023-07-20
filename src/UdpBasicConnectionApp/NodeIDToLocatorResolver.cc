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

#include "NodeIDToLocatorResolver.h"

#include "UniSphere/UniSphereControlPlane.h"
#include "util.h"

#include "inet/networklayer/ipv4/IIpv4RoutingTable.h"

using namespace inet;

bool NodeIDToLocatorResolver::tryResolve(const char *s, Locator& result, int addrType) {
    // empty address
//    result = Locator();
//    if (isUniSphere())
//        result.uniSphereLocator = UniSphereLocator();
//    else
//        result.ipv4Locator = L3Address();

    if (!s || !*s)
        return true;

    // handle address literal
    inet::L3Address a;
    if (!isUniSphere() && tryParse(a, s, addrType)) {
        throw cRuntimeError("Didn't expect succesful parsing");
        return true;
    }

    // must be " modulename [ { '%' interfacename | '>' destnode } ] [ '(' protocol ')' ] [ '/' ] " syntax
    // interfacename: existing_interface_of_module | 'routerId'
    // protocol: 'ipv4' | 'ipv6' | 'mac' | 'modulepath' | 'moduleid'
    // '/': returns mask instead address
    std::string modname, ifname, protocol, destnodename;
    bool netmask = addrType & inet::L3AddressResolver::ADDR_MASK;
    const char *p = s;
    const char *endp = strchr(p, '\0');
    const char *nextsep = strpbrk(p, "%>(/");
    if (!nextsep)
        nextsep = endp;
    modname.assign(p, nextsep - p);

    char c = *nextsep;

    if (c == '%') {
        {
            p = nextsep + 1;
            nextsep = strpbrk(p, "(/");
            if (!nextsep)
                nextsep = endp;
        }
        ifname.assign(p, nextsep - p);
        c = *nextsep;
    }
    else if (c == '>') {
        {
            p = nextsep + 1;
            nextsep = strpbrk(p, "(/");
            if (!nextsep)
                nextsep = endp;
        }
        destnodename.assign(p, nextsep - p);
        c = *nextsep;
    }

    if (c == '(') {
        {
            p = nextsep + 1;
            nextsep = strpbrk(p, ")");
            if (!nextsep)
                nextsep = endp;
        }
        protocol.assign(p, nextsep - p);
        c = *nextsep;
        if (c == ')') {
            {
                p = nextsep + 1;
                nextsep = p;
            }
            c = *nextsep;
        }
    }

    if (c == '/') {
        netmask = true;
        {
            p = nextsep + 1;
            nextsep = p;
        }
        c = *nextsep;
    }

    if (c)
        throw cRuntimeError("L3AddressResolver: syntax error parsing address spec `%s'", s);

    // find module
    cModule *mod = cSimulation::getActiveSimulation()->findModuleByPath(modname.c_str());
    if (!mod)
        throw cRuntimeError("L3AddressResolver: module `%s' not found", modname.c_str());

    // check protocol
    if (!protocol.empty()) {
        if (protocol == "ipv4")
            addrType = NodeIDToLocatorResolver::ADDR_IPv4;
        else if (protocol == "ipv6")
            addrType = NodeIDToLocatorResolver::ADDR_IPv6;
        else if (protocol == "mac")
            addrType = NodeIDToLocatorResolver::ADDR_MAC;
        else if (protocol == "modulepath")
            addrType = NodeIDToLocatorResolver::ADDR_MODULEPATH;
        else if (protocol == "moduleid")
            addrType = NodeIDToLocatorResolver::ADDR_MODULEID;
        else
            throw cRuntimeError("L3AddressResolver: error parsing address spec `%s': address type must be `(ipv4)' or `(ipv6)'", s);
    }
    if (netmask)
        addrType |= NodeIDToLocatorResolver::ADDR_MASK;

    // find interface for dest node
    // get address from the given module/interface
    if (!destnodename.empty()) {
        cModule *destnode = cSimulation::getActiveSimulation()->findModuleByPath(destnodename.c_str());
        if (!destnode)
            throw cRuntimeError("L3AddressResolver: destination module `%s' not found", destnodename.c_str());
        throw cRuntimeError("Unsupported");
    }
    else if (ifname.empty())
        addressOf(mod, result, addrType);
    else if (ifname == "routerId")
        routerIdOf(mod, result); // addrType is meaningless here, routerId is protocol independent
    else
        addressOf(mod, ifname.c_str(), result, addrType);
    return !result.isUnspecified();
}

void NodeIDToLocatorResolver::routerIdOf(cModule *host, Locator& result) {
    if (isUniSphere()) {
        UniSphereControlPlane* usphere = check_and_cast_nullable<UniSphereControlPlane*>(host->getSubmodule("unisphere"));
        ASSERT(usphere);
//        result.uniSphereLocator = usphere->getLocator();
        result = Locator(usphere->getLocator());
    }
    else
//        result.ipv4Locator = L3AddressResolver::routerIdOf(host);
        result = Locator(L3AddressResolver::routerIdOf(host));
}
void NodeIDToLocatorResolver::addressOf(cModule *host, Locator& result, int addrType) {
    if (isUniSphere()) {
        UniSphereControlPlane* usphere = check_and_cast_nullable<UniSphereControlPlane*>(host->getSubmodule("unisphere"));
        ASSERT(usphere);
//        result.uniSphereLocator = usphere->getLocator();
        result = Locator(usphere->getLocator());
    }
    else
//        result.ipv4Locator = L3AddressResolver::addressOf(host, addrType);
        result = Locator(L3AddressResolver::addressOf(host, addrType));
}
void NodeIDToLocatorResolver::addressOf(cModule *host, const char *ifname, Locator& result, int addrType) {
    throw cRuntimeError("Not implemented");
}

cModule* NodeIDToLocatorResolver::findHostWithAddress(const L3Address& addr) {
    // first try normal resolver (for IP case where the interfaces are addressed (p.-) )
    cModule* mod = L3AddressResolver::findHostWithAddress(addr);
    if (mod)
        return mod;

    // now try the ID->mod resolver
    if (addr.isUnspecified() || addr.isMulticast())
        return nullptr;

    auto networkNodes = collectNetworkNodes();
    for (cModule *mod : networkNodes) {
        IIpv4RoutingTable *rtable = getIpv4RoutingTableOf(mod);
        if(rtable->getRouterIdAsGeneric() == addr)
            return mod;
    }
    return nullptr;
}

NodeIDToLocatorResolver::NodeIDToLocatorResolver() {
    // TODO Auto-generated constructor stub

}

NodeIDToLocatorResolver::~NodeIDToLocatorResolver() {
    // TODO Auto-generated destructor stub
}

