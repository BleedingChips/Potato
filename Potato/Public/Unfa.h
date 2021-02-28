#pragma once
#include <string>
#include <vector>
#include <variant>
#include <set>
#include <functional>
#include <optional>
#include <cassert>
#include <cstddef>
#include <span>

#include "Misc.h"
#include "Interval.h"

namespace Potato::Exception::Unfa
{
	struct Interface { virtual ~Interface() = default; };

	using BaseDefineInterface = DefineInterface<Interface>;

	struct UnaccaptableRexgex {
		using ExceptionInterface = BaseDefineInterface;
		std::u32string regex;
		uint32_t accepetable_state;
		uint32_t accepetable_mask;
		size_t Index;
	};
}


namespace Potato::Unfa
{
	struct March
	{
		struct Sub
		{
			std::u32string_view string;
			size_t index;
		};
		Sub capture;
		uint32_t acception_state;
		uint32_t acception_mask;
		std::vector<Sub> sub_capture;
	};

	using IntervalT = Interval<char32_t>;
	using SeqIntervalT = SequenceInterval<char32_t>;
	using SeqIntervalWrapperT = SequenceIntervalWrapper<char32_t>;

	struct Table
	{

		

		struct EAcception { uint32_t acception_index; uint32_t acception_mask; };
		struct EEpsilon {};
		struct ECapture { uint32_t begin; uint32_t require_index; };
		struct EComsume { SeqIntervalT interval; };

		struct Edge
		{
			using PropertyT = std::variant<EAcception, EEpsilon, ECapture, EComsume>;
			uint32_t jump_state;
			PropertyT property;
			Edge(Edge const&) = default;
			Edge(Edge&&) = default;
			Edge(uint32_t js, PropertyT T) : jump_state(js), property(std::move(T)) {}
			template<typename Type>
			bool Is() const noexcept { return std::holds_alternative<Type>(property); }
			template<typename Type>
			decltype(auto) Get() const noexcept { return std::get<Type>(property); }
			template<typename Type>
			decltype(auto) Get() noexcept { return std::get<Type>(property); }
			Edge& operator=(Edge&&) = default;
			Edge& operator=(Edge const&) = default;
		};
		std::vector<std::vector<Edge>> nodes;

		static Table CreateFromRegex(std::u32string_view rex, uint32_t state = 0, uint32_t mask = 0);
		size_t NodeCount() const noexcept { return nodes.size(); }
		size_t StartNodeIndex() const noexcept { return 0; }
		//std::optional<March> Mark(std::u32string_view string, bool greey = true) const;
		std::tuple<std::set<uint32_t>, std::vector<Edge>> SearchThroughEpsilonEdge(uint32_t const* require_state, size_t length) const;
		std::vector<std::tuple<SeqIntervalT, std::vector<uint32_t>>> MergeComsumeEdge(Edge const* edges, size_t edges_length) const;
		static Table Link(Table const* other_table, size_t table_size);
		static void DefaultFilter(Table const&, std::vector<Edge>&);
		Table Simplify(std::function<void(Table const&, std::vector<Edge>&)> edge_filter = Table::DefaultFilter) const;
		operator bool() const noexcept { return nodes.size() >= 2; }
	};

	inline Table CreateUnfaTableFromRegex(std::u32string_view rex, uint32_t state = 0, uint32_t mask = 0) { return Table::CreateFromRegex(rex, state, mask); }
	inline Table LinkUnfaTable(Table const* other_table, size_t table_size) { return Table::Link(other_table, table_size); }

	struct SerilizedTableWrapper
	{

		enum class SEdgeType : uint32_t
		{
			Acception,
			Epsilon,
			CaptureBegin,
			CaptureEnd,
			Comsume
		};

		struct SEEdgeDescription
		{
			SEdgeType type;
			uint32_t jump_state;
		};

		struct SENode { uint32_t edge_start_offset; uint32_t edge_count; };

