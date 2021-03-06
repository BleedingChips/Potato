#include "../Public/Unfa.h"
#include "../Public/Lr0.h"
#include <optional>
#include <variant>
#include <string_view>
#include <deque>

namespace Potato::Unfa
{
	
	char32_t MaxChar() { return  0x10FFFF;};
	IntervalT const& MaxIntervalRange() { static IntervalT max{1, MaxChar()}; return max; };

	enum class T
	{
		Char = 0,
		Min, // -
		SquareBracketsLeft, //[
		SquareBracketsRight, // ]
		ParenthesesLeft, //(
		ParenthesesRight, //)
		Mulity, //*
		MulityNoGreedy,
		Question, // ?
		QuestionNoGreedy,
		Or, // |
		Add, // +
		AddNoGreedy,
		Not, // ^
		Colon, // :
	};

	constexpr Lr0::Symbol operator*(T sym) { return Lr0::Symbol{ static_cast<size_t>(sym), Lr0::TerminalT{} }; };

	enum class NT
	{
		Statement,
		ExpressionStatement,
		LeftOrStatement,
		RightOrStatement,
		OrStatement,
		CharList,
		CharListNotable,
		Expression,
		NoGreedAppend,
	};

	constexpr Lr0::Symbol operator*(NT sym) { return Lr0::Symbol{ static_cast<size_t>(sym), Lr0::NoTerminalT{} }; };

	const Lr0::Table& rex_lr0()
	{
		static Lr0::Table rex_lr0 = Lr0::CreateTable(
			*NT::Statement,
			{
				{{*NT::Statement, *NT::ExpressionStatement}, 0},
				{{*NT::Statement, *NT::OrStatement}, 0},
				{{*NT::LeftOrStatement, *NT::LeftOrStatement, *NT::Expression}, 1},
				{{*NT::LeftOrStatement, *NT::Expression}, 0},
				{{*NT::RightOrStatement, *NT::Expression, *NT::RightOrStatement}, 1},
				{{*NT::RightOrStatement, *NT::Expression}, 0},
				{{*NT::OrStatement, *NT::LeftOrStatement, *T::Or, *NT::RightOrStatement}, 2},
				{{*NT::OrStatement, *NT::OrStatement, *T::Or, *NT::RightOrStatement}, 2},
				{{*NT::Expression, *T::ParenthesesLeft, *T::Question, *T::Colon, *NT::OrStatement, *T::ParenthesesRight}, 3},
				{{*NT::Expression, *T::ParenthesesLeft, *T::Question, *T::Colon, *NT::ExpressionStatement, *T::ParenthesesRight}, 3},
				{{*NT::Expression, *T::ParenthesesLeft, *T::Question, *T::Colon, *T::ParenthesesRight}, 19},
				{{*NT::Expression, *T::ParenthesesLeft, *T::ParenthesesRight}, 20},
				{{*NT::Expression, *T::ParenthesesLeft, *NT::OrStatement, *T::ParenthesesRight}, 4},
				{{*NT::Expression, *T::ParenthesesLeft, *NT::ExpressionStatement, *T::ParenthesesRight}, 4},
				{{*NT::CharList}, 5},
				{{*NT::CharList, *NT::CharList, *T::Char}, 6},
				{{*NT::CharList, *NT::CharList, *T::Char, *T::Min, *T::Char}, 7},
				{{*NT::Expression, *T::SquareBracketsLeft, *NT::CharList, *T::SquareBracketsRight}, 8},
				{{*NT::Expression, *T::SquareBracketsLeft, *T::Not, *NT::CharList, *T::SquareBracketsRight}, 9},
				{{*NT::Expression, *T::Colon}, 12},
				{{*NT::Expression, *T::Char}, 12},
				{{*NT::Expression, *T::Min}, 12},
				{{*NT::Expression, *T::Not}, 12},
				{{*NT::Expression, *NT::Expression, *T::MulityNoGreedy}, 13},
				{{*NT::Expression, *NT::Expression, *T::AddNoGreedy}, 14},
				{{*NT::Expression, *NT::Expression, *T::QuestionNoGreedy}, 15},
				{{*NT::Expression, *NT::Expression, *T::Mulity}, 16},
				{{*NT::Expression, *NT::Expression, *T::Add}, 17},
				{{*NT::Expression, *NT::Expression, *T::Question}, 18},
				{{*NT::ExpressionStatement, *NT::ExpressionStatement, *NT::Expression}, 1},
				{{*NT::ExpressionStatement, *NT::Expression}, 0},
			}, {}
		);
		return rex_lr0;
	}

