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

#if false

#include "UniSphereRoutingTable.h"

#include <unordered_map>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/make_shared.hpp>

namespace midx = boost::multi_index;

/// RIB index tags
namespace RIBTags {
  class DestinationId;
  class ActiveRoutes;
  class LandmarkCost;
  class Vicinity;
  class VicinityAny;
  class VportDestination;
}

/// Routing information base
using RoutingInformationBase = boost::multi_index_container<
  RoutingEntryPtr,
  midx::indexed_by<
    // Index by destination identifier and order by cost within
    midx::ordered_non_unique<
      midx::tag<RIBTags::DestinationId>,
      midx::composite_key<
        RoutingEntry,
        BOOST_MULTI_INDEX_MEMBER(RoutingEntry, NodeIdentifier, destination),
        BOOST_MULTI_INDEX_MEMBER(RoutingEntry, std::uint16_t, cost)
      >
    >,

    // Indey by activeness and destination identifier
    midx::ordered_non_unique<
      midx::tag<RIBTags::ActiveRoutes>,
      midx::composite_key<
        RoutingEntry,
        BOOST_MULTI_INDEX_MEMBER(RoutingEntry, bool, active),
        BOOST_MULTI_INDEX_MEMBER(RoutingEntry, NodeIdentifier, destination)
      >
    >,

    // Index by landmark status and cost
    midx::ordered_non_unique<
      midx::tag<RIBTags::LandmarkCost>,
      midx::composite_key<
        RoutingEntry,
        BOOST_MULTI_INDEX_MEMBER(RoutingEntry, bool, landmark),
        BOOST_MULTI_INDEX_MEMBER(RoutingEntry, std::uint16_t, cost)
      >
    >,

    // Index by vicinity and extended vicinity
    midx::ordered_non_unique<
      midx::tag<RIBTags::Vicinity>,
      midx::composite_key<
        RoutingEntry,
        BOOST_MULTI_INDEX_MEMBER(RoutingEntry, bool, active),
        BOOST_MULTI_INDEX_MEMBER(RoutingEntry, bool, vicinity),
        BOOST_MULTI_INDEX_MEMBER(RoutingEntry, bool, extendedVicinity),
        midx::const_mem_fun<RoutingEntry, size_t, &RoutingEntry::hops>
      >
    >,

    // Index by vicinity
    midx::ordered_non_unique<
      midx::tag<RIBTags::VicinityAny>,
      midx::composite_key<
        RoutingEntry,
        BOOST_MULTI_INDEX_MEMBER(RoutingEntry, bool, active),
        BOOST_MULTI_INDEX_MEMBER(RoutingEntry, bool, vicinity),
        BOOST_MULTI_INDEX_MEMBER(RoutingEntry, NodeIdentifier, destination),
        midx::const_mem_fun<RoutingEntry, size_t, &RoutingEntry::hops>
      >
    >,

    // Index by origin vport
    midx::ordered_unique<
      midx::tag<RIBTags::VportDestination>,
      midx::composite_key<
        RoutingEntry,
        midx::const_mem_fun<RoutingEntry, Vport, &RoutingEntry::originVport>,
        BOOST_MULTI_INDEX_MEMBER(RoutingEntry, NodeIdentifier, destination)
      >
    >
  >
>;

/// Bidirectional nodeId-vport mapping
using VportMap = boost::bimap<
  boost::bimaps::unordered_set_of<NodeIdentifier>,
  boost::bimaps::unordered_set_of<Vport>
>;

RoutingEntry::RoutingEntry(Context &context,
                           const PublicPeerKey &publicKey,
                           bool landmark,
                           std::uint16_t seqno)
  : destination(publicKey.nodeId()),
    landmark(landmark),
    seqno(seqno),
    active(false),
    expiryTimer(context.service())
{
}

RoutingEntry::~RoutingEntry() {}