		bool Empty() const noexcept { return data.empty(); }
		uint32_t NodeCount() const noexcept {
			if (!Empty())
				return *reinterpret_cast<uint32_t const*>(data.data());
			return 0;
		}
		SENode const* Node(size_t node_index) const noexcept {
			assert(!Empty());
			return reinterpret_cast<SENode const*>(data.data() + sizeof(uint32_t)) + node_index;
		}
		SEEdgeDescription const* EdgeStart(size_t node_index) const noexcept
		{
			assert(!Empty());
			return reinterpret_cast<SEEdgeDescription const*>(Node(node_index)->edge_start_offset * sizeof(uint32_t) + data.data());
		}
		uint32_t StartNodeIndex() const noexcept { return 0; }
		std::optional<March> Mark(std::u32string_view string, bool greey = true) const;
		std::span<std::byte const> data;
	};

	struct SerilizedTable
	{
		using SEdgeType = SerilizedTableWrapper::SEdgeType;
		using SEEdgeDescription = SerilizedTableWrapper::SEEdgeDescription;
		using SENode = SerilizedTableWrapper::SENode;

		uint32_t NodeCount() const noexcept { return SerilizedTableWrapper{}.NodeCount(); }
		uint32_t StartNodeIndex() const noexcept { return 0; }
		bool Empty() const noexcept { return datas.empty(); }
		SerilizedTableWrapper AsWrapper() const noexcept { return { std::span<std::byte const>{datas.begin(), datas.end()} }; }
		operator SerilizedTableWrapper() const noexcept { return AsWrapper(); }
		std::optional<March> Mark(std::u32string_view string, bool greey = true) const { return AsWrapper().Mark(string, greey); }
		SerilizedTable() = default;
		SerilizedTable(Table const& table);
		SerilizedTable(SerilizedTable const&) = default;
		SerilizedTable(SerilizedTable&&) = default;
		SerilizedTable& operator=(SerilizedTable const&) = default;
		SerilizedTable& operator=(SerilizedTable&&) = default;
	private:
		std::vector<std::byte> datas;
	};

	inline SerilizedTable SerilizedUnfaTable(Table const& table) { return { table }; }
}

