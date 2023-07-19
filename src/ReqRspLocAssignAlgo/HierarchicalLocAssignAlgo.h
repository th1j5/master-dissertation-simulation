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

#ifndef REQRSPLOCASSIGNALGO_HIERARCHICALLOCASSIGNALGO_H_
#define REQRSPLOCASSIGNALGO_HIERARCHICALLOCASSIGNALGO_H_

#include <omnetpp.h>
#include "inet/common/ModuleRefByPar.h"
#include "inet/common/Protocol.h"
#include "inet/common/ProtocolGroup.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/routing/base/RoutingProtocolBase.h"
#include "inet/networklayer/contract/IRoutingTable.h"
#include "inet/networklayer/ipv4/IIpv4RoutingTable.h"

#include "Locator_m.h"
#include "ReqRspLocMessage_m.h"
#include "LocUpdatable/LocUpdatable.h"
#include "AdjacencyManager/AdjacencyManager.h"

using namespace omnetpp;

class HierarchicalLocAssignAlgo: public inet::RoutingProtocolBase, protected omnetpp::cListener, public LocUpdatable {
  public:
    HierarchicalLocAssignAlgo();
    virtual ~HierarchicalLocAssignAlgo();

  protected:
    // server
    // ID -> Loc
    typedef std::map<inet::L3Address, inet::L3Address> LocLeased;
    LocLeased leased;
    int maxNumOfClients = 0;
    inet::Ipv4Address iface;
    inet::Ipv4Address subnetMask;
    inet::Ipv4Address ipAddressStart;

    //client
    int seqRcvd = -1; // latest received message
    struct {
        inet::L3Address loc{};
        inet::Ipv4Address netmask{};
        inet::L3Address neigh{}; // neighbours of the old Locator (at the moment just 1)
    } oldLocData;

    // 201 should be available, see 'networklayer/common/IpProtocolId.msg'
    static const int protocolId = 201;
    const inet::Protocol *hierLocAssignAlgo;

    // parameters
    inet::ModuleRefByPar<inet::IIpv4RoutingTable> irt;
    cGate *peerIn = nullptr;
    cGate *peerOut = nullptr;
    bool client = false;
    bool server = false;

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

    // client & server
    virtual void sendToNeighbour(inet::L3Address neighbour, inet::Ptr<ReqRspLocMessage> payload);
    inet::NetworkInterface* chooseInterface(const char *interfaceName = nullptr);

    // client
    virtual inet::Ptr<ReqRspLocMessage> createLocReqPayload();
    virtual void handleLocRspMessage(inet::Packet *packet);
    virtual bool isFilteredMessageClient(const inet::Ptr<const ReqRspLocMessage> & msg);
    virtual bool updateLocator(Locator const& loc, const inet::Ipv4Address & subnetMask);
    virtual void removeOldLocClient();
    virtual void fixDynamicRoutesClient(const inet::Ptr<const ReqRspLocMessage> & piggybackMsg);

    // server
    virtual inet::Ptr<ReqRspLocMessage> createLocRspPayload(const inet::Ptr<const ReqRspLocMessage> req);
    virtual void handleLocReqMessage(inet::Packet *packet);
    virtual bool isFilteredMessageServer(inet::Packet *packet);
    //FIXME: change next functions
    virtual inet::L3Address assignLoc(inet::L3Address clientID);
    virtual inet::L3Address* getLocByID(inet::L3Address clientID);

    // lifecycle
    virtual void handleStartOperation(inet::LifecycleOperation *operation) override;
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override;
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override;

  private:
    //server
    template<typename K, typename V, typename _C, typename Tv, typename = typename std::enable_if<std::is_convertible<Tv, V>::value>::type>
    inline bool containsValue(const std::map<K,V,_C>& m, const Tv& a) {
        for (const auto& [key, value] : m) {
            if (value == a)
                return true;
        }
        return false;
    }
};

#endif /* REQRSPLOCASSIGNALGO_HIERARCHICALLOCASSIGNALGO_H_ */
