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


class AdjacencyManagerClient: public AdjacencyManager {
  public:
    static simsignal_t newLocAssignedSignal;
  protected:
    inet::ModuleRefByPar<inet::IArp> arp;
    inet::NetworkInterface *ieOld = nullptr; // interface to configure

    //parameters
    const char * const locUpdateName = "neighLocUpdate";
    // statistics
    int numLocUpdates = -1; // First send is 0
    int numLocUpdateSend = 0;

    // state
    int seqSend = 0;
    int seqRcvd = -1;
    struct {
        inet::L3Address loc{};
        inet::Ipv4Address netmask{};
        inet::L3Address neigh{}; // neighbours of the old Locator (at the moment just 1)
    } oldLocData;
    struct ip_netmask {
        inet::Ipv4Address ip;
        inet::Ipv4Address netmask;
    };

  protected:
    virtual void initialize(int stage) override;

    virtual void handleSelfMessages(cMessage *msg) override;
    virtual void handleAdjMgmtMessage(inet::Packet *packet) override;
    virtual void handleNeighMessage(inet::Packet *pk) override {}; // don't participate in neigh messages
    virtual void openSocket() override;

    virtual bool isFilteredMessage(const inet::Ptr<const AdjMgmtMessage> & msg);
    virtual void processGetLoc();
    virtual void sendGetLocPacket();
    virtual bool isLocReachable(inet::NetworkInterface* ie1);
    virtual void assignNewLoc(ip_netmask ip_net);
    virtual void removeOldLoc();
    virtual inet::Ipv4Address getGateway(inet::NetworkInterface* ie);
    virtual void sendNeighLocUpdate(inet::L3Address newLoc, inet::L3Address oldLoc, inet::L3Address neigh);

    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

  public:
    // FIXME: encoding ints mostly works, but not necessarily (https://stackoverflow.com/questions/10749419/encode-multiple-ints-into-a-double)
    double getCorrID(int numLocUpdates) {return (((int64_t)host->getId())<<32) | ((int64_t)numLocUpdates);};
    int getNumLocUpdates() {return numLocUpdates;};
    inet::NetworkInterface* getIeNew() { return ie;};
    inet::NetworkInterface* getIeOld() { return ieOld;};
};

#endif /* ADJACENCYMANAGER_ADJACENCYMANAGERCLIENT_H_ */
