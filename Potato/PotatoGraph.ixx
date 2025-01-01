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
		operator bool() const { return node_index != std::numeric_limits<std::size_t>::max(); }
	};

	struct GraphEdge
	{
		GraphNode from;
		GraphNode to;
	};

	struct EdgeOptimize
	{
		bool need_repeat_check = true;
	};

	struct CheckOptimize
	{
		bool need_acyclic_check = true;
	};

	struct DirectedAcyclicGraphImmediately
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

		DirectedAcyclicGraphImmediately(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			: nodes(resource), edges(resource) {}

		GraphNode Add();

		bool AddEdge(GraphNode from, GraphNode to, EdgeOptimize optimize = {}, std::pmr::memory_resource* temp_resource = std::pmr::get_default_resource());
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

	struct DirectedAcyclicGraphDefer
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
		};

		struct Edge
		{
			std::size_t from;
			std::size_t to;

			bool operator==(Edge const&) const = default;
		};

		DirectedAcyclicGraphDefer(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			: nodes(resource), edges(resource) {}

		GraphNode Add(std::size_t append_info = 0);
		
		bool AddEdge(GraphNode from, GraphNode to, EdgeOptimize optimize = {});
		bool RemoveEdge(GraphNode from, GraphNode to);
		bool RemoveNode(GraphNode node);
		std::size_t GetNodeCount() const { return count; }
		bool CheckExist(GraphNode node) const;

		std::optional<std::span<GraphEdge const>> AcyclicCheck(std::span<GraphEdge> output_buffer, CheckOptimize optimize = {}, std::pmr::memory_resource* temporary_resource = std::pmr::get_default_resource());

		std::span<Node const> GetNodes() const { return std::span(nodes); }
		std::span<Edge const> GetEdges() const { return std::span(edges); }
		void Clear() { nodes.clear(); edges.clear(); version = 0; count = 0; }

	protected:

		std::pmr::vector<Node> nodes;
		std::pmr::vector<Edge> edges;
		std::size_t version = 0;
		std::size_t count = 0;
		bool need_update = false;
	};

}