module;

#include <cassert>

export module PotatoGraph;

import std;
import PotatoPointer;
import PotatoMisc;

export namespace Potato::Graph
{
	struct GraphNode
	{
		std::size_t node_index = std::numeric_limits<std::size_t>::max();
		std::size_t version = std::numeric_limits<std::size_t>::max();
		std::size_t GetIndex() const { return node_index; }
	};

	struct DirectedAcyclicGraph
	{
		enum class State
		{
			None,
			Active,
		};

		struct Node
		{
			State state = State::None;
			std::size_t version = 0;
			std::size_t in_degree = 0;
			std::size_t topology_degree = 0;
		};

		struct Edge
		{
			std::size_t from;
			std::size_t to;

			bool operator==(Edge const&) const = default;
		};

		DirectedAcyclicGraph(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			: nodes(resource), edges(resource) {}

		GraphNode Add();
		bool AddEdge(GraphNode from, GraphNode to, bool check_repeat = true, bool skip_circle_check = false, std::pmr::memory_resource* temp_resource = std::pmr::get_default_resource());
		bool RemoveEdge(GraphNode from, GraphNode to);
		bool RemoveNode(GraphNode node);
		std::size_t GetNodeCount() const { return count; }
		bool CheckExist(GraphNode node) const;

		std::span<Node const> GetNodes() const { return std::span(nodes); }
		std::span<Edge const> GetEdges() const { return std::span(edges); }
		void Clear() { nodes.clear(); edges.clear(); version = 0; count = 0; }

	protected:

		std::pmr::vector<Node> nodes;
		std::pmr::vector<Edge> edges;
		std::size_t version = 0;
		std::size_t count = 0;
	};

}