	std::tuple<T, SeqIntervalT> RexLexerTranslater(std::u32string_view::const_iterator& begin, std::u32string_view::const_iterator end)
	{
		assert(begin != end);
		char32_t input = *(begin++);
		switch (input)
		{
		case U'-':return { T::Min, SeqIntervalT{input} };
		case U'[': return { T::SquareBracketsLeft,  SeqIntervalT{input} };
		case U']':return  { T::SquareBracketsRight,  SeqIntervalT{input} };
		case U'(': return  { T::ParenthesesLeft ,  SeqIntervalT{input} };
		case U')':return { T::ParenthesesRight,  SeqIntervalT{input} };
		case U'*':
		{
			if(begin != end && *(begin) == U'?')
			{
				++begin;
				return {T::MulityNoGreedy, SeqIntervalT{input} };
			}else
				return { T::Mulity, SeqIntervalT{input} };
		}
		case U'?':
		{
			if (begin != end && *(begin) == U'?')
			{
				++begin;
				return { T::QuestionNoGreedy,  SeqIntervalT{input} };
			}
			else
				return { T::Question, SeqIntervalT{input} };
		}
		case U'.': return { T::Char, SeqIntervalT(MaxIntervalRange()) };
		case U'|':return { T::Or, SeqIntervalT(input) };
		case U'+':
		{
			if (begin != end && *(begin) == U'?')
			{
				++begin;
				return { T::AddNoGreedy, SeqIntervalT{input} };
			}
			else
				return { T::Add, SeqIntervalT{input} };
		}
		case U'^':return { T::Not, SeqIntervalT{input} };
		case U':':return {T::Colon, SeqIntervalT{input} };
		case U'\\':
		{
			if(begin != end)
			{
				input = *(begin++);
				switch (input)
				{
				case U'd': return { T::Char, SeqIntervalT({U'0', U'9' + 1}) };
				case U'D': {
					SeqIntervalT Tem{{ {1, U'0'},{U'9' + 1, MaxChar()} }};
					return { T::Char, std::move(Tem) };
				};
				case U'f': return { T::Char, SeqIntervalT{ U'\f'} };
				case U'n': return { T::Char, SeqIntervalT{ U'\n'} };
				case U'r': return { T::Char, SeqIntervalT{ U'\r'} };
				case U't': return { T::Char, SeqIntervalT{ U'\t'} };
				case U'v': return { T::Char, SeqIntervalT{ U'\v'} };
				case U's':
				{
					SeqIntervalT tem({ { 1, 33 }, {127, 128} });
					return { T::Char, std::move(tem) };
				}
				case U'S':
				{
					SeqIntervalT tem({ {33, 127}, {128, MaxChar()} });
					return { T::Char, std::move(tem) };
				}
				case U'w':
				{
					SeqIntervalT tem({ { U'a', U'z' + 1 }, { U'A', U'Z' + 1 }, { U'_'} });
					return { T::Char, std::move(tem) };
				}
				case U'W':
				{
					SeqIntervalT tem({ { U'a', U'z' + 1 }, { U'A', U'Z' + 1 }, { U'_'} });
					SeqIntervalT total({1, MaxChar() });
					return { T::Char, tem.Complementary(MaxIntervalRange())};
				}
				case U'u':
				{
					assert(begin + 4 <= end);
					size_t index = 0;
					for (size_t i = 0; i < 4; ++i)
					{
						char32_t in = *(begin++);
						index *= 16;
						if (in >= U'0' && in <= U'9')
							index += in - U'0';
						else if (in >= U'a' && in <= U'f')
							index += in - U'a' + 10;
						else if (in >= U'A' && in <= U'F')
							index += in - U'A' + 10;
						else
							assert(false);
					}
					return { T::Char, SeqIntervalT{static_cast<char32_t>(index)} };
				}
				default:
					return { T::Char, SeqIntervalT{input} };
					break;
				}
			}else
				throw Exception::MakeExceptionTuple(Exception::Unfa::UnaccaptableRexgex{});
			break;
		}
		default:
			return { T::Char, SeqIntervalT{input} };
			break;
		}
	}

