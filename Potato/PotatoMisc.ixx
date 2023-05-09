module;
#include <assert.h>
export module Potato.Misc;

export import Potato.TMP;

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

		constexpr IndexSpan ForwardBegin(Type Offset) { StartPoint += Offset; return *this; }
		constexpr IndexSpan BackwardEnd(Type Offset) { StartPoint += Offset; return *this; }
		constexpr IndexSpan Normalize(Type Length) { EndPoint = StartPoint + Length; return *this; };

	protected:
		
		Type StartPoint;
		Type EndPoint;
	};

	struct AtomicRefCount
	{
		void WaitTouch(size_t targe_value) const noexcept;
		bool TryAndRef() const noexcept;
		void AddRef() const noexcept;
		bool SubRef() const noexcept;
		size_t Count() const noexcept { return ref.load(std::memory_order_relaxed); }
		AtomicRefCount() noexcept : ref(0) {}
		AtomicRefCount(AtomicRefCount const&) = delete;
		AtomicRefCount& operator= (AtomicRefCount const&) = delete;
		~AtomicRefCount();
	private:
		mutable std::atomic_size_t ref = 0;
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
				return StructedSerilizerReader<StructT>{std::span(*OutputBuffer).subspan(0, WritedSize)};
			}
			return {};
		}

		std::size_t GetWritedSize() const { return WritedSize; }

		StructedSerilizerWritter() = default;
		StructedSerilizerWritter(std::span<StructT> ProxyBuffer) :
			OutputBuffer(ProxyBuffer) {}

	public:

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
		return OldWriteSize;
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
		return OldWriteSize;
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
		return OldWriteSize;
	}

	/*
	namespace SerilizerHelper
	{

		template<typename StorageT>
		struct Predicter
		{
			static std::size_t Element() { return 1; }
			template<typename Type> static std::size_t Object() { return AlignedSize<StorageT>(sizeof(std::remove_cvref_t<Type>)); }
			template<typename Type> static std::size_t ObjectArray(std::span<Type const> Source) { return ObjectArray<Type>(Source.size()); };
			template<typename Type> static std::size_t ObjectArray(std::size_t Count) { return AlignedSize<StorageT>(sizeof(std::remove_cvref_t<Type>) * Count); };
			std::size_t SpaceRecord = 0;
			std::size_t WriteElement() { SpaceRecord += Element(); return SpaceRecord; }
			template<typename Type>
			std::size_t WriteObject() { SpaceRecord += Object<Type>(); return SpaceRecord; };
			template<typename Type>
			std::size_t WriteObjectArray(std::span<Type const> Source) { SpaceRecord += ObjectArray(Source); return SpaceRecord; };
			template<typename Type>
			std::size_t WriteObjectArray(std::size_t Num) { SpaceRecord += ObjectArray<Type>(Num); return SpaceRecord; };
			std::size_t RequireSpace() const { return SpaceRecord; }
		};

		template<typename SourceT, typename TargetT>
		bool CrossTypeSet(SourceT& Source, TargetT const& Target) requires(std::is_convertible_v<TargetT, SourceT>)
		{
			Source = static_cast<SourceT>(Target);
			return Source == Target;
		}

		template<typename ExceptableT, typename SourceT, typename TargetT, typename ...Par>
		void CrossTypeSetThrow(SourceT& Source, TargetT const& Target, Par&& ...Pars) requires(std::is_convertible_v<TargetT, SourceT>&& std::is_constructible_v<ExceptableT, Par...>)
		{
			if (!CrossTypeSet(Source, Target))
				throw ExceptableT{ std::forward<Par>(Pars)... };
		}

		template<typename StorageT>
		struct SpanReader
		{

			template<typename Type>
			using AddConst = std::conditional_t<std::is_const_v<StorageT>, std::add_const_t<Type>, Type>;

			SpanReader(std::span<StorageT> Buffer) : TotalBuffer(Buffer), IteBuffer(Buffer) {}
			SpanReader SubSpan(std::size_t Index) const { auto New = *this; New.IteBuffer = IteBuffer.subspan(Index); return New; }
			SpanReader& operator=(SpanReader const&) = default;
			std::size_t GetSize() const { return IteBuffer.size(); }
			std::span<StorageT> GetIteSpan() { return IteBuffer; }
			void OffsetIteSpan(std::size_t Offset) { IteBuffer = IteBuffer.subspan(Offset); }
			void ResetIteSpan(std::size_t Offset = 0) { IteBuffer = TotalBuffer.subspan(Offset); }

			std::span<StorageT> TotalBuffer;
			std::span<StorageT> IteBuffer;

			std::size_t GetIteSpacePositon() const { return TotalBuffer.size() - IteBuffer.size(); }

			template<typename Type>
			Type* NewObject(Type const& Ite) requires(std::is_standard_layout_v<Type> && !std::is_const_v<StorageT>)
			{
				std::size_t AlignedLength = Predicter<StorageT>::template Object<Type>();
				if (AlignedLength <= IteBuffer.size())
				{
					Type* Data = new (IteBuffer.data()) std::remove_cvref_t<Type>{std::forward<Type>(Ite)};
					IteBuffer = IteBuffer.subspan(AlignedLength);
					return Data;
				}
				return nullptr;
			}

			template<typename Type>
			Type* NewObject() requires(std::is_standard_layout_v<Type> && !std::is_const_v<StorageT>)
			{
				std::size_t AlignedLength = Predicter<StorageT>::template Object<Type>();
				if (AlignedLength <= IteBuffer.size())
				{
					Type* Data = new (IteBuffer.data()) std::remove_cvref_t<Type>{};
					IteBuffer = IteBuffer.subspan(AlignedLength);
					return Data;
				}
				return nullptr;
			}

			template<typename Type>
			std::optional<std::span<Type>> NewObjectArray(std::span<Type const> Ite) requires(std::is_standard_layout_v<Type> && !std::is_const_v<StorageT>)
			{
				std::size_t AlignedLength = Predicter<StorageT>::template ObjectArray<Type>(Ite.size());
				if (AlignedLength <= IteBuffer.size())
				{
					std::memcpy(IteBuffer.data(), Ite.data(), Ite.size() * sizeof(std::remove_cvref_t<Type>));
					std::span<Type> Result{ reinterpret_cast<Type*>(IteBuffer.data()), Ite.size() };
					IteBuffer = IteBuffer.subspan(AlignedLength);
					return Result;
				}
				else {
					return {};
				}
			}

			template<typename Type>
			std::optional<std::span<Type>> NewObjectArray(std::span<Type> Ite) requires(std::is_standard_layout_v<Type> && !std::is_const_v<StorageT>)
			{
				return NewObjectArray(std::span<Type const>{Ite});
			}

			template<typename Type>
			std::optional<std::span<Type>> NewObjectArray(std::size_t Count) requires(std::is_standard_layout_v<Type> && !std::is_const_v<StorageT>)
			{
				std::size_t AlignedLength = Predicter<StorageT>::template ObjectArray<Type>(Count);
				if (AlignedLength <= IteBuffer.size())
				{
					new (IteBuffer.data()) Type[Count];
					std::span<Type> Result{ reinterpret_cast<Type*>(IteBuffer.data()), Count };
					IteBuffer = IteBuffer.subspan(AlignedLength);
					return Result;
				}
				else {
					return {};
				}
			}


			StorageT* NewElement() requires(!std::is_const_v<StorageT>) {
				auto Re = IteBuffer.data();
				IteBuffer = IteBuffer.subspan(1);
				return Re;
			}

			template<typename SourceT, typename TargetT>
			bool CrossTypeSet(SourceT& Source, TargetT const& Target) const requires(std::is_convertible_v<TargetT, SourceT>)
			{
				Source = static_cast<SourceT>(Target);
				return Source == Target;
			}

			template<typename ExceptableT, typename SourceT, typename TargetT, typename ...Par>
			void CrossTypeSetThrow(SourceT& Source, TargetT const& Target, Par&& ...Pars) const requires(std::is_convertible_v<TargetT, SourceT>&& std::is_constructible_v<ExceptableT, Par...>)
			{
				if (!CrossTypeSet(Source, Target))
					throw ExceptableT{ std::forward<Par>(Pars)... };
			}

			StorageT* ReadElement() {
				if (IteBuffer.size() >= 1)
				{
					auto Buffer = reinterpret_cast<StorageT*>(IteBuffer.data());
					IteBuffer = IteBuffer.subspan(1);
					return Buffer;
				}
				return nullptr;
			}

			template<typename Type>
			AddConst<Type>* ReadObject()
			{
				auto RS = Predicter<StorageT>::template Object<Type>();

				if (RS <= IteBuffer.size())
				{
					auto Buffer = reinterpret_cast<AddConst<Type>*>(IteBuffer.data());
					IteBuffer = IteBuffer.subspan(RS);
					return Buffer;
				}
				return nullptr;
			}

			template<typename Type>
			std::optional<std::span<AddConst<Type>>> ReadObjectArray(std::size_t Count)
			{
				auto RS = Predicter<StorageT>::template ObjectArray<Type>(Count);

				if (RS <= IteBuffer.size())
				{
					auto Buffer = std::span<AddConst<Type>>{ reinterpret_cast<AddConst<Type>*>(IteBuffer.data()), Count };
					IteBuffer = IteBuffer.subspan(RS);
					return Buffer;
				}
				return {};
			}
		};

	};
	*/


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

	bool AtomicRefCount::TryAndRef() const noexcept
	{
		auto oldValue = ref.load(std::memory_order_relaxed);
		assert(static_cast<std::ptrdiff_t>(oldValue) >= 0);
		do
		{
			if (oldValue == 0)
			{
				return false;
			}
		} while (!ref.compare_exchange_strong(oldValue, oldValue + 1, std::memory_order_relaxed, std::memory_order_relaxed));
		return true;
	}

	void AtomicRefCount::WaitTouch(size_t targe_value) const noexcept
	{
		while (auto oldValue = ref.load(std::memory_order_relaxed))
		{
			assert(static_cast<std::ptrdiff_t>(oldValue) >= 0);
			if (oldValue == targe_value)
				break;
		}
	}
}

