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

Define_Module(UniSphereControlPlane);

UniSphereControlPlane::UniSphereControlPlane() {
    // TODO Auto-generated constructor stub

}

UniSphereControlPlane::~UniSphereControlPlane() {
    // TODO Auto-generated destructor stub
}

void UniSphereControlPlane::announceOurselves() {
    // TODO: everything under this has to be reviewed...
    // Announce ourselves to all neighbours and send them routing updates
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
}