	std::tuple<std::vector<Lr0::Symbol>, std::vector<SeqIntervalT>> RexLexer(std::u32string_view Input)
	{
		auto begin = Input.begin();
		auto end = Input.end();
		std::vector<Lr0::Symbol> Symbols;
		std::vector<SeqIntervalT> RangeSets;
		while (begin != end)
		{
			auto [Sym, Rs] = RexLexerTranslater(begin, end);
			Symbols.push_back(*Sym);
			RangeSets.push_back(Rs);
		}
		return { std::move(Symbols), std::move(RangeSets) };
	}

	Table Table::CreateFromRegex(std::u32string_view rex, uint32_t acception_index, uint32_t acception_mask)
	{
		try
		{
			struct TemNode
			{
				uint32_t in;
				uint32_t out;
			};
			std::vector<std::vector<Edge>> temporary_node;
			temporary_node.emplace_back();
			auto [symbols, comsumes] = RexLexer(rex);
			auto history = Lr0::Process(rex_lr0(), symbols.data(), symbols.size());
			auto result = Lr::Process(history, [&](Lr0::NTElement& tra) -> std::any
			{
				switch (tra.mask)
				{
				case 0: return tra[0].Consume();
				case 1:
				{
					auto Node1 = tra[0].Consume<TemNode>();
					auto Node2 = tra[1].Consume<TemNode>();
					temporary_node[Node1.out].emplace_back(Edge{Node2.in,  EEpsilon{}});
					return TemNode{Node1.in, Node2.out};
				}
				case 2:
				{
					auto Node1 = tra[0].Consume<TemNode>();
					auto Node2 = tra[2].Consume<TemNode>();
					uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
					uint32_t NewNodeOut = NewNodeIn + 1;
					temporary_node.push_back({Edge{Node1.in, EEpsilon{}}, Edge{Node2.in, EEpsilon{}}});
					temporary_node.emplace_back();
					temporary_node[Node1.out].emplace_back(Edge{ NewNodeOut, EEpsilon{}});
					temporary_node[Node2.out].emplace_back(Edge{ NewNodeOut, EEpsilon{}});
					return TemNode{NewNodeIn, NewNodeOut};
				}
				case 3:
				{
					return tra[3].Consume();
				}
				case 4:
				{
					auto Node1 = tra[1].Consume<TemNode>();
					uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
					uint32_t NewNodeOut = NewNodeIn + 1;
					temporary_node.push_back({Edge{Node1.in, ECapture{true, acception_index }}});
					temporary_node.emplace_back();
					temporary_node[Node1.out].emplace_back(Edge{NewNodeOut, ECapture{false, acception_index }});
					return TemNode{ NewNodeIn, NewNodeOut };
				}
				case 19:
				{
					uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
					uint32_t NewNodeOut = NewNodeIn + 1;
					temporary_node.push_back({Edge{NewNodeOut, EEpsilon{}}});
					temporary_node.emplace_back();
					return TemNode{ NewNodeIn, NewNodeOut };
				}
				case 20:
				{
					uint32_t NewNode1 = static_cast<uint32_t>(temporary_node.size());
					uint32_t NewNode2 = NewNode1 + 1;
					uint32_t NewNode3 = NewNode2 + 1;
					temporary_node.push_back({Edge{NewNode2, ECapture{true, acception_index}}});
					temporary_node.push_back({Edge{NewNode3, ECapture{false, acception_index}}});
					temporary_node.emplace_back();
					return TemNode{ NewNode1, NewNode3 };
				}
				case 5: {return SeqIntervalT{}; }
				case 6:
				{
					auto r1 = tra[0].Consume<SeqIntervalT>();
					auto r2 = tra[1].Consume<SeqIntervalT>();
					r1 = r1.Union(r2);
					return std::move(r1);
				}
				case 7:
				{
					auto r1 = tra[0].Consume<SeqIntervalT>();
					auto r2 = tra[1].Consume<SeqIntervalT>();
					auto r3 = tra[3].Consume<SeqIntervalT>();
					r1 = r1.Union(r2.MinMax().Union(r3.MinMax()));
					return std::move(r1);
				}
				case 8:
				{
					uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
					uint32_t NewNodeOut = NewNodeIn + 1;
					temporary_node.push_back({ Edge{ NewNodeOut, EComsume{tra[1].Consume<SeqIntervalT>()} } });
					temporary_node.emplace_back();
					return TemNode{ NewNodeIn, NewNodeOut };
				}
				case 9:
				{
					SeqIntervalT r1 = tra[2].Consume<SeqIntervalT>();
					r1 = r1.Complementary(MaxIntervalRange());
					uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
					uint32_t NewNodeOut = NewNodeIn + 1;
					temporary_node.push_back({ Edge{ NewNodeOut, EComsume{std::move(r1)} } });
					temporary_node.emplace_back();
					return TemNode{ NewNodeIn, NewNodeOut };
				}
				case 12:
				{
					uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
					uint32_t NewNodeOut = NewNodeIn + 1;
					temporary_node.push_back({ Edge{ NewNodeOut, EComsume{tra[0].Consume<SeqIntervalT>()}} });
					temporary_node.emplace_back();
					return TemNode{ NewNodeIn, NewNodeOut };
				}
				case 13:
				{
					auto Node = tra[0].Consume<TemNode>();
					uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
					uint32_t NewNodeOut = NewNodeIn + 1;
					temporary_node.emplace_back();
					temporary_node.emplace_back();
					auto& ref = temporary_node[Node.out];
					auto& ref2 = temporary_node[NewNodeIn];
					ref.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
					ref.emplace_back(Edge{ Node.in, EEpsilon{} });
					ref2.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
					ref2.emplace_back(Edge{ Node.in, EEpsilon{} });
					return TemNode{ NewNodeIn, NewNodeOut };
				}
				case 14:
				{
					auto Node = tra[0].Consume<TemNode>();
					uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
					uint32_t NewNodeOut = NewNodeIn + 1;
					temporary_node.emplace_back();
					temporary_node.emplace_back();
					auto& ref = temporary_node[Node.out];
					auto& ref2 = temporary_node[NewNodeIn];
					ref.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
					ref.emplace_back(Edge{ Node.in, EEpsilon{} });
					ref2.emplace_back(Edge{ Node.in, EEpsilon{} });
					return TemNode{ NewNodeIn, NewNodeOut };
				}
				case 15:
				{
					auto Node = tra[0].Consume<TemNode>();
					uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
					uint32_t NewNodeOut = NewNodeIn + 1;
					temporary_node.emplace_back();
					temporary_node.emplace_back();
					auto& ref = temporary_node[Node.out];
					auto& ref2 = temporary_node[NewNodeIn];
					ref.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
					ref2.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
					ref2.emplace_back(Edge{ Node.in, EEpsilon{} });
					return TemNode{ NewNodeIn, NewNodeOut };
				}
				case 16:
				{
					auto Node = tra[0].Consume<TemNode>();
					uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
					uint32_t NewNodeOut = NewNodeIn + 1;
					temporary_node.emplace_back();
					temporary_node.emplace_back();
					auto& ref = temporary_node[Node.out];
					auto& ref2 = temporary_node[NewNodeIn];
					ref.emplace_back(Edge{ Node.in, EEpsilon{} });
					ref.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
					ref2.emplace_back(Edge{ Node.in, EEpsilon{} });
					ref2.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
					return TemNode{ NewNodeIn, NewNodeOut };
				}
				case 17:
				{
					auto Node = tra[0].Consume<TemNode>();
					uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
					uint32_t NewNodeOut = NewNodeIn + 1;
					temporary_node.emplace_back();
					temporary_node.emplace_back();
					auto& ref = temporary_node[Node.out];
					auto& ref2 = temporary_node[NewNodeIn];
					ref.emplace_back(Edge{ Node.in, EEpsilon{} });
					ref.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
					ref2.emplace_back(Edge{ Node.in, EEpsilon{} });
					return TemNode{ NewNodeIn, NewNodeOut };
				}
				case 18:
				{
					auto Node = tra[0].Consume<TemNode>();
					uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
					uint32_t NewNodeOut = NewNodeIn + 1;
					temporary_node.emplace_back();
					temporary_node.emplace_back();
					auto& ref = temporary_node[Node.out];
					auto& ref2 = temporary_node[NewNodeIn];
					ref.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
					ref2.emplace_back(Edge{ Node.in, EEpsilon{} });
					ref2.emplace_back(Edge{  NewNodeOut, EEpsilon{} });
					return TemNode{ NewNodeIn, NewNodeOut };
				}
				default: assert(false); return {};
				}
				return {};
			}, 
			[&](Lr0::TElement& elf) -> std::any
			{
				return comsumes[elf.token_index];
			}
			);
			auto result_node = std::any_cast<TemNode>(result);
			temporary_node[0].emplace_back(Edge{result_node.in, EEpsilon{}});
			auto last_index = static_cast<uint32_t>(temporary_node.size());
			temporary_node.emplace_back();
			temporary_node[result_node.out].emplace_back(Edge{ last_index, EAcception{acception_index, acception_mask} });
			return {std::move(temporary_node)};
		}
		catch(Exception::Lr::UnaccableSymbol const& Symbol)
		{
			throw Exception::MakeExceptionTuple(Exception::Unfa::UnaccaptableRexgex{ std::u32string(rex), acception_index, acception_mask, Symbol.index });
		}
	}

