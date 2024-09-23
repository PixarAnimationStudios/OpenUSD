//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "pxr/external/boost/python/object/inheritance.hpp"
#include "pxr/external/boost/python/type_id.hpp"
#include <algorithm>
#include <deque>
#include <memory>
#include <queue>
#include <tuple>
#include <vector>

//
// Procedure:
//
//      The search is a BFS over the space of (type,address) pairs
//      guided by the edges of the casting graph whose nodes
//      correspond to classes, and whose edges are traversed by
//      applying associated cast functions to an address. We use
//      vertex distance to the goal node in the cast_graph to rate the
//      paths. The vertex distance to any goal node is calculated on
//      demand and outdated by the addition of edges to the graph.

namespace PXR_BOOST_NAMESPACE {
namespace
{
  template <class T> inline void unused_variable(const T&) { }

  typedef void*(*cast_function)(void*);
  
  //
  // Here we put together the low-level data structures of the
  // casting graph representation.
  //
  typedef python::type_info class_id;

  typedef unsigned int vertex_t;
  typedef unsigned int distance_t;

  struct edge_t
  {
      vertex_t target;
      cast_function cast;
  };

  // represents a graph of available casts
  
  struct cast_graph
  {
      typedef std::vector<edge_t>::const_iterator out_edge_iterator;
      typedef std::pair<out_edge_iterator, out_edge_iterator> out_edges_t;

      typedef std::vector<vertex_t>::const_iterator in_edge_iterator;
      typedef std::pair<in_edge_iterator, in_edge_iterator> in_edges_t;

      std::size_t num_vertices() const
      {
          return m_out_edges.size();
      }

      vertex_t add_vertex()
      {
          vertex_t v = m_out_edges.size();
          m_out_edges.push_back(std::vector<edge_t>());
          m_in_edges.push_back(std::vector<vertex_t>());
          assert(m_out_edges.size() == m_in_edges.size());
          return v;
      }

      void add_edge(vertex_t src, vertex_t target, cast_function cast)
      {
          assert(target < m_in_edges.size());
          edge_t e = { target, cast };
          m_out_edges[src].push_back(e);
          m_in_edges[target].push_back(src);
      }

      bool has_edge(vertex_t src, vertex_t target) const
      {
          assert(src < m_out_edges.size());
          std::vector<edge_t> const& out_edges = m_out_edges[src];

          for (out_edge_iterator p = out_edges.begin()
                   , finish = out_edges.end()
                   ; p != finish
                   ; ++p)
          {
              if (p->target == target) {
                  return true;
              }
          }
          return false;
      }

      out_edges_t out_edges(vertex_t src) const
      {
          assert(src < m_out_edges.size());
          std::vector<edge_t> const& out_edges = m_out_edges[src];
          return out_edges_t(out_edges.begin(), out_edges.end());
      }

      in_edges_t in_edges(vertex_t src) const
      {
          assert(src < m_in_edges.size());
          std::vector<vertex_t> const& in_edges = m_in_edges[src];
          return in_edges_t(in_edges.begin(), in_edges.end());
      }

   private:
      // A pair of adjacency lists to hold the graph of casts as well as its
      // transpose.
      std::vector<std::vector<edge_t> > m_out_edges;
      std::vector<std::vector<vertex_t> > m_in_edges;
  };
  
  // Each distance is stored along with the version of the graph used to
  // compute it.  As the graph expands, the distance can be recomputed in
  // place.
  struct path_distance
  {
      path_distance(vertex_t source, vertex_t target)
          : source(source)
          , target(target)
          , distance(0)
          , version(0)
      {}

      vertex_t source;
      vertex_t target;
      distance_t distance;
      unsigned int version;
  };

