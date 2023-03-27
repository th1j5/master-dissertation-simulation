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

#ifndef ADJACENCYMANAGER_ADJACENCYMANAGERCLIENT_H_
#define ADJACENCYMANAGER_ADJACENCYMANAGERCLIENT_H_

#include "AdjacencyManager.h"
#include "inet/networklayer/contract/IArp.h"

class AdjacencyManagerClient: public AdjacencyManager {
protected:
    inet::ModuleRefByPar<inet::IArp> arp;

    // state
    cMessage *selfMsg = nullptr;
    int seqSend = 0;
    int seqRcvd = -1;

  protected:
    virtual void initialize(int stage) override;

    virtual void handleSelfMessages(cMessage *msg) override;
    virtual void handleAdjMgmtMessage(inet::Packet *packet) override;
    virtual void openSocket() override;

    virtual void processGetLoc();
    virtual void sendGetLocPacket();
    virtual bool checkReachabilityOldLoc();
    virtual inet::Ipv4Address getGateway(inet::NetworkInterface* ie);

    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

    // Lifecycle methods
    virtual void handleStartOperation(inet::LifecycleOperation *operation) override;
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override;
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override;

  public:
    AdjacencyManagerClient() {}
    virtual ~AdjacencyManagerClient();
};

#endif /* ADJACENCYMANAGER_ADJACENCYMANAGERCLIENT_H_ */
