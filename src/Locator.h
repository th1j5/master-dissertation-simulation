/*
 * Locator.h
 *
 *  Created on: Jun 29, 2023
 *      Author: thijs
 */

#ifndef LOCATOR_H_
#define LOCATOR_H_

#include "inet/networklayer/common/L3Tools.h"

#include "UniSphere/RoutingPath_m.h"
#include "util.h"

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
    bool isUnspecified() {
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


class Locator {
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
    Locator() {
        if (isUniSphere()) {
            locatorType = Tag::UNISPHERE;
            new(&uniSphereLocator) UniSphereLocator();
        }
        else {
            locatorType = Tag::IPV4;
            new(&ipv4Locator) inet::Ipv4Address();
        }
    }
    Locator(const Locator& o):
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
    ~Locator() {
        switch (locatorType) {
            case Tag::UNISPHERE:
                uniSphereLocator.~UniSphereLocator();
                break;
            case Tag::IPV4:
                ipv4Locator.~L3Address();
                break;
        }
    }
    Locator& operator=(const Locator& o) {
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
    bool isUnspecified() {
        switch (locatorType) {
            case Tag::UNISPHERE:
                return uniSphereLocator.isUnspecified();
                break;
            case Tag::IPV4:
                return ipv4Locator.isUnspecified();
                break;
        }
    }
    inet::L3Address getFinalDestination() {
        switch (locatorType) {
            case Tag::UNISPHERE:
                return uniSphereLocator.ID;
                break;
            case Tag::IPV4:
                return ipv4Locator;
                break;
        }
    }
    RoutingPath getPath() {
        switch (locatorType) {
            case Tag::UNISPHERE:
                return uniSphereLocator.path;
                break;
            case Tag::IPV4:
                throw cRuntimeError("Not supported");
                break;
        }
    }
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
