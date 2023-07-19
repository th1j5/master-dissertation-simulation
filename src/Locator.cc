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

#include "Locator.h"

#include "util.h"

using namespace inet; // more OK to use in .cc

Register_Class(Locator);

Locator::Locator() {
    if (isUniSphere()) {
        locatorType = Tag::UNISPHERE;
        new(&uniSphereLocator) UniSphereLocator();
    }
    else {
        locatorType = Tag::IPV4;
        new(&ipv4Locator) inet::Ipv4Address();
    }
}
Locator::Locator(const Locator& o):
    locatorType(o.locatorType)
{
    switch (locatorType) {
        case Tag::UNISPHERE:
            new(&uniSphereLocator) UniSphereLocator();
            uniSphereLocator = o.uniSphereLocator;
            break;
        case Tag::IPV4:
            new(&ipv4Locator) inet::L3Address();
            ipv4Locator = o.ipv4Locator;
            break;
    }
}
Locator::~Locator() {
    switch (locatorType) {
        case Tag::UNISPHERE:
            uniSphereLocator.~UniSphereLocator();
            break;
        case Tag::IPV4:
            ipv4Locator.~L3Address();
            break;
    }
}
Locator& Locator::operator=(const Locator& o) {
    if (locatorType != o.locatorType) {
        throw cRuntimeError("not supported");
    }
    switch (o.locatorType) {
        case Tag::UNISPHERE:
            uniSphereLocator = o.uniSphereLocator;
            break;
        case Tag::IPV4:
            ipv4Locator = o.ipv4Locator;
            break;
    }
    return *this;
}
bool Locator::isUnspecified() const {
    switch (locatorType) {
        case Tag::UNISPHERE:
            return uniSphereLocator.isUnspecified();
            break;
        case Tag::IPV4:
            return ipv4Locator.isUnspecified();
            break;
    }
}
inet::L3Address Locator::getFinalDestination() {
    switch (locatorType) {
        case Tag::UNISPHERE:
            return uniSphereLocator.ID;
            break;
        case Tag::IPV4:
            return ipv4Locator;
            break;
    }
}
RoutingPath Locator::getPath() {
    switch (locatorType) {
        case Tag::UNISPHERE:
            return uniSphereLocator.path;
            break;
        case Tag::IPV4:
            throw cRuntimeError("Not supported");
            break;
    }
}
