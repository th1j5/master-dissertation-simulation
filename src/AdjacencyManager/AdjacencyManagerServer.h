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

#ifndef ADJACENCYMANAGER_ADJACENCYMANAGERSERVER_H_
#define ADJACENCYMANAGER_ADJACENCYMANAGERSERVER_H_

#include <map>
#include "AdjacencyManager.h"

class AdjacencyManagerServer: public AdjacencyManager {
  private:
    template<typename K, typename V, typename _C, typename Tv, typename = typename std::enable_if<std::is_convertible<Tv, V>::value>::type>
    inline bool containsValue(const std::map<K,V,_C>& m, const Tv& a) {
        for (const auto& [key, value] : m) {
            if (value == a)
                return true;
        }
        return false;
    }

  protected:
    typedef std::map<inet::MacAddress, inet::L3Address> LocLeased;
    LocLeased leased;

    unsigned int maxNumOfClients = 0;
    inet::Ipv4Address subnetMask;
    inet::Ipv4Address ipAddressStart;

  protected:
    virtual void initialize(int stage) override;

    virtual void handleSelfMessages(cMessage *msg) override;
    virtual void handleAdjMgmtMessage(inet::Packet *packet) override;
    virtual void openSocket() override;

    virtual inet::L3Address assignLoc(inet::MacAddress clientID);
    virtual inet::L3Address* getLocByID(inet::MacAddress clientID);
//    virtual void sendAssignLocPacket(int seqNum, inet::L3Address assignedLoc);
    virtual void sendAssignLocPacket(const inet::Ptr<const AdjMgmtMessage> msg, inet::L3Address assignedLoc);

    // Lifecycle methods
    virtual void handleStartOperation(inet::LifecycleOperation *operation) override;
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override;
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override;

  public:
    AdjacencyManagerServer() {}
    virtual ~AdjacencyManagerServer();
};

#endif /* ADJACENCYMANAGER_ADJACENCYMANAGERSERVER_H_ */
