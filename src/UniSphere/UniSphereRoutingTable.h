/*
 * This file is part of UNISPHERE.
 *
 * Copyright (C) 2014 Jernej Kos <jernej@kos.mx>
 * Copyright (C) 2023 Thijs Paelman <thijs.paelman@ugent.be>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef UNISPHERE_UNISPHEREROUTINGTABLE_H_
#define UNISPHERE_UNISPHEREROUTINGTABLE_H_

#include <omnetpp/csimplemodule.h>

/// A list of routing path delegations
using RoutingPathDelegations = std::list<std::string>;

/**
 * An entry in the compact routing table.
 */
class RoutingEntry {
public:
  /**
   * Constructs a routing entry.
   *
   * @param context UNISPHERE context
   * @param publicKey Originator public key
   * @param landmark Landmark status of the entry
   * @param seqno Sequence number
   */
  RoutingEntry(Context &context,
               const PublicPeerKey &publicKey,
               bool landmark,
               std::uint16_t seqno);

  /**
   * Class destructor.
   */
  ~RoutingEntry();

  /**
   * Returns true if this entry represents a direct route.
   */
  bool isDirect() const { return forwardPath.size() == 1; }

  /**
   * Returns the vport identifier of the first routing hop.
   */
  Vport originVport() const { return forwardPath.front(); }

  /**
   * Returns the length of the forward path.
   */
  size_t hops() const { return forwardPath.size(); }

  /**
   * Returns the age of this routing entry.
   */
  //boost::posix_time::time_duration age() const;
public:
  /// Destination node identifier
  NodeIdentifier destination;
  /// Originator public key
  //PublicPeerKey publicKey;
  /// Delegations
  RoutingPathDelegations delegations;
  /// Last SA public signing key
  std::string saKey;
  /// Path of vports to destination
  RoutingPath forwardPath;
  /// Path of vports from destination (only for landmarks)
  RoutingPath reversePath;
  /// Entry type
  bool landmark;
  /// Vicinity status
  bool vicinity;
  /// Extended vicinity status
  bool extendedVicinity;
  /// Sequence number
  std::uint16_t seqno;
  /// Cost to route to that entry
  std::uint16_t cost;
  /// Active mark
  bool active;
  /// Entry liveness
  //boost::posix_time::ptime lastUpdate;
  /// Expiration timer
  //boost::asio::deadline_timer expiryTimer;
};

inline bool operator==(const RoutingEntry &lhs, const RoutingEntry &rhs)
{
  return lhs.destination == rhs.destination && lhs.landmark == rhs.landmark && lhs.seqno == rhs.seqno &&
         lhs.cost == rhs.cost && lhs.forwardPath == rhs.forwardPath && lhs.reversePath == rhs.reversePath;
}
class UniSphereRoutingTable: public omnetpp::cSimpleModule {
  public:
    /// A helper structure for returning next hops
    struct NextHop {
      /// Next hop identifier
      NodeIdentifier nodeId;
      /// Source-route towards the destination
      RoutingPath path;
    };

    /// A helper structure for returning sloppy group relays
    struct SloppyGroupRelay {
      /// Node identifier of the sloppy group relay node in vicinity
      NodeIdentifier nodeId;
      /// Next hop in route towards the sloppy group relay node
      NodeIdentifier nextHop;
    };

    /// A helper structure for returning vicinity descriptors
    struct VicinityDescriptor {
      /// Destination node identifier
      NodeIdentifier nodeId;
      /// Number of hops to reach the destination
      size_t hops;
    }; 

     /**
     * Class constructor.
     *
     * @param context UNISPHERE context
     * @param localId Local node identifier
     * @param sizeEstimator A network size estimator
     * @param sloppyGroup Sloppy group manager
     */
    UniSphereRoutingTable(Context &context,
                        const NodeIdentifier &localId,
                        NetworkSizeEstimator &sizeEstimator,
                        SloppyGroupManager &sloppyGroup);

    UniSphereRoutingTable(const CompactRoutingTable&) = delete;
    UniSphereRoutingTable &operator=(const CompactRoutingTable&) = delete;
    //UniSphereRoutingTable();
    virtual ~UniSphereRoutingTable();

    /**
     * Returns the currently active route to the given destination
     * based only on local information. If there is no known direct
     * route an invalid entry is returned.
     *
     * @param destination Destination address
     * @return Next hop metadata
     */
    NextHop getActiveRoute(const NodeIdentifier &destination);

    /**
     * Returns the routing entry that can be used as a relay when
     * the destination identifier can't be resolved locally.
     *
     * @param destination Destiantion address
     * @return Descriptor for the sloppy group relay
     */
    SloppyGroupRelay getSloppyGroupRelay(const NodeIdentifier &destination);

    /**
     * Returns a vport identifier corresponding to the given neighbor
     * identifier. If a vport has not yet been assigned, a new one is
     * assigned on the fly.
     *
     * @param neighbor Neighbor node identifier
     * @return Vport identifier
     */
    Vport getVportForNeighbor(const NodeIdentifier &neighbor);

    /**
     * Returns the neighbor identifier corresponding to the given
     * vport identifier. If this vport has not been assigned, a
     * null node identifier is returned.
     *
     * @param vport Vport identifier
     * @return Neighbor node identifier
     */
    NodeIdentifier getNeighborForVport(Vport vport) const;
    /**
     * Exports the full routing table to the specified peer.
     *
     * @param peer Peer to export the routing table to
     */
    void fullUpdate(const NodeIdentifier &peer);

    /**
     * Returns the number of all records that are being stored in the routing
     * table.
     */
    size_t size() const;

    /**
     * Returns the number of active routing entries.
     */
    size_t sizeActive() const;

    /**
     * Returns the number of routing entries from node's vicinity.
     */
    size_t sizeVicinity() const;

    /**
     * Returns a list of vicinity descriptors for the node's vicinity.
     */
    std::list<VicinityDescriptor> getVicinity() const;

    /**
     * Clears the whole routing table (RIB and vport mappings).
     */
    void clear();

    /**
     * Sets the landmark status of the local node.
     *
     * @param landmark True for the local node to become a landmark
     */
    void setLandmark(bool landmark);

    /**
     * Returns true if the local node is currently a landmark for other
     * nodes.
     */
    bool isLandmark() const;

    /**
     * Returns a list of landmark-relative local addresses.
     *
     * @return A list of landmark addresses
     */
    LandmarkAddressList getLocalAddresses() const;

    /**
     * Returns the first local address.
     */
    LandmarkAddress getLocalAddress() const;
};

#endif /* UNISPHERE_UNISPHEREROUTINGTABLE_H_ */
