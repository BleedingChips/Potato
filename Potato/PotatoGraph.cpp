module;

#include <cassert>

module PotatoGraph;


namespace Potato::Graph
{
	GraphNode DirectedAcyclicGraph::Add()
	{
		if(count < nodes.size())
		{
			for (std::size_t i = 0; i < nodes.size(); ++i)
			{
				auto& ref = nodes[i];
				if (ref.state == State::None)
				{
					ref.in_degree = 0;
					ref.state = State::Active;
					ref.topology_degree = version;
					ref.version = version;
					++version;
					++count;
					return { i, ref.topology_degree };
				}
			}
		}
		auto index = nodes.size();
		GraphNode node{ index, version };
		nodes.emplace_back(State::Active, version, 0, version);
		++version;
		++count;
		return node;
	}

	bool DirectedAcyclicGraph::RemoveEdge(GraphNode from, GraphNode to)
	{
		if(
			CheckExist(from) && CheckExist(to)
			)
		{
			auto size_count = std::erase_if(
				edges,
				[=](Edge ite) { return ite.from == from.node_index && ite.to == to.node_index; }
			);
			if(size_count != 0)
			{
				nodes[to.node_index].in_degree -= size_count;
				return true;
			}
		}
		return false;
	}

	bool DirectedAcyclicGraph::RemoveNode(GraphNode node)
	{
		if (
			CheckExist(node)
			)
		{

			auto size_count = std::erase_if(
				edges,
				[&](Edge ite)
				{
					if(ite.from == node.node_index)
					{
						nodes[ite.to].in_degree -= 1;
						return true;
					}
					return ite.to == node.node_index;
				}
			);

			nodes[node.node_index].state = State::None;
			count -= 1;
			return true;
		}
		return false;
	}

	bool DirectedAcyclicGraph::AddEdge(GraphNode from, GraphNode to, bool check_repeat, bool skip_circle_check, std::pmr::memory_resource* temp_resource)
	{
		if (
			CheckExist(from) && CheckExist(to)
			)
		{
			if(check_repeat)
			{
				auto find = std::find(edges.begin(), edges.end(), Edge{from.node_index, to.node_index});
				if(find != edges.end())
				{
					return false;
				}
			}

			if(skip_circle_check)
			{
				edges.emplace_back(from.GetIndex(), to.GetIndex());
				return true;
			}

			auto f = from.node_index;
			auto t = to.node_index;

			auto& node_f = nodes[f];
			auto& node_t = nodes[t];

			auto node_f_topology_degree = node_f.topology_degree;
			auto node_t_topology_degree = node_t.topology_degree;

			if (node_f_topology_degree > node_t_topology_degree)
			{
				edges.emplace_back(f, t);
				node_t.in_degree += 1;
				return true;
			}
			else
			{
				std::size_t min_topology_degree_to_from_node = version + 1;
				std::size_t max_topology_degree_from_to_node = 0;

				for (auto& ite : edges)
				{
					if (ite.to == f)
					{
						min_topology_degree_to_from_node = std::min(min_topology_degree_to_from_node, nodes[ite.from].topology_degree + 1);
					}

					if (ite.from == t)
					{
						max_topology_degree_from_to_node = std::max(max_topology_degree_from_to_node, nodes[ite.to].topology_degree + 1);
					}
				}

				bool move_left = (min_topology_degree_to_from_node > node_t_topology_degree + 1);
				bool move_right = (max_topology_degree_from_to_node < node_f_topology_degree + 1);


				if (move_left && move_right)
				{
					edges.emplace_back(f, t);
					node_t.in_degree += 1;
					std::swap(node_f.topology_degree, node_t.topology_degree);
					return true;
				}
				else if (move_left)
				{
					node_t.in_degree += 1;
					edges.emplace_back(f, t);
					for (auto& ite : nodes)
					{
						if (ite.topology_degree == node_f_topology_degree)
						{
							ite.topology_degree = node_t_topology_degree;
						}
						else if (ite.topology_degree > node_f_topology_degree && ite.topology_degree <= node_t_topology_degree)
						{
							ite.topology_degree -= 1;
						}
					}
					return true;
				}
				else if (move_right)
				{
					node_t.in_degree += 1;
					edges.emplace_back(f, t);
					for (auto& ite : nodes)
					{
						if (ite.topology_degree == node_t_topology_degree)
						{
							ite.topology_degree = node_f_topology_degree;
						}
						else if (ite.topology_degree >= node_f_topology_degree && ite.topology_degree < node_t_topology_degree)
						{
							ite.topology_degree += 1;
						}
					}
					return true;
				}
				else
				{

					struct NodePro
					{
						std::size_t in_degree = 0;
						std::size_t ref_index = 0;
						bool exported = false;
					};

					std::pmr::vector<NodePro> cur_in_degree(temp_resource);
					cur_in_degree.reserve(nodes.size());
					std::pmr::vector<std::size_t> mapping_array(temp_resource);
					mapping_array.resize(nodes.size(), 0);

					std::size_t total_node = 0;

					std::size_t index = 0;
					for (auto& ite : nodes)
					{
						cur_in_degree.emplace_back(ite.in_degree, index++);
					}

					cur_in_degree[t].in_degree += 1;

					bool Finded = true;
					while (Finded)
					{
						Finded = false;
						for (auto& ite : cur_in_degree)
						{
							if (!ite.exported)
							{
								if (ite.in_degree == 0)
								{
									Finded = true;
									ite.exported = true;
									mapping_array[ite.ref_index] = total_node + 1;
									total_node += 1;
									if (ite.ref_index == f)
									{
										cur_in_degree[t].in_degree -= 1;
									}
									for (auto& ite2 : edges)
									{
										if (ite2.from == ite.ref_index)
										{
											cur_in_degree[ite2.to].in_degree -= 1;
										}
									}
								}
							}
						}
					}
					if (total_node != cur_in_degree.size())
					{
						return false;
					}

					nodes[t].in_degree += 1;

					edges.emplace_back(f, t);

					assert(!nodes.empty());

					for (std::size_t i = 0; i < nodes.size(); ++i)
					{
						nodes[i].topology_degree = total_node - mapping_array[i];
					}
					return true;
				}
			}
		}
		return false;
	}

	bool DirectedAcyclicGraph::CheckExist(GraphNode node) const
	{
		return node.node_index < nodes.size() &&
			nodes[node.node_index].version == node.version;
	}

}