  // Sparse storage of cast_graph distances.
  //
  // This is a very simple implementation of a chaining hash map whose number
  // of buckets is always a power-of-two and load factor is one.  It supports
  // only insertion and lookup and implements a rudimentary node pool via
  // std::deque.
  class all_pairs_distance_map
  {
      BOOST_STATIC_CONSTANT(std::size_t, initial_bucket_count = 16);

      struct node
          : path_distance
      {
          node(vertex_t source, vertex_t target, node *next)
              : path_distance(source, target)
              , next(next)
          {}

          node *next;
      };
      
   public:
      all_pairs_distance_map()
          : m_buckets(std::make_unique<node*[]>(initial_bucket_count))
          , m_size(0)
          , m_mask(initial_bucket_count-1)
      {
      }

      path_distance const* find(vertex_t source, vertex_t target) const
      {
          std::size_t idx = compute_hash(source, target) & m_mask;
          node *nd = m_buckets[idx];
          for (; nd; nd = nd->next) {
              if (nd->source == source && nd->target == target) {
                  return nd;
              }
          }
          return 0;
      }

      path_distance* insert(vertex_t source, vertex_t target)
      {
          std::size_t h = compute_hash(source, target);
          {
              std::size_t idx = h & m_mask;
              node *nd = m_buckets[idx];
              for (; nd; nd = nd->next) {
                  if (nd->source == source && nd->target == target) {
                      return nd;
                  }
              }
          }

          if (m_size == bucket_count()) {
              rehash();
          }
          ++m_size;

          std::size_t idx = h & m_mask;
          m_node_pool.push_back(node(source, target, m_buckets[idx]));
          node *nd = &m_node_pool.back();
          m_buckets[idx] = nd;
          return nd;
      }

   private:
      std::size_t bucket_count() const
      {
          return m_mask + 1;
      }

      void rehash()
      {
          std::size_t old_bucket_count = bucket_count();
          std::size_t new_bucket_count = 2*old_bucket_count;
          std::size_t new_mask = new_bucket_count - 1;
          auto new_buckets = std::make_unique<node*[]>(new_bucket_count);

          for (std::size_t i=0; i<old_bucket_count; ++i) {
              node *nd = m_buckets[i];
              while (nd) {
                  node *next = nd->next;
                  size_t new_idx = compute_hash(nd->source, nd->target) & new_mask;

                  nd->next = new_buckets[new_idx];
                  new_buckets[new_idx] = nd;
                  nd = next;
              }
          }

          m_buckets.swap(new_buckets);
          m_mask = new_mask;
      }

      static std::size_t compute_hash(vertex_t source, vertex_t target)
      {
          // This function previously used boost::hash and boost::hash_combine.
          // The implementations of those functions have been recreated here
          // to avoid depending on boost directly.
          auto hash = [](vertex_t t) { return static_cast<std::size_t>(t); };
          auto hash_combine = [](std::size_t& seed, std::size_t value) {
              seed ^= value + 0x9e3779b9 + (seed<<6) + (seed>>2);
          };

          std::size_t h = hash(source);
          hash_combine(h, hash(target));
          return h;
      }

      std::unique_ptr<node*[]> m_buckets;
      std::size_t m_size;
      std::size_t m_mask;
      std::deque<node> m_node_pool;
  };

  // For a pair of vertices, (outer_key, inner_key), node_distance_map
  // provides a submap view over the inner_keys for a given outer_key.
  class node_distance_map
  {
   public:
      node_distance_map(all_pairs_distance_map &full_map, vertex_t outer_key, unsigned int version)
          : m_all_pairs_map(full_map)
          , m_outer_key(outer_key)
          , m_version(version)
      {}

      bool is_initialized() const
      {
          // Check the version of the identity entry (if it exists) to see if
          // this row needs (re-)initialization.
          path_distance const *i = m_all_pairs_map.find(m_outer_key, m_outer_key);
          assert(!i || i->distance == 0);
          return i && i->version == m_version;
      }