	std::tuple<std::set<uint32_t>, std::vector<Table::Edge>> Table::SearchThroughEpsilonEdge(uint32_t const* require_state, size_t length) const
	{
		std::vector<Edge> all_edge;
		struct SearchTuple
		{
			uint32_t node;
			uint32_t edge;
		};
		std::set<uint32_t> finded;
		std::vector<SearchTuple> search_stack;

		for (uint32_t i = static_cast<uint32_t>(length); i > 0; --i)
		{
			auto state = require_state[i - 1];
			auto P = finded.insert(state);
			if(P.second)
				search_stack.push_back({ require_state[i - 1], 0 });
		}
			
		while (!search_stack.empty())
		{
			auto& ref = *search_stack.rbegin();
			assert(ref.node < nodes.size());
			auto& node = nodes[ref.node];
			if (ref.edge >= node.size())
			{
				search_stack.pop_back();
			}
			else while (ref.edge < node.size())
			{
				auto edge = node[(ref.edge++)];
				if (edge.Is<EEpsilon>())
				{
					auto re = finded.insert(edge.jump_state);
					if (re.second)
						search_stack.push_back({ edge.jump_state, 0 });
					break;
				}
				else
					all_edge.push_back(edge);
			}
		}
		return { std::move(finded), std::move(all_edge) };
	}

