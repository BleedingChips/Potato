module;

#include <assert.h>

export module PotatoMisc;

import std;
import PotatoTMP;


export namespace Potato::Misc
{

	template<typename Type = std::size_t>
	struct IndexSpan
	{
		constexpr IndexSpan(Type Start, Type End) : StartPoint(Start), EndPoint(End) { assert(End >= Start); }
		constexpr IndexSpan() : StartPoint(0), EndPoint(0) {   }
		constexpr IndexSpan(IndexSpan const&) = default;
		constexpr IndexSpan& operator=(IndexSpan const&) = default;

		constexpr Type Begin() const { return StartPoint; }
		constexpr Type End() const { return EndPoint; }
		constexpr Type begin() const { return Begin(); }
		constexpr Type end() const { return End(); }
		constexpr Type Size() const { return End() - Begin(); }
		constexpr IndexSpan Expand(IndexSpan const& IS) const {
			return IndexSpan{
				Begin() < IS.Begin() ? Begin() : IS.Begin(),
				End() > IS.End() ? End() : IS.End()
			};
		};

		constexpr bool IsInclude(Type Value) const { return Begin()<= Value && Value < End(); }

		template<typename ArrayType>
		constexpr auto Slice(std::span<ArrayType> Span) const ->std::span<ArrayType>
		{
			return Span.subspan(Begin(), Size());
		};

		template<typename CharT, typename CharTTarid>
		constexpr auto Slice(std::basic_string_view<CharT, CharTTarid> Str) const ->std::basic_string_view<CharT, CharTTarid>
		{
			return Str.substr(Begin(), Size());
		};

		constexpr IndexSpan SubIndex(Type Offset, Type Size = std::numeric_limits<Type>::max()) const {
			auto CurSize = this->Size();
			assert(Offset <= CurSize);
			auto LastSize = CurSize - Offset;
			return IndexSpan { 
				StartPoint + Offset,
				StartPoint + Offset + std::min(LastSize, Size)
			};
		}

		constexpr IndexSpan& ForwardBegin(Type Offset) { StartPoint += Offset; return *this; }
		constexpr IndexSpan& BackwardEnd(Type Offset) { EndPoint += Offset; return *this; }
		constexpr IndexSpan& Normalize(Type Length) { EndPoint = StartPoint + Length; return *this; };
		constexpr IndexSpan& WholeOffset(Type Length) { StartPoint += Length; EndPoint += Length; return *this;}

	protected:
		
		Type StartPoint;
		Type EndPoint;
	};

	struct AtomicRefCount
	{
		bool TryAddRefNotFromZero() const noexcept;
		bool TryAddRefFromZero() const noexcept;
		void AddRef() const noexcept;
		bool SubRef() const noexcept;
		size_t Count() const noexcept { return ref.load(std::memory_order_relaxed); }
		AtomicRefCount() noexcept : ref(static_cast<std::size_t>(0)) {}
		AtomicRefCount(AtomicRefCount const&) = delete;
		AtomicRefCount& operator= (AtomicRefCount const&) = delete;
		~AtomicRefCount();
	private:
		mutable std::atomic_size_t ref;
	};

	template<typename RequireAlignType>
	inline constexpr std::size_t AlignedSize(std::size_t Input)
	{
		auto A1 = Input / sizeof(RequireAlignType);
		if (Input % sizeof(RequireAlignType) != 0)
			++A1;
		return A1;
	}

	template<typename SourceT, typename TargetT>
	inline constexpr bool CrossTypeSet(SourceT& Source, TargetT const& Target) requires(std::is_convertible_v<TargetT, SourceT>)
	{
		Source = static_cast<SourceT>(Target);
		return Source == Target;
	}

	template<typename ExceptableT, typename SourceT, typename TargetT, typename ...Par>
	inline constexpr void CrossTypeSetThrow(SourceT& Source, TargetT const& Target, Par&& ...Pars) requires(std::is_convertible_v<TargetT, SourceT>&& std::is_constructible_v<ExceptableT, Par...>)
	{
		if (!CrossTypeSet(Source, Target))
			throw ExceptableT{ std::forward<Par>(Pars)... };
	}