      distance_t distance(vertex_t inner_key) const
      {
          path_distance const *i = m_all_pairs_map.find(m_outer_key, inner_key);
          return (i && i->version == m_version)
              ? i->distance : (std::numeric_limits<distance_t>::max)();
      }

      // Returns true if a new entry was inserted or an older version was
      // replaced.
      bool set_distance(vertex_t inner_key, distance_t value)
      {
          path_distance *i = m_all_pairs_map.insert(m_outer_key, inner_key);
          if (i->version != m_version) {
              i->distance = value;
              i->version = m_version;
              return true;
          }
          return false;
      }

   private:
      all_pairs_distance_map &m_all_pairs_map;
      vertex_t m_outer_key;
      unsigned int m_version;
  };

  struct smart_graph
  {
      // Return a map of the distances from any node to the given
      // target node
      node_distance_map distances_to(vertex_t target) const
      {
          unsigned int version = m_topology.num_vertices();

          node_distance_map to_target(m_distances, target, version);

          // this node hasn't been used as a target yet
          if (!to_target.is_initialized())
          {
              typedef std::pair<vertex_t, distance_t> node_distance;
              std::queue<node_distance> q;

              q.push(node_distance(target, 0));
              while (!q.empty())
              {
                  node_distance top = q.front();
                  q.pop();

                  vertex_t v = top.first;
                  distance_t dist = top.second;
                  if (!to_target.set_distance(v, dist)) {
                      continue;
                  }

                  cast_graph::in_edges_t in_edges = m_topology.in_edges(v);
                  for (cast_graph::in_edge_iterator p = in_edges.first
                           , finish = in_edges.second
                           ; p != finish
                           ; ++p
                      )
                  {
                      q.push(node_distance(*p, dist + 1));
                  }
              }
          }

          return to_target;
      }

      cast_graph& topology() { return m_topology; }
      cast_graph const& topology() const { return m_topology; }
      
   private:
      cast_graph m_topology;
      mutable all_pairs_distance_map m_distances;
  };
  
  smart_graph& full_graph()
  {
      static smart_graph x;
      return x;
  }
  
  smart_graph& up_graph()
  {
      static smart_graph x;
      return x;
  }

  //
  // Our index of class types
  //
  using PXR_BOOST_NAMESPACE::python::objects::dynamic_id_function;
  struct index_entry {
      index_entry(class_id src_static_type
                  , vertex_t vertex)
          : src_static_type(src_static_type)
          , vertex(vertex)
          , dynamic_id(0)
      {}

      class_id src_static_type;       // static type
      vertex_t vertex;                // corresponding vertex 
      dynamic_id_function dynamic_id; // dynamic_id if polymorphic, or 0
  };
  typedef std::vector<index_entry> type_index_t;

  
  type_index_t& type_index()
  {
      static type_index_t x;
      return x;
  }

  inline bool index_entry_static_type_less_than(index_entry const& entry, class_id type)
  {
      return entry.src_static_type < type;
  }

  // map a type to a position in the index
  inline type_index_t::iterator type_position(class_id type)
  {
      type_index_t &ti = type_index();
      
      return std::lower_bound(
          ti.begin(), ti.end(), type,
          index_entry_static_type_less_than);
  }

  inline index_entry* seek_type(class_id type)
  {
      type_index_t::iterator p = type_position(type);
      if (p == type_index().end() || p->src_static_type != type)
          return 0;
      else
          return &*p;
  }
  
  // Get the entry for a type, inserting if necessary
  inline type_index_t::iterator demand_type(class_id type)
  {
      type_index_t::iterator p = type_position(type);

      if (p != type_index().end() && p->src_static_type == type)
          return p;

      vertex_t v = full_graph().topology().add_vertex();
      vertex_t v2 = up_graph().topology().add_vertex();
      unused_variable(v2);
      assert(v == v2);
      return type_index().insert(p, index_entry(type, v));
  }