	std::vector<std::tuple<SeqIntervalT, std::vector<uint32_t>>> Table::MergeComsumeEdge(Edge const* tar_edges, size_t edges_length) const
	{
		std::vector<std::tuple<SeqIntervalT, std::vector<uint32_t>>> output_edge;
		for(size_t i = 0; i < edges_length; ++i)
		{
			auto& cur_edge = tar_edges[i];
			assert(cur_edge.Is<EComsume>());
			std::vector<std::tuple<SeqIntervalT, std::vector<uint32_t>>> temporary = std::move(output_edge);
			SeqIntervalT cur(cur_edge.Get<EComsume>().interval);
			for(auto& ite : temporary)
			{
				if(cur.empty())
					output_edge.push_back(std::move(ite));
				auto& [interval, list] = ite;
				auto result = cur.AsWrapper().Collision(interval.AsWrapper());
				cur = SeqIntervalT{std::move(result.left), NoDetectT{}};
				if(!result.middle.empty())
				{
					auto new_list = list;
					assert(std::find(new_list.begin(), new_list.end(), cur_edge.jump_state) == new_list.end());
					new_list.push_back(cur_edge.jump_state);
					output_edge.push_back({ SeqIntervalT{std::move(result.middle), NoDetectT{}}, std::move(new_list)});
				}
				if(!result.right.empty())
					output_edge.push_back({ SeqIntervalT{std::move(result.right), NoDetectT{}}, std::move(list) });
			}
			if(!cur.empty())
				output_edge.push_back({std::move(cur), {cur_edge.jump_state}} );
		}
		return {std::move(output_edge) };
	}