UniSphereRoutingTable::UniSphereRoutingTable(Context &context,
                                                       const NodeIdentifier &localId,
                                                       NetworkSizeEstimator &sizeEstimator,
                                                       SloppyGroupManager &sloppyGroup)
  : m_context(context),
    m_localId(localId),
    m_sizeEstimator(sizeEstimator),
    m_sloppyGroup(sloppyGroup),
    m_nextVport(0),
    m_landmark(false)
{
    EV_INFO << "LocalNodeID: " << localId <<endl;
}

Vport UniSphereRoutingTable::getVportForNeighbor(const NodeIdentifier &neighbor)
{
  VportMap::left_const_iterator i = m_vportMap.left.find(neighbor);
  if (i == m_vportMap.left.end()) {
    // No vport has been assigned yet, create a new mapping
    m_vportMap.insert(VportMap::value_type(neighbor, m_nextVport));
    return m_nextVport++;
  }

  return (*i).second;
}

NodeIdentifier UniSphereRoutingTable::getNeighborForVport(Vport vport) const
{
  VportMap::right_const_iterator i = m_vportMap.right.find(vport);
  if (i == m_vportMap.right.end())
    return NodeIdentifier();

  return (*i).second;
}

size_t UniSphereRoutingTable::getMaximumVicinitySize() const
{
  // TODO: This is probably not the best way (int -> double -> sqrt -> int)
  double n = static_cast<double>(m_sizeEstimator.getNetworkSize());
  return static_cast<size_t>(std::sqrt(n * std::log(n)));
}

size_t UniSphereRoutingTable::getMaximumBucketSize() const
{
  // TODO: This is probably not the best way (int -> double -> log -> int)
  double n = static_cast<double>(m_sizeEstimator.getNetworkSize());
  return static_cast<size_t>(std::log(n));
}

UniSphereRoutingTable::CurrentVicinity UniSphereRoutingTable::getCurrentVicinity() const
{
  auto entries = m_rib.get<RIBTags::Vicinity>().equal_range(boost::make_tuple(true, true, false));
  CurrentVicinity vicinity;
  vicinity.size = std::distance(entries.first, entries.second);
  if (vicinity.size > 0) {
    vicinity.maxHopIterator = --entries.second;
    vicinity.maxHopEntry = *vicinity.maxHopIterator;
  }

  return vicinity;
}

UniSphereRoutingTable::SloppyGroupBucket UniSphereRoutingTable::getSloppyGroupBucket(const NodeIdentifier &nodeId) const
{
  NodeIdentifier groupStart = nodeId.prefix(m_sloppyGroup.getGroupPrefixLength());
  NodeIdentifier groupEnd = nodeId.prefix(m_sloppyGroup.getGroupPrefixLength(), 0xFF);

  auto &ribVicinity = m_rib.get<RIBTags::VicinityAny>();
  auto begin = ribVicinity.lower_bound(boost::make_tuple(true, true, groupStart));
  auto end = ribVicinity.upper_bound(boost::make_tuple(true, true, groupEnd));
  SloppyGroupBucket bucket;
  bucket.size = 0;

  for (auto it = begin; it != end; ++it) {
    bucket.size++;

    // Only consider entries in extended vicinity for removal
    if ((*it)->extendedVicinity &&
        (!bucket.maxHopEntry || (*it)->hops() >= bucket.maxHopEntry->hops())) {
      bucket.maxHopEntry = *it;
      bucket.maxHopIterator = it;
    }
  }

  return bucket;
}

void UniSphereRoutingTable::entryTimerExpired(const boost::system::error_code &error,
                                                   RoutingEntryWeakPtr entry)
{
    // FIXME: wrong
  if (error)
    return;

  if (RoutingEntryPtr e = entry.lock()) {
    // Retract the entry from the routing table
    m_statistics.routeExpirations++;
    retract(e->originVport(), e->destination);
  }
}

UniSphereRoutingTable::UniSphereRoutingTable() {
    // TODO Auto-generated constructor stub

}

UniSphereRoutingTable::~UniSphereRoutingTable() {
    // TODO Auto-generated destructor stub
}

#endif
