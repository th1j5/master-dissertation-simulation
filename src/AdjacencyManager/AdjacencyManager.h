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

#ifndef __PROTOTYPE_ADJACENCYMANAGER_H_
#define __PROTOTYPE_ADJACENCYMANAGER_H_

#include <omnetpp.h>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/linklayer/common/MacAddress.h"
#include "inet/networklayer/common/InterfaceTable.h"
#include "inet/networklayer/ipv4/Ipv4RoutingTable.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

#include "AdjMgmtMessage_m.h"

using namespace omnetpp;

/**
 * Adjacency Manager (AdjMgmt)
 *
 * This simulator component does 3 things
 * - FLM (Flow Liveness Monitoring):
 *   It checks which flow/neighbour is closest
 * - Adjacency mgmt
 *   It decides to connect to the closest neighbour with policy A
 *   - policy A: ask new Loc (effectively a new node in the graph)
 *   - policy B: only add adjacency, might change Loc
 *   The old Loc is remembered and reachable as long as packets with the old Loc reach the receiver.
 * - Enrolls
 *   It asks a new Loc (= enrollment)
 *
 * It does so pretty stateless, sending each 0.Xs a getLoc message (quick&dirty implementation)
 *
 * Locator Updater Client
 *
 * Broadcasts every "getLocInterval" a packet to ask for it's Locator.
 * The Locator Updater Server will answer with the Loc.
 * The first answer wins, and becomes/remains the Loc.
 * Whenever the Loc changes, the interface table is updated (and signals are activated)
 */
class AdjacencyManager : public inet::ApplicationBase, public cListener, public inet::UdpSocket::ICallback
{
  protected:
    enum SelfMsgKinds { SEND = 1 }; // TODO: enough?

    // parameters
    int serverPort = -1;
    int clientPort = -1;
    inet::UdpSocket socket; // UDP socket for client-server communication
    simtime_t startTime; // application start time
    inet::MacAddress macAddress; // client's MAC address
    cModule *host = nullptr; // containing host module (@networkNode)
    inet::NetworkInterface *ie = nullptr; // interface to configure
    inet::ModuleRefByPar<inet::IIpv4RoutingTable> irt; // routing table to update

    // statistics
    int numSent = 0; // number of sent DHCP messages
    int numReceived = 0; // number of received DHCP messages
    int responseTimeout = 0; // timeout waiting for DHCPACKs, DHCPOFFERs

  protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
//    const char *getAndCheckMessageTypeName();
//    virtual void refreshDisplay() const override;

    virtual void openSocket() = 0;                                  // Opens a UDP socket for client-server communication.
    virtual void handleSelfMessages(cMessage *msg) = 0;
    virtual void handleAdjMgmtMessage(inet::Packet *packet) = 0;

    virtual void sendToUdp(inet::Packet *msg, int srcPort, const inet::L3Address& destAddr, int destPort);
//    virtual inet::NetworkInterface *chooseInterface();
    virtual inet::NetworkInterface *chooseInterface(const char *interfaceName = nullptr);

    // UdpSocket::ICallback methods
    virtual void socketDataArrived(inet::UdpSocket *socket, inet::Packet *packet) override;
    virtual void socketErrorArrived(inet::UdpSocket *socket, inet::Indication *indication) override;
    virtual void socketClosed(inet::UdpSocket *socket) override;
    // Lifecycle methods
    virtual void handleStartOperation(inet::LifecycleOperation *operation) override;
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override;
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override;

  public:
    AdjacencyManager() {}
    virtual ~AdjacencyManager() {}
};

#endif
