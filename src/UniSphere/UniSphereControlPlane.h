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

#ifndef UNISPHERE_UNISPHERECONTROLPLANE_H_
#define UNISPHERE_UNISPHERECONTROLPLANE_H_

#include <omnetpp.h>
#include "inet/common/ModuleRefByPar.h"
#include "inet/common/Protocol.h"
#include "inet/common/ProtocolGroup.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/routing/base/RoutingProtocolBase.h"
#include "inet/networklayer/contract/IRoutingTable.h"
#include "inet/networklayer/nexthop/NextHopRoute.h"

#include "UniSphereRoute.h"

using namespace omnetpp;

class UniSphereControlPlane: public inet::RoutingProtocolBase, protected omnetpp::cListener {
  private:
    /**
     * Current vicinity descriptor.
     */
    struct CurrentVicinity {
      /// Current vicinity size
      size_t size;
      /// Routing entry with the largest hop count within the vicinity
      /// (retract candidate in case of overflows)
      inet::NextHopRoute* maxHopEntry;
      /// Iterator pointing to the maxHopEntry
//      RoutingInformationBase::index<RIBTags::Vicinity>::type::iterator maxHopIterator;
    };


  public:
    UniSphereControlPlane();
    virtual ~UniSphereControlPlane();

  protected:
    // 200 should be available, see 'networklayer/common/IpProtocolId.msg'
    static const int protocolId = 200;
    static const inet::Protocol *unisphere;

    // state
    cMessage *selfMsg = nullptr;
    // parameters
    cModule *host = nullptr;
    inet::ModuleRefByPar<inet::IRoutingTable> irt;
    inet::ModuleRefByPar<inet::IInterfaceTable> ift;

    cGate *peerIn = nullptr;
    cGate *peerOut = nullptr;
    const simtime_t interval_announce = 30;

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override {}
    virtual void announceOurselves();
    virtual void processPacket(inet::Packet *pkt);
    virtual bool importRoute(UniSphereRoute *route);
    size_t getMaximumVicinitySize() const;
    CurrentVicinity getCurrentVicinity() const;
    virtual void sendToNeighbour(inet::L3Address neigbour, inet::Ptr<PathAnnounce> payload); // == ribExportQueueAnnounce in U-Sphere

    inet::NetworkInterface *getSourceInterfaceFrom(inet::Packet *packet) {
        const auto& interfaceInd = packet->findTag<inet::InterfaceInd>();
        return interfaceInd != nullptr ? ift->getInterfaceById(interfaceInd->getInterfaceId()) : nullptr;
    }


    // lifecycle
    virtual void handleStartOperation(inet::LifecycleOperation *operation) override;
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override { cancelEvent(selfMsg); }
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override { cancelEvent(selfMsg); }

};

#endif /* UNISPHERE_UNISPHERECONTROLPLANE_H_ */