  // Map a two types to a vertex in the graph, inserting if necessary
  typedef std::pair<type_index_t::iterator, type_index_t::iterator>
        type_index_iterator_pair;
  
  inline type_index_iterator_pair
  demand_types(class_id t1, class_id t2)
  {
      // be sure there will be no reallocation between the first and second
      // call to demand_type (but don't thwart geometric growth)
      type_index_t &ti = type_index();
      type_index_t::size_type new_size = ti.size() + 2;
      if (new_size > ti.capacity()) {
          ti.reserve(new_size + new_size/2);
      }
      type_index_t::iterator first = demand_type(t1);
      type_index_t::iterator second = demand_type(t2);
      if (first == second)
          ++first;
      return std::make_pair(first, second);
  }

  struct q_elt
  {
      q_elt(distance_t distance
            , void* src_address
            , vertex_t target
            , cast_function cast
            )
          : distance(distance)
          , src_address(src_address)
          , target(target)
          , cast(cast)
      {}
      
      distance_t distance;
      void* src_address;
      vertex_t target;
      cast_function cast;

      bool operator<(q_elt const& rhs) const
      {
          return distance < rhs.distance;
      }
  };

  // Optimization:
  //
  // Given p, src_t, dst_t
  //
  // Get a pointer pd to the most-derived object
  //    if it's polymorphic, dynamic_cast to void*
  //    otherwise pd = p
  //
  // Get the most-derived typeid src_td
  //
  // ptrdiff_t offset = p - pd
  //
  // Now we can keep a cache, for [src_t, offset, src_td, dst_t] of
  // the cast transformation function to use on p and the next src_t
  // in the chain.  src_td, dst_t don't change throughout this
  // process. In order to represent unreachability, when a pair is
  // found to be unreachable, we stick a 0-returning "dead-cast"
  // function in the cache.
  
  // This is needed in a few places below
  inline void* identity_cast(void* p)
  {
      return p;
  }

  void* search(smart_graph const& g, void* p, vertex_t src, vertex_t dst)
  {
      node_distance_map d(g.distances_to(dst));

      // If we know there's no path; bail now.
      distance_t const unreachable = (std::numeric_limits<distance_t>::max)();
      distance_t const src_distance = d.distance(src);
      if (src_distance == unreachable)
          return 0;

      typedef std::pair<vertex_t,void*> search_state;
      typedef std::vector<search_state> visited_t;
      visited_t visited;
      std::priority_queue<q_elt> q;
      
      q.push(q_elt(src_distance, p, src, identity_cast));
      while (!q.empty())
      {
          q_elt top = q.top();
          q.pop();
          
          // Check to see if we have a real state
          void* dst_address = top.cast(top.src_address);
          if (dst_address == 0)
              continue;

          if (top.target == dst)
              return dst_address;
          
          search_state s(top.target,dst_address);

          visited_t::iterator pos = std::lower_bound(
              visited.begin(), visited.end(), s);

          // If already visited, continue
          if (pos != visited.end() && *pos == s)
              continue;
          
          visited.insert(pos, s); // mark it

          // expand it:
          cast_graph::out_edges_t edges = g.topology().out_edges(s.first);
          for (cast_graph::out_edge_iterator p = edges.first
                   , finish = edges.second
                   ; p != finish
                   ; ++p
              )
          {
              edge_t e = *p;
              distance_t dist = d.distance(e.target);
              if (dist != unreachable) {
                  q.push(q_elt(
                             dist
                             , dst_address
                             , e.target
                             , e.cast));
              }
          }
      }
      return 0;
  }

  struct cache_element
  {
      typedef std::tuple<
          class_id              // source static type
          , class_id            // target type
          , std::ptrdiff_t      // offset within source object
          , class_id            // source dynamic type
          > key_type;

      cache_element(key_type const& k)
          : key(k)
          , offset(0)
      {}
      
      key_type key;
      std::ptrdiff_t offset;