/*
export{
    namespace Potato
    {
		namespace Exception
		{
			struct Interface
			{
				virtual ~Interface() = default;
			};

			struct UnaccaptableRexgex {
				std::u32string regex;
				uint32_t accepetable_state;
				uint32_t accepetable_mask;
				size_t Index;
			};
		}

		template<typename StorageInfo>
		auto MakeUnfaException(StorageInfo&& info) { return MakeException<Exception::Interface>(std::forward<StorageInfo>(info)); }

		struct UnfaMarch
		{
			struct Sub
			{
				std::u32string_view string;
				size_t index;
			};
			Sub capture;
			uint32_t acception_state;
			uint32_t acception_mask;
			std::vector<Sub> sub_capture;
		};

		struct UnfaTable
		{

			struct EAcception { uint32_t acception_index; uint32_t acception_mask; };
			struct EEpsilon {};
			struct ECapture { uint32_t begin; uint32_t require_index; };
			struct EComsume { Interval<char32_t> interval; };

			struct Edge
			{
				using PropertyT = std::variant<EAcception, EEpsilon, ECapture, EComsume>;
				uint32_t jump_state;
				PropertyT property;
				Edge(Edge const&) = default;
				Edge(Edge&&) = default;
				Edge(uint32_t js, PropertyT T) : jump_state(js), property(std::move(T)) {}
				template<typename Type>
				bool Is() const noexcept { return std::holds_alternative<Type>(property); }
				template<typename Type>
				decltype(auto) Get() const noexcept { return std::get<Type>(property); }
				template<typename Type>
				decltype(auto) Get() noexcept { return std::get<Type>(property); }
				Edge& operator=(Edge&&) = default;
				Edge& operator=(Edge const&) = default;
			};
			std::vector<std::vector<Edge>> nodes;

			static UnfaTable CreateFromRegex(std::u32string_view rex, uint32_t state = 0, uint32_t mask = 0);
			size_t NodeCount() const noexcept { return nodes.size(); }
			size_t StartNodeIndex() const noexcept { return 0; }
			//std::optional<March> Mark(std::u32string_view string, bool greey = true) const;
			std::tuple<std::set<uint32_t>, std::vector<Edge>> SearchThroughEpsilonEdge(uint32_t const* require_state, size_t length) const;
			std::vector<std::tuple<Interval<char32_t>, std::vector<uint32_t>>> MergeComsumeEdge(Edge const* edges, size_t edges_length) const;
			static UnfaTable Link(UnfaTable const* other_table, size_t table_size);
			static void DefaultFilter(UnfaTable const&, std::vector<Edge>&);
			UnfaTable Simplify(std::function<void(UnfaTable const&, std::vector<Edge>&)> edge_filter = UnfaTable::DefaultFilter) const;
			operator bool() const noexcept { return nodes.size() >= 2; }
		};

		inline UnfaTable CreateUnfaTableFromRegex(std::u32string_view rex, uint32_t state = 0, uint32_t mask = 0) { return UnfaTable::CreateFromRegex(rex, state, mask); }
		inline UnfaTable LinkUnfaTable(UnfaTable const* other_table, size_t table_size) { return UnfaTable::Link(other_table, table_size); }

		struct UnfaSerilizedTableWrapper
		{

			enum class SEdgeType : uint32_t
			{
				Acception,
				Epsilon,
				CaptureBegin,
				CaptureEnd,
				Comsume
			};

			struct SEEdgeDescription
			{
				SEdgeType type;
				uint32_t jump_state;
			};

			struct SENode { uint32_t edge_start_offset; uint32_t edge_count; };

			bool Empty() const noexcept { return data.empty(); }
			uint32_t NodeCount() const noexcept {
				if (!Empty())
					return *reinterpret_cast<uint32_t const*>(data.data());
				return 0;
			}
			SENode const* Node(size_t node_index) const noexcept {
				assert(!Empty());
				return reinterpret_cast<SENode const*>(data.data() + sizeof(uint32_t)) + node_index;
			}
			SEEdgeDescription const* EdgeStart(size_t node_index) const noexcept
			{
				assert(!Empty());
				return reinterpret_cast<SEEdgeDescription const*>(Node(node_index)->edge_start_offset * sizeof(uint32_t) + data.data());
			}
			uint32_t StartNodeIndex() const noexcept { return 0; }
			std::optional<UnfaMarch> Mark(std::u32string_view string, bool greey = true) const;
			std::span<std::byte const> data;
		};

		struct UnfaSerilizedTable
		{
			using SEdgeType = UnfaSerilizedTableWrapper::SEdgeType;
			using SEEdgeDescription = UnfaSerilizedTableWrapper::SEEdgeDescription;
			using SENode = UnfaSerilizedTableWrapper::SENode;

			uint32_t NodeCount() const noexcept { return UnfaSerilizedTableWrapper{}.NodeCount(); }
			uint32_t StartNodeIndex() const noexcept { return 0; }
			bool Empty() const noexcept { return datas.empty(); }
			UnfaSerilizedTableWrapper AsWrapper() const noexcept { return { std::span<std::byte const>{datas.begin(), datas.end()} }; }
			operator UnfaSerilizedTableWrapper() const noexcept { return AsWrapper(); }
			std::optional<UnfaMarch> Mark(std::u32string_view string, bool greey = true) const { return AsWrapper().Mark(string, greey); }
			UnfaSerilizedTable() = default;
			UnfaSerilizedTable(UnfaTable const& table);
			UnfaSerilizedTable(UnfaSerilizedTable const&) = default;
			UnfaSerilizedTable(UnfaSerilizedTable&&) = default;
			UnfaSerilizedTable& operator=(UnfaSerilizedTable const&) = default;
			UnfaSerilizedTable& operator=(UnfaSerilizedTable&&) = default;
		private:
			std::vector<std::byte> datas;
		};

		inline UnfaSerilizedTable SerilizedUnfaTable(UnfaTable const& table) { return { table }; }
    }
};
*/