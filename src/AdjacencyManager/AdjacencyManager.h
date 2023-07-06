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

#ifndef ADJACENCYMANAGER_ADJACENCYMANAGER_H_
#define ADJACENCYMANAGER_ADJACENCYMANAGER_H_

#include <omnetpp.h>

#include "inet/networklayer/contract/INetfilter.h"
#include "inet/common/ModuleRefByPar.h"
#include "inet/networklayer/contract/IRoutingTable.h"

#include <queue>

using namespace omnetpp;

class AdjacencyManager: public omnetpp::cSimpleModule, public cListener {
  private:
//    virtual bool cmp(cModule* left, cModule* right);
    // typedef bool (*cmp)(cModule* const& left, cModule* const& right) const;
//    typedef std::priority_queue<cModule*, std::vector<cModule*>, decltype(&AdjacencyManager::cmp)> SortedDistanceList;
    typedef std::vector<cModule*> SortedDistanceList;

  protected:
    // parameters
    cModule *host = nullptr; // containing host module (@networkNode)
    inet::ModuleRefByPar<inet::IRoutingTable> irt; // routing table to update
    inet::ModuleRefByPar<inet::IInterfaceTable> ift;

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual bool connectNode(cModule*, inet::NetworkInterface * iface);
    virtual void disconnectNode(cModule*);

//    virtual void addNeighbourRoute() override;
    virtual SortedDistanceList getAPsInRangeSorted();
    virtual bool isWirelessAPAndInRange(cModule *module);
    virtual inet::units::values::m getDistance(cModule *other);

    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

  public:
    static const simsignal_t newNeighbourConnectedSignal;
    static const simsignal_t oldNeighbourDisconnectedSignal;

    AdjacencyManager();
    virtual ~AdjacencyManager();

  private:
    template<typename T>
    void print(T const& q) {
        for (auto const& n : q)
            EV_WARN << n << ' ';
    }
};

#endif /* ADJACENCYMANAGER_ADJACENCYMANAGER_H_ */