	Table Table::Link(Table const* other_table, size_t table_size)
	{
		size_t edge_count = 1;
		for(size_t i = 0; i < table_size; ++i)
		{
			if(other_table[i])
				edge_count += other_table[i].nodes.size();
		}
		if(edge_count > 3)
		{
			std::vector<std::vector<Edge>> new_node;
			new_node.reserve(edge_count);
			new_node.emplace_back();
			for (size_t i = 0; i < table_size; ++i)
			{
				auto& ref = other_table[i];
				if (ref)
				{
					uint32_t cur_node = static_cast<uint32_t>(new_node.size());
					new_node.insert(new_node.end(), ref.nodes.begin(), ref.nodes.end());
					for (size_t k = cur_node; k < new_node.size(); ++k)
					{
						for (auto& ite : new_node[k])
							ite.jump_state += cur_node;
					}
					new_node[0].emplace_back(Edge{ cur_node, EEpsilon{} });
				}
			}
			return {std::move(new_node)};
		}
		return {};
	}

	void Table::DefaultFilter(Table const&, std::vector<Edge>& edges)
	{
		bool finded_acception = false;
		edges.erase(std::remove_if(edges.begin(), edges.end(), [&](Edge edge)
		{
			if (edge.Is<EAcception>())
			{
				if(!finded_acception)
				{
					finded_acception = true;
					return false;
				}else
					return true;
			}
			return false;
		}), edges.end());
	}

	Table Table::Simplify(std::function<void(Table const&, std::vector<Edge>&)> edge_filter) const
	{
		assert(*this);
		struct RedefineStateInfo
		{
			size_t new_state;
			std::vector<Edge> Edges;
		};

		std::map<std::set<uint32_t>, uint32_t> redefine_state;
		std::deque<std::tuple<uint32_t, std::vector<Edge>>> search;
		std::vector<IntervalT> segment_set;
		std::vector<std::vector<Edge>> temporary_node;

		auto InsertNewStateFuncion = [&](std::set<uint32_t> set, std::vector<Edge> edges) -> uint32_t
		{
			auto re = redefine_state.insert({std::move(set), static_cast<uint32_t>(redefine_state.size())});
			if(re.second)
			{
				temporary_node.emplace_back();
				search.push_back({ re.first->second, std::move(edges)});
			}
			return re.first->second;
		};
		
		{
			uint32_t start_index = 0;
			auto [sets, edges] = SearchThroughEpsilonEdge(&start_index, 1);
			InsertNewStateFuncion(std::move(sets), std::move(edges));
		}
		
		while(!search.empty())
		{
			auto [state_set, edges] = std::move(*search.rbegin());
			search.pop_back();
			if(edge_filter)
				edge_filter(*this, edges);
			auto ite = edges.begin();
			std::optional<uint32_t> AcceptionState;
			while(ite != edges.end())
			{
				auto cur = ite;
				while(cur != edges.end() && cur->Is<EComsume>())
					++cur;
				if(cur != ite)
				{
					auto MergeResult = MergeComsumeEdge(&(*ite), cur - ite);
					for(auto& ite2 : MergeResult)
					{
						auto& [inv, temporary_list] = ite2;
						//std::vector<size_t> temporary_list(old_state_set.begin(), old_state_set.end());
						auto [search_state, search_edge] = SearchThroughEpsilonEdge(temporary_list.data(), temporary_list.size());
						uint32_t state = InsertNewStateFuncion(std::move(search_state), std::move(search_edge));
						temporary_node[state_set].emplace_back(Edge{state, EComsume{std::move(inv)}});
						// todo list
					}
				}
				if(cur != edges.end())
				{
					auto [search_state, search_edge] = SearchThroughEpsilonEdge(&(cur->jump_state), 1);
					uint32_t state = InsertNewStateFuncion(std::move(search_state), std::move(search_edge));
					temporary_node[state_set].emplace_back(Edge{state, cur->property});
					++cur;
				}
				ite = cur;
			}
		}
		return { std::move(temporary_node) };
	}
	
	struct SEAcception{ uint32_t acception_index; uint32_t acception_mask; };
	struct SECapture { uint32_t require_index; };
	struct SEComsume { uint32_t count; };