	template<typename StructT>
	struct StructedSerilizerReader
	{
		StructedSerilizerReader(std::span<StructT> Buffer) : Buffer(Buffer), PointerIndex(0) {}
		
		StructedSerilizerReader(StructedSerilizerReader const&) = default;

		template<typename OtherT>
		using FollowConst = std::conditional_t<
			std::is_const_v<StructT>,
			std::add_const_t<OtherT>,
			OtherT
		>;

		void SetPointer(std::size_t Pointer) { PointerIndex = Pointer; }

		template<typename OtherT>
		auto ReadObject() -> FollowConst<OtherT>*;

		template<typename OtherT>
		auto ReadObjectArray(std::size_t ObjectCount) -> std::span<FollowConst<OtherT>>;

		StructedSerilizerReader SubReader(std::size_t Offset = 0) { StructedSerilizerReader New(*this); New.SetPointer(PointerIndex + Offset);  return New; }

		

	public:

		std::size_t PointerIndex;
		std::span<StructT> Buffer;
	};

	template<typename StructT>
	StructedSerilizerReader(std::span<StructT> Buffer) -> StructedSerilizerReader<StructT>;

	template<typename StructT>
	template<typename OtherT>
	auto StructedSerilizerReader<StructT>::ReadObject() -> FollowConst<OtherT>*
	{
		auto TarSpan = Buffer.subspan(PointerIndex);
		PointerIndex += AlignedSize<StructT>(sizeof(OtherT));
		return reinterpret_cast<
			FollowConst<OtherT>*
		>(TarSpan.data());
	}

	template<typename StructT>
	template<typename OtherT>
	auto StructedSerilizerReader<StructT>::ReadObjectArray(std::size_t ObjectCount)->std::span<FollowConst<OtherT>>
	{
		auto TarSpan = Buffer.subspan(PointerIndex);
		PointerIndex += AlignedSize<StructT>(sizeof(OtherT) * ObjectCount);
		return std::span(reinterpret_cast<FollowConst<OtherT>*>(TarSpan.data()), ObjectCount);
	}

	template<typename StructT>
	struct StructedSerilizerWritter 
	{
		bool IsPredice() const { return !OutputBuffer.has_value(); }
		bool IsWritting() const { return OutputBuffer.has_value(); }

		template<typename OtherT>
		std::size_t WriteObject(OtherT const& Type);

		template<typename OtherT, std::size_t Count>
		std::size_t WriteObjectArray(std::span<OtherT, Count> Input) { return WriteObjectArray(Input.data(), Input.size()); }

		template<typename OtherT>
		std::size_t WriteObjectArray(OtherT* BufferAdress, std::size_t Count);

		template<typename OtherT>
		std::size_t NewObjectArray(std::size_t Count);

		std::optional<StructedSerilizerReader<StructT>> GetReader() const {
			if (IsWritting())
			{
				return StructedSerilizerReader<StructT>{std::span(*OutputBuffer).subspan(Mark, WritedSize - Mark)};
			}
			return {};
		}

		std::size_t GetWritedSize() const { return WritedSize - Mark; }

		StructedSerilizerWritter() = default;
		StructedSerilizerWritter(std::span<StructT> ProxyBuffer) :
			OutputBuffer(ProxyBuffer) {}

		std::size_t PushMark() { auto OldMark = Mark; Mark = WritedSize; return OldMark; }
		void PopMark(std::size_t OldMark) { Mark = OldMark; }

	public:
		
		std::size_t Mark = 0;
		std::size_t WritedSize = 0;
		std::optional<std::span<StructT>> OutputBuffer;
	};

	template<typename StructT>
	StructedSerilizerWritter(std::span<StructT> ProxyBuffer) -> StructedSerilizerWritter<StructT>;

