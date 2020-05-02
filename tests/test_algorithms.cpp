#include <gtest/gtest.h>

#include <algorithm>
#include <set>
#include <unordered_set>
#include <vector>

#include <gbwtgraph/algorithms.h>
#include <gbwtgraph/gfa.h>

#include "shared.h"

using namespace gbwtgraph;

namespace
{

//------------------------------------------------------------------------------

class ComponentTest : public ::testing::Test
{
public:
  gbwt::GBWT index;
  GBWTGraph graph;
  size_t components;
  std::vector<std::set<gbwt::vector_type>> correct_paths;

  ComponentTest()
  {
  }

  void SetUp() override
  {
    auto gfa_parse = gfa_to_gbwt("components.gfa");
    this->index = *(gfa_parse.first);
    this->graph = GBWTGraph(this->index, *(gfa_parse.second));
    this->components = 2;
  }
};

TEST_F(ComponentTest, Components)
{
  std::vector<std::vector<nid_t>> correct_components =
  {
    { 11, 12, 13, 14, 15, 16, 17 },
    { 21, 22, 23, 24, 25 }
  };
  std::vector<std::vector<nid_t>> result = weakly_connected_components(this->graph);
  ASSERT_EQ(result.size(), correct_components.size()) << "Wrong number of components";

  for(size_t i = 0; i < result.size(); i++)
  {
    ASSERT_EQ(result[i].size(), correct_components[i].size()) << "Wrong number of nodes in component " << i;
    auto result_iter = result[i].begin();
    auto correct_iter = correct_components[i].begin();
    while(result_iter != result[i].end())
    {
      EXPECT_EQ(*result_iter, *correct_iter) << "Incorrect node in component " << i;
      ++result_iter; ++correct_iter;
    }
  }
}

TEST_F(ComponentTest, HeadNodes)
{
  std::vector<std::vector<nid_t>> correct_heads =
  {
    { 11 },
    { }
  };
  std::vector<std::vector<nid_t>> components = weakly_connected_components(this->graph);
  ASSERT_EQ(components.size(), correct_heads.size()) << "Wrong number of components";

  for(size_t i = 0; i < components.size(); i++)
  {
    std::vector<nid_t> heads = is_nice_and_acyclic(this->graph, components[i]);
    ASSERT_EQ(heads.size(), correct_heads[i].size()) << "Wrong number of head nodes in component " << i;
    auto result_iter = heads.begin();
    auto correct_iter = correct_heads[i].begin();
    while(result_iter != heads.end())
    {
      EXPECT_EQ(*result_iter, *correct_iter) << "Incorrect head node in component " << i;
      ++result_iter; ++correct_iter;
    }
  }
}

//------------------------------------------------------------------------------

class TopologicalOrderTest : public ::testing::Test
{
public:
  gbwt::GBWT index;
  GBWTGraph graph;

  TopologicalOrderTest()
  {
  }

  void SetUp() override
  {
    auto gfa_parse = gfa_to_gbwt("cyclic.gfa");
    this->index = *(gfa_parse.first);
    this->graph = GBWTGraph(this->index, *(gfa_parse.second));
  }

  void check_subgraph(const std::unordered_set<nid_t>& subgraph, bool acyclic) const
  {
    std::vector<handle_t> order = topological_order(this->graph, subgraph);
    if(!acyclic)
    {
      ASSERT_TRUE(order.empty()) << "Non-empty order for a subgraph containing cycles";
      return;
    }

    ASSERT_EQ(order.size(), 2 * subgraph.size()) << "Wrong number of handles in the order";
    for(nid_t node : subgraph)
    {
      for(bool orientation : { false, true })
      {
        handle_t from = this->graph.get_handle(node, orientation);
        auto from_iter = std::find(order.begin(), order.end(), from);
        ASSERT_NE(from_iter, order.end()) << "Node " << node << ", orientation " << orientation << " not found in the order";
        bool ok = this->graph.follow_edges(from, false, [&](const handle_t& to) -> bool
        {
          if(subgraph.find(this->graph.get_id(to)) == subgraph.end()) { return true; }
          auto to_iter = std::find(order.begin(), order.end(), to);
          if(to_iter == order.end()) { return false; }
          return (from_iter < to_iter);
        });
        EXPECT_TRUE(ok) << "Constraints not satisfied for node " << node << ", orientation " << orientation;
      }
    }
  }
};

TEST_F(TopologicalOrderTest, SingleComponent)
{
  std::unordered_set<nid_t> subgraph =
  {
    static_cast<nid_t>(1),
    static_cast<nid_t>(2),
    static_cast<nid_t>(4),
    static_cast<nid_t>(5),
    static_cast<nid_t>(6)
  };
  this->check_subgraph(subgraph, true);
}

TEST_F(TopologicalOrderTest, TwoComponents)
{
  std::unordered_set<nid_t> subgraph =
  {
    static_cast<nid_t>(1),
    static_cast<nid_t>(2),
    static_cast<nid_t>(4),
    static_cast<nid_t>(6),
    static_cast<nid_t>(7),
    static_cast<nid_t>(8),
    static_cast<nid_t>(9)
  };
  this->check_subgraph(subgraph, true);  
}

TEST_F(TopologicalOrderTest, CyclicComponent)
{
  std::unordered_set<nid_t> subgraph =
  {
    static_cast<nid_t>(2),
    static_cast<nid_t>(4),
    static_cast<nid_t>(5),
    static_cast<nid_t>(6),
    static_cast<nid_t>(8)
  };
  this->check_subgraph(subgraph, false);  
}

//------------------------------------------------------------------------------

} // namespace