	struct SENode{ uint32_t edge_start_offset; uint32_t edge_count; };

	SerilizedTable::SerilizedTable(Table const& table)
	{
		if(table)
		{
			size_t total_space = sizeof(uint32_t);
			total_space += table.nodes.size() * sizeof(SENode);
			for (auto& ite : table.nodes)
			{
				for (auto& ite2 : ite)
				{
					total_space += sizeof(SEEdgeDescription);
					if (ite2.Is<Table::EAcception>())
						total_space += sizeof(SEAcception);
					else if (ite2.Is<Table::ECapture>())
						total_space += sizeof(SECapture);
					else if (ite2.Is<Table::EComsume>())
					{
						total_space += sizeof(SEComsume);
						auto& ref = ite2.Get<Table::EComsume>();
						total_space += ref.interval.size() * sizeof(IntervalT);
					}
				}
			}

			datas.resize(total_space);
			uint32_t& node_count = *reinterpret_cast<uint32_t*>(datas.data());
			node_count = static_cast<uint32_t>(table.nodes.size());
			SENode *node_adress = reinterpret_cast<SENode*>(datas.data() + sizeof(uint32_t));
			std::byte *edges_adress = reinterpret_cast<std::byte*>(node_adress + node_count);
			for(auto& ite : table.nodes)
			{
				*(node_adress++) = {
					static_cast<uint32_t>((reinterpret_cast<size_t>(edges_adress) - reinterpret_cast<size_t>(datas.data())) / sizeof(uint32_t)),
					static_cast<uint32_t>(ite.size())
				};
				for(auto& ite2 : ite)
				{
					auto& edge_desc = *reinterpret_cast<SEEdgeDescription*>(edges_adress);
					edges_adress += sizeof(SEEdgeDescription);
					edge_desc.jump_state = ite2.jump_state;
					if(ite2.Is<Table::EAcception>())
					{
						edge_desc.type = SEdgeType::Acception;
						auto& acception = *reinterpret_cast<SEAcception*>(edges_adress);
						edges_adress += sizeof(SEAcception);
						auto& ref = ite2.Get<Table::EAcception>();
						acception.acception_index = ref.acception_index;
						acception.acception_mask = ref.acception_mask;
					}else if(ite2.Is<Table::ECapture>())
					{
						auto& ref = ite2.Get<Table::ECapture>();
						if (ref.begin)
							edge_desc.type = SEdgeType::CaptureBegin;
						else
							edge_desc.type = SEdgeType::CaptureEnd;
						auto& capture = *reinterpret_cast<SECapture*>(edges_adress);
						edges_adress += sizeof(SECapture);
						capture.require_index = ref.require_index;
					}else if(ite2.Is<Table::EComsume>())
					{
						edge_desc.type = SEdgeType::Comsume;
						auto& comsume = *reinterpret_cast<SEComsume*>(edges_adress);
						edges_adress += sizeof(SEComsume);
						auto& ref = ite2.Get<Table::EComsume>();
						comsume.count = static_cast<uint32_t>(ref.interval.size());
						std::memcpy(edges_adress, ref.interval.AsWrapper().data(), sizeof(IntervalT) * comsume.count);
						edges_adress += sizeof(IntervalT) * comsume.count;
					}
				}
			}
		}
	}

