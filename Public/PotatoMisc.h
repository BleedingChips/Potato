#pragma once
#include "PotatoTMP.h"

#include <concepts>
#include <any>
#include <span>
#include <atomic>
#include <vector>
#include <algorithm>
#include <cassert>
#include <optional>

namespace Potato::Exception
{
}

namespace Potato::Misc
{
	template<typename RequireType>
	struct SelfCompare
	{
		auto operator()(RequireType const& i1, RequireType const& i2) const { return i1 <=> i2; }
	};

	/*
	inline bool AnyViewer(std::any const& ref)
	{
		return false;
	}

	template<typename CurObject, typename ...FunctionObject>
	bool AnyViewer(std::any const& ref, CurObject&& co, FunctionObject&& ...input_fo)
	{
		using FunT = std::remove_cvref_t<CurObject>;
		if constexpr (std::is_function_v<FunT>)
		{
			using RequireType = std::remove_reference_t<typename TMP::FunctionInfo<FunT>::template PackParameters<TMP::ItSelf>::Type>;
			if (auto P = std::any_cast<RequireType>(&ref); P != nullptr)
			{
				std::forward<CurObject>(co)(*P);
				return true;
			}
			else {
				return AnyViewer(ref, std::forward<FunctionObject>(input_fo)...);
			}
		}
		else if constexpr (TMP::IsFunctionObject<FunT>::Value)
		{
			using RequireType = std::remove_reference_t<typename TMP::FunctionObjectInfo<FunT>::template PackParameters<TMP::ItSelf>::Type>;
			if (auto P = std::any_cast<RequireType>(&ref); P != nullptr)
			{
				std::forward<CurObject>(co)(*P);
				return true;
			}
			else {
				return AnyViewer(ref, std::forward<FunctionObject>(input_fo)...);
			}
		}
		else
			static_assert(false, "wrong function type");
	}

	inline bool AnyViewer(std::any& ref)
	{
		return false;
	}

	template<typename CurObject, typename ...FunctionObject>
	bool AnyViewer(std::any& ref, CurObject&& co, FunctionObject&& ...input_fo)
	{
		using FunT = std::remove_cvref_t<CurObject>;
		if constexpr (std::is_function_v<FunT>)
		{
			using RequireType = std::remove_reference_t<typename TMP::FunctionInfo<FunT>::template PackParameters<TMP::ItSelf>::Type>;
			if (auto P = std::any_cast<RequireType>(&ref); P != nullptr)
			{
				std::forward<CurObject>(co)(*P);
				return true;
			}
			else {
				return AnyViewer(ref, std::forward<FunctionObject>(input_fo)...);
			}
		}
		else if constexpr (IsFunctionObject<FunT>::Value)
		{
			using RequireType = std::remove_reference_t<typename TMP::FunctionObjectInfo<FunT>::template PackParameters<TMP::ItSelf>::Type>;
			if (auto P = std::any_cast<RequireType>(&ref); P != nullptr)
			{
				std::forward<CurObject>(co)(*P);
				return true;
			}
			else {
				return AnyViewer(ref, std::forward<FunctionObject>(input_fo)...);
			}
		}
		else
			static_assert(false, "wrong function type");
	}

	template<typename ...RequireT>
	struct AnyViewerTemplate
	{
		template<typename Func>
		bool operator()(std::any& ref, Func&& func) { return false; }
		template<typename Func>
		bool operator()(std::any const& ref, Func&& func) { return false; }
	};

	template<typename RequireTThis, typename ...RequireT>
	struct AnyViewerTemplate<RequireTThis, RequireT...>
	{
		template<typename Func>
		bool operator()(std::any& ref, Func&& func) {
			if (auto P = std::any_cast<RequireTThis>(&ref); P != nullptr)
			{
				std::forward<Func>(func)(*P);
				return true;
			}
			else {
				return AnyViewerTemplate<RequireT...>{}(ref, std::forward<Func>(func));
			}
		}
		template<typename Func>
		bool operator()(std::any const& ref, Func&& func) {
			if (auto P = std::any_cast<RequireTThis>(&ref); P != nullptr)
			{
				std::forward<Func>(func)(*P);
				return true;
			}
			else {
				return AnyViewerTemplate<RequireT...>{}(ref, std::forward<Func>(func));
			}
		}
	};
	*/

	template<typename StorageType = std::size_t>
	struct IndexSpan
	{
		StorageType Offset = 0;
		StorageType Length = 0;
		template<typename ArrayType>
		auto Slice(ArrayType& Type) const ->std::span<std::remove_reference_t<decltype(*Type.data())>>
		{
			return std::span<std::remove_reference_t<decltype(*Type.begin())>>(Type.data() + Begin(), Length);
		};
		StorageType Begin() const noexcept { return Offset; }
		StorageType End() const noexcept { return Offset + Length; }
		StorageType Count() const noexcept { return Length; };
		bool IsInclude(StorageType Index) const noexcept { return Begin() <= Index && End() > Index; };
		explicit operator bool() const noexcept { return Length != 0; }
		IndexSpan Sub(StorageType InputOffset) const noexcept { assert(InputOffset <= Length);  return { Offset + InputOffset, Length - InputOffset }; }
		IndexSpan Sub(StorageType InputOffset, StorageType Count) const noexcept { assert(InputOffset + Count <= Length);  return { Offset + InputOffset, Count }; }
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
	constexpr std::size_t AlignedSize(std::size_t Input)
	{
		auto A1 = Input / sizeof(RequireAlignType);
		if(Input % sizeof(RequireAlignType) != 0)
			++A1;
		return A1;
	}

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
				if(!CrossTypeSet(Source, Target))
					throw ExceptableT{std::forward<Par>(Pars)...};
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


}