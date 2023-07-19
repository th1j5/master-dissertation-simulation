/*
 * Locator.h
 *
 *  Created on: Jun 29, 2023
 *      Author: thijs
 */

#ifndef LOCATOR_H_
#define LOCATOR_H_

#include <omnetpp.h>
#include "inet/networklayer/common/L3Tools.h"

#include "UniSphere/RoutingPath_m.h"

using namespace omnetpp;

struct UniSphereLocator {
    inet::L3Address ID = inet::ModulePathAddress(); // any type different from ::NONE
    RoutingPath path; // landmark part of the path
    // This gave me a hell of a lot of problems, but why? Interesting... (runtime crashes)
    // Why: destructor crashes...
//    UniSphereLocator& operator=(UniSphereLocator other) noexcept {
//        std::swap(ID, other.ID);
//        std::swap(path, other.path);
//        return *this;
//    }
    bool isUnspecified() const {
        return ID.isUnspecified();
    }
//    UniSphereLocator() {
//        ID = inet::L3Address();
//        path = RoutingPath();
//    }
//    ~UniSphereLocator() {
//        ID.~L3Address();
//        path.~RoutingPath();
//    }
    UniSphereLocator& operator=(const UniSphereLocator& other) {
        ID = other.ID;
        path = other.path;
        return *this;
    }
};

class Locator: public cObject {
  private:
    enum class Tag { UNISPHERE, IPV4 } locatorType;
    union {
        inet::L3Address ipv4Locator;
        UniSphereLocator uniSphereLocator;
    };

  public:
//    Locator() = delete;
    Locator(UniSphereLocator loc):
        locatorType(Tag::UNISPHERE), uniSphereLocator(loc) {}
    Locator(inet::L3Address loc):
        locatorType(Tag::IPV4), ipv4Locator(loc) {}
    Locator();
    Locator(const Locator& o);
    ~Locator();
    Locator& operator=(const Locator& o);
    bool isUnspecified() const;
    inet::L3Address getFinalDestination() const;
    RoutingPath getPath();
};

/*
union Locator {
    inet::L3Address ipv4Locator;
    UniSphereLocator uniSphereLocator;
    Locator() {
//        memset( this, 0, sizeof( Locator ) );
        if (isUniSphere())
            new(&uniSphereLocator) UniSphereLocator();
//            uniSphereLocator = UniSphereLocator();
        else
            new(&ipv4Locator) inet::L3Address();
//            ipv4Locator = inet::L3Address();
    }
    Locator(const Locator& loc) {
        if (isUniSphere())
            uniSphereLocator = loc.uniSphereLocator;
        else
            ipv4Locator = loc.ipv4Locator;
    }
    Locator& operator=(const Locator& other) {
        // copy-and-swap idiom
//        if (isUniSphere())
//            std::swap(uniSphereLocator, other.uniSphereLocator);
//        else
//            std::swap(ipv4Locator, other.ipv4Locator);
        //
        if (isUniSphere()) {
            uniSphereLocator = other.uniSphereLocator;
        }
        else {
            ipv4Locator = other.ipv4Locator;
        }
        return *this;
    }
    ~Locator() {
        if (isUniSphere())
            uniSphereLocator.~UniSphereLocator();
        else
            ipv4Locator.~L3Address();
    }
    bool isUnspecified() {
        if (isUniSphere())
            return uniSphereLocator.isUnspecified();
        else
            return ipv4Locator.isUnspecified();
    }
};
*/

inline std::ostream& operator<<(std::ostream& os, const UniSphereLocator& loc) {
    os << "ID:" << loc.ID << "path:" << loc.path;
    return os;
}

#endif /* LOCATOR_H_ */