	std::optional<March> SerilizedTableWrapper::Mark(std::u32string_view string, bool greey) const
	{
		assert(!Empty());
		struct CaptureTuple
		{
			bool is_begin;
			size_t capture_index;
			size_t require_state;
		};
		struct SearchStack
		{
			size_t node;
			SEEdgeDescription const* current_edge;
			size_t current_edge_index;
			size_t edge_count;
			std::u32string_view last_string;
			size_t capture_count;
		};
		struct NextSearch
		{
			size_t node;
			std::u32string_view last;
		};
		std::deque<CaptureTuple> capture_stack;
		std::deque<SearchStack> search_stack;
		search_stack.push_back({StartNodeIndex(), EdgeStart(StartNodeIndex()), 0, Node(StartNodeIndex())->edge_count, string, capture_stack.size()});
		std::optional<std::tuple<March, std::deque<CaptureTuple>>> last_acception;
		while (!search_stack.empty())
		{
			const SearchStack stack = *search_stack.rbegin();
			search_stack.pop_back();
			capture_stack.resize(stack.capture_count);
			SEEdgeDescription const* next_edge = nullptr;
			std::optional<NextSearch> next_search;
			switch (stack.current_edge->type)
			{
			case SEdgeType::Comsume:
			{
				SEComsume const* comsume = reinterpret_cast<SEComsume const*>(stack.current_edge + 1);
				size_t seg_count = comsume->count;
				IntervalT const* seg = reinterpret_cast<IntervalT const*>(comsume + 1);
				next_edge = reinterpret_cast<SEEdgeDescription const*>(seg + seg_count);
				if (!stack.last_string.empty())
				{
					SeqIntervalWrapperT wrapper(seg, seg_count);
					auto cur_character = *stack.last_string.begin();
					if (wrapper.IsInclude(cur_character))
						next_search = NextSearch{ stack.current_edge->jump_state, {stack.last_string.data() + 1, stack.last_string.size() - 1} };
				}
			}break;
			case SEdgeType::Acception: {
				SEAcception const* acception = reinterpret_cast<SEAcception const*>(stack.current_edge + 1);
				search_stack.clear();
				std::u32string_view capture_str(string.data(), string.size() - stack.last_string.size());
				if (greey)
				{
					next_edge = reinterpret_cast<SEEdgeDescription const*>(acception + 1);
					if (!last_acception || std::get<0>(*last_acception).capture.string.size() < capture_str.size())
					{
						last_acception = {March{
							{capture_str, 0},
							acception->acception_index,
							acception->acception_mask,
							{}
						}, capture_stack};
					}
					next_search = NextSearch{ stack.current_edge->jump_state, stack.last_string };
				}
				else
				{
					last_acception = {March{
							{capture_str, 0},
							acception->acception_index,
							acception->acception_mask,
							{}
					}, std::move(capture_stack)};
				}
			} break;
			case SEdgeType::CaptureBegin:
			case SEdgeType::CaptureEnd:
			{
				SECapture const* capture = reinterpret_cast<SECapture const*>(stack.current_edge + 1);
				next_edge = reinterpret_cast<SEEdgeDescription const*>(capture + 1);
				capture_stack.push_back({ stack.current_edge->type == SEdgeType::CaptureBegin, string.size() - stack.last_string.size(), capture->require_index });
				next_search = NextSearch{ stack.current_edge->jump_state, stack.last_string };
				break;
			}
			case SEdgeType::Epsilon:
			{
				next_edge = stack.current_edge + 1;
				next_search = NextSearch{ stack.current_edge->jump_state, stack.last_string };
			}break;
			default: {assert(false); } break;
			}
			if(next_edge != nullptr)
			{
				auto old_stack = stack;
				++old_stack.current_edge_index;
				if(old_stack.current_edge_index < old_stack.edge_count)
				{
					old_stack.current_edge = next_edge;
					search_stack.push_back(old_stack);
				}
			}
			if(next_search)
			{
				SearchStack stack{
					next_search->node,
					EdgeStart(next_search->node),
					0, Node(next_search->node)->edge_count,
					next_search->last,
					capture_stack.size()
				};
				if(stack.current_edge_index < stack.edge_count)
					search_stack.push_back(stack);
			}
		}
		if (last_acception)
		{
			auto [march, acception_capture_stack] = *last_acception;
			std::vector<March::Sub> mark_result;
			std::deque<std::tuple<CaptureTuple, size_t>> handle_stack;
			for (auto& ite : acception_capture_stack)
			{
				if (ite.require_state == march.acception_state)
				{
					if (ite.is_begin)
					{
						handle_stack.emplace_back(ite, mark_result.size());
						mark_result.emplace_back();
					}
					else {
						assert(!handle_stack.empty());
						auto [tuple, index] = *handle_stack.rbegin();
						handle_stack.pop_back();
						assert(index < mark_result.size());
						auto start_index = tuple.capture_index;
						mark_result[index] = March::Sub{ std::u32string_view{string.data() + start_index, string.data() + ite.capture_index}, start_index };
					}
				}
			}
			assert(handle_stack.empty());
			march.sub_capture = std::move(mark_result);
			return std::move(march);
		}
		return std::nullopt;
	}
}