	template<typename StructT>
	template<typename OtherT>
	std::size_t StructedSerilizerWritter<StructT>::WriteObject(OtherT const& Type)
	{
		auto OldWriteSize = WritedSize;
		auto TargetObject = AlignedSize<StructT>(sizeof(OtherT));
		if (OutputBuffer.has_value())
		{
			auto Tar = OutputBuffer->subspan(WritedSize);
			new (Tar.data()) OtherT{ Type };
		}
		WritedSize += TargetObject;
		return OldWriteSize - Mark;
	}

	template<typename StructT>
	template<typename OtherT>
	std::size_t StructedSerilizerWritter<StructT>::NewObjectArray(std::size_t Count)
	{
		auto OldWriteSize = WritedSize;
		auto TargetObject = AlignedSize<StructT>(sizeof(OtherT) * Count);
		if (OutputBuffer.has_value())
		{
			auto Tar = OutputBuffer->subspan(WritedSize);
			auto Adress = reinterpret_cast<std::byte*>(Tar.data());
			for (std::size_t I = 0; I < Count; ++I)
			{
				new (Adress) OtherT{ };
				Adress += sizeof(OtherT);
			}
		}
		WritedSize += TargetObject;
		return OldWriteSize - Mark;
	}

	template<typename StructT>
	template<typename OtherT>
	std::size_t StructedSerilizerWritter<StructT>::WriteObjectArray(OtherT* BufferAdress, std::size_t Count)
	{
		auto OldWriteSize = WritedSize;
		auto TargetObject = AlignedSize<StructT>(sizeof(OtherT) * Count);
		if (OutputBuffer.has_value())
		{
			auto Tar = OutputBuffer->subspan(WritedSize);
			auto Adress = reinterpret_cast<std::byte*>(Tar.data());
			for (std::size_t I = 0; I < Count; ++I)
			{
				new (Adress) OtherT{ BufferAdress[I]};
				Adress += sizeof(OtherT);
			}
		}
		WritedSize += TargetObject;
		return OldWriteSize - Mark;
	}

	struct LineRecorder
	{
		std::size_t Line = 0;
		std::size_t CharacterCount = 0;
		void NewLine() { Line += 1; CharacterCount = 0;}
		void AddCharacter(std::size_t Count = 1) { CharacterCount += 1; }
	};

	inline std::strong_ordering PriorityCompareStrongOrdering() { return std::strong_ordering::equivalent; }

	template<typename T, typename K, typename ...OT>
	std::strong_ordering PriorityCompareStrongOrdering(T&& k1, K&& k2, OT&&... ot)
	{
		std::strong_ordering re = std::forward<T&&>(k1) <=> std::forward<K&&>(k2);
		if(re == std::strong_ordering::equivalent)
			return PriorityCompareStrongOrdering(std::forward<OT>(ot)...);
		return re;
	}

	

}

namespace Potato::Misc
{
	void AtomicRefCount::AddRef() const noexcept
	{
		assert(static_cast<std::ptrdiff_t>(ref.load(std::memory_order_relaxed)) >= 0);
		ref.fetch_add(1, std::memory_order_relaxed);
	}

	bool AtomicRefCount::SubRef() const noexcept
	{
		assert(static_cast<std::ptrdiff_t>(ref.load(std::memory_order_relaxed)) >= 0);
		return ref.fetch_sub(1, std::memory_order_relaxed) == 1;
	}

	AtomicRefCount::~AtomicRefCount() { assert(ref.load(std::memory_order_relaxed) == 0); }

	bool AtomicRefCount::TryAddRefNotFromZero() const noexcept
	{
		std::size_t Except = 1;
		std::size_t Desired = 2;

		while (!ref.compare_exchange_strong(Except, Desired, std::memory_order_relaxed, std::memory_order_relaxed))
		{
			if(Except == 0)
				return false;
			else
				Desired = Except + 1;
		}
		return true;
	}

	bool AtomicRefCount::TryAddRefFromZero() const noexcept
	{
		std::size_t Except = 0;
		return ref.compare_exchange_strong(Except, 1, std::memory_order_relaxed, std::memory_order_relaxed);
	}
}

