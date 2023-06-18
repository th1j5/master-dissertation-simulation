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

#include "UniSphereControlPlane.h"
#include "../util.h"

#include "inet/common/ProtocolTag_m.h"
#include "inet/common/ProtocolGroup.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/HopLimitTag_m.h"

using namespace inet; // more OK to use in .cc
Define_Module(UniSphereControlPlane);

const inet::Protocol *UniSphereControlPlane::unisphere = new Protocol("unisphere", "U-Sphere");

UniSphereControlPlane::UniSphereControlPlane() {
    // TODO Auto-generated constructor stub

}

UniSphereControlPlane::~UniSphereControlPlane() {
    cancelAndDelete(selfMsg);
}

void UniSphereControlPlane::initialize(int stage) {
    RoutingProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        selfMsg = new cMessage("announceTimer");
        // get the routing table to update and subscribe it to the blackboard
        irt.reference(this, "routingTableModule", true);
//        ift.reference(this, "interfaceTableModule", true);
        peerIn = gate("networkLayerIn");
        peerOut = gate("networkLayerOut");
        if (!ProtocolGroup::getIpProtocolGroup()->findProtocol(protocolId)) { // one-shot execution
            ProtocolGroup::getIpProtocolGroup()->addProtocol(protocolId, unisphere);
        }
    }
    else if (stage == INITSTAGE_ROUTING_PROTOCOLS) {
        registerProtocol(*unisphere, gate("networkLayerOut"), gate("networkLayerIn"));
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        // get the hostname
//        host = getContainingNode(this);
    }
}

void UniSphereControlPlane::handleMessageWhenUp(cMessage *msg) {
    if (msg->isSelfMessage()) {
        // handle self message (announce timer)
        announceOurselves();
    }
    else if (msg->arrivedOn("networkLayerIn")) {
        // handle incoming message
        Packet *pkt = check_and_cast<Packet *>(msg);
        auto protocol = pkt->getTag<PacketProtocolTag>()->getProtocol();
        if (protocol == unisphere) {
            //TODO
//            processPacket(packet);
            EV_WARN << "Received unisphere packet from network: " << msg->getName() << " (" << msg->getClassName() << ")" << endl;
            delete msg;
        }
        else
            throw cRuntimeError("U-SphereControlPlane: received unknown packet '%s (%s)' from the network layer.", msg->getName(), msg->getClassName());
    }
    else
        throw cRuntimeError("Unknown message origin.");
}

void UniSphereControlPlane::announceOurselves() {
    short ttl = 1;
    // Announce ourselves to all neighbours and send them routing updates
    for (auto peer: getConnectedNodes(irt)) {
        //
        //    check_and_cast<Packet *>(msg)->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
        Packet *pkt = new Packet();

        // TODO - see routing/pim/modes/PimSM.cc/sendToIP
        pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(unisphere);
        pkt->addTagIfAbsent<DispatchProtocolInd>()->setProtocol(unisphere);
        pkt->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::nextHopForwarding);
        pkt->addTagIfAbsent<L3AddressReq>()->setDestAddress(getHostID(peer));
        pkt->addTagIfAbsent<HopLimitReq>()->setHopLimit(ttl);
        send(pkt, peerOut);
    }
    scheduleAfter(interval_announce, selfMsg);

/* TODO: everything under this has to be reviewed...
    Protocol::PathAnnounce announce;
    for (const std::pair<NodeIdentifier, PeerPtr> &peer : m_identity.peers()) {
        PeerPtr peerInfo = peer.second;
        // Get a security association for this link to setup delegations
        PeerSecurityAssociationPtr sa = peerInfo->selectPeerSecurityAssociation(m_context);
        if (!sa) {
            // TODO: Rate limit transmission of SA_Flush
            m_manager.send(peerInfo->contact(),
              Message(Message::Type::Social_SA_Flush, Protocol::SecurityAssociationFlush()));
            m_statistics.saUpdateXmits++;
            continue;
        }

        announce.Clear();
        announce.set_public_key(m_identity.localKey().raw());
        announce.set_landmark(m_routes.isLandmark());
        if (m_routes.isLandmark()) {
            // Get/assign the outgoing vport for this announcement
            Vport vport = m_routes.getVportForNeighbor(peer.first);
            announce.add_reverse_path(vport);
        }

        // Construct the delegation message
        Protocol::PathDelegation delegation;
        delegation.set_delegation(sa->raw());
        // TODO: Include reverse vport in signature
        announce.add_delegation_chain(m_identity.localKey().privateSignSubkey().sign(delegation));

        announce.set_seqno(m_seqno);
        ribExportQueueAnnounce(peerInfo->contact(), announce);

        // Send full routing table to neighbor
        m_routes.fullUpdate(peer.first);
    }

    // Reschedule self announce
    m_announceTimer.expires_from_now(m_context.roughly(CompactRouter::interval_announce));
    m_announceTimer.async_wait(boost::bind(&CompactRouterPrivate::announceOurselves, this, _1));
*/
}

void UniSphereControlPlane::handleStartOperation(LifecycleOperation *operation) {
    simtime_t start = simTime(); // std::max(startTime
    scheduleAt(start, selfMsg);
}
