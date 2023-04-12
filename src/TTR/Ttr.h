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

#ifndef __PROTOTYPE_TTR_H_
#define __PROTOTYPE_TTR_H_

#include <omnetpp.h>

#include "inet/networklayer/contract/INetfilter.h"
#include "inet/common/ModuleRefByPar.h"

using namespace omnetpp;

/**
 * Transient Triangular Routing Class
 * It handles
 */
class Ttr : public cSimpleModule, public inet::NetfilterBase::HookBase
{
  protected:
    inet::ModuleRefByPar<inet::INetfilter> networkProtocol;
    struct ttrEntry {
        inet::L3Address newLoc;
        bool active;
        simtime_t TTL;
        cMessage* destructMsg;
    };

    std::map<inet::L3Address, ttrEntry> ttrEntries;
    // TODO unordered_map might be interesting (O(1)), but L3Address needs to implement a hash
    // Will need to `friend std::hash<L3Address>;` or use parsim (un)packing??
//    template<> struct std::hash<inet::L3Address> {
//        std::size_t operator()(inet::L3Address const& s) const noexcept {
//            return std::hash<int>{}(2); //s.<getNumber>
//        }
//    };

  protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

  public:
    Ttr() {};
    virtual ~Ttr();
    virtual void addTTREntry(inet::L3Address newLoc, inet::L3Address oldLoc, simtime_t TTL = SIMTIME_ZERO);
    virtual void activateEntry(inet::L3Address oldLoc);
    virtual Result datagramPreRoutingHook(inet::Packet *datagram) override;
    virtual Result datagramForwardHook(inet::Packet *datagram) override {return ACCEPT;};
    virtual Result datagramPostRoutingHook(inet::Packet *datagram) override {return ACCEPT;};
    virtual Result datagramLocalInHook(inet::Packet *datagram) override {return ACCEPT;};
    virtual Result datagramLocalOutHook(inet::Packet *datagram) override {return ACCEPT;};
};

#endif