      BOOST_STATIC_CONSTANT(
          std::ptrdiff_t, not_found = std::numeric_limits<std::ptrdiff_t>::min());
      
      bool operator<(cache_element const& rhs) const
      {
          return this->key < rhs.key;
      }

      bool unreachable() const
      {
          return offset == not_found;
      }
  };
  
  typedef std::vector<cache_element> cache_t;

  cache_t& cache()
  {
      static cache_t x;
      return x;
  }

  inline void* convert_type(void* const p, class_id src_t, class_id dst_t, bool polymorphic)
  {
      // Quickly rule out unregistered types
      index_entry* src_p = seek_type(src_t);
      if (src_p == 0)
          return 0;

      index_entry* dst_p = seek_type(dst_t);
      if (dst_p == 0)
          return 0;
    
      // Look up the dynamic_id function and call it to get the dynamic
      // info
      PXR_BOOST_NAMESPACE::python::objects::dynamic_id_t dynamic_id = polymorphic
          ? (src_p->dynamic_id)(p)
          : std::make_pair(p, src_t);
    
      // Look in the cache first for a quickie address translation
      std::ptrdiff_t offset = (char*)p - (char*)dynamic_id.first;

      cache_element seek(std::make_tuple(src_t, dst_t, offset, dynamic_id.second));
      cache_t& c = cache();
      cache_t::iterator const cache_pos
          = std::lower_bound(c.begin(), c.end(), seek);
                      

      // if found in the cache, we're done
      if (cache_pos != c.end() && cache_pos->key == seek.key)
      {
          return cache_pos->offset == cache_element::not_found
              ? 0 : (char*)p + cache_pos->offset;
      }

      // If we are starting at the most-derived type, only look in the up graph
      smart_graph const& g = polymorphic && dynamic_id.second != src_t
          ? full_graph() : up_graph();
    
      void* result = search(g, p, src_p->vertex, dst_p->vertex);

      // update the cache
      c.insert(cache_pos, seek)->offset
          = (result == 0) ? cache_element::not_found : (char*)result - (char*)p;

      return result;
  }
}

namespace python { namespace objects {

PXR_BOOST_PYTHON_DECL void* find_dynamic_type(void* p, class_id src_t, class_id dst_t)
{
    return convert_type(p, src_t, dst_t, true);
}

PXR_BOOST_PYTHON_DECL void* find_static_type(void* p, class_id src_t, class_id dst_t)
{
    return convert_type(p, src_t, dst_t, false);
}

PXR_BOOST_PYTHON_DECL void add_cast(
    class_id src_t, class_id dst_t, cast_function cast, bool is_downcast)
{
    // adding an edge will invalidate any record of unreachability in
    // the cache.
    static std::size_t expected_cache_len = 0;
    cache_t& c = cache();
    if (c.size() > expected_cache_len)
    {
        c.erase(std::remove_if(
                    c.begin(), c.end(),
                    [](cache_element const& e) { return e.unreachable(); })
                , c.end());

        // If any new cache entries get added, we'll have to do this
        // again when the next edge is added
        expected_cache_len = c.size();
    }
    
    type_index_iterator_pair types = demand_types(src_t, dst_t);
    vertex_t src = types.first->vertex;
    vertex_t dst = types.second->vertex;

    cast_graph* const g[2] = { &up_graph().topology(), &full_graph().topology() };
    
    for (cast_graph*const* p = g + (is_downcast ? 1 : 0); p < g + 2; ++p)
    {
        assert(!((*p)->has_edge(src, dst)));
        (*p)->add_edge(src, dst, cast);
    }
}

PXR_BOOST_PYTHON_DECL void register_dynamic_id_aux(
    class_id static_id, dynamic_id_function get_dynamic_id)
{
    demand_type(static_id)->dynamic_id = get_dynamic_id;
}

}}} // namespace PXR_BOOST_NAMESPACE::python::objects
