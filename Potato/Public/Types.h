#pragma once
#include <cassert>
#include <optional>
#include <vector>
namespace Potato::Types
{

	enum class BuildIn
	{
		BOOL,
		BYTE,
		CHAR8,
		CHAR16,
		CHAR32,
		INT32,
		UINT32,
		FLOAT,
		DOUBLE,
		INT64,
		UINT64,
		UNKNOW,
	};

	template<typename Type> struct BuildInEnum { static inline constexpr BuildIn value = BuildIn::UNKNOW; };
	template<> struct BuildInEnum<bool> { static inline constexpr BuildIn value = BuildIn::BOOL; };
	template<> struct BuildInEnum<std::byte> { static inline constexpr BuildIn value = BuildIn::BYTE; };
	template<> struct BuildInEnum<char8_t> { static inline constexpr BuildIn value = BuildIn::CHAR8; };
	template<> struct BuildInEnum<char16_t> { static inline constexpr BuildIn value = BuildIn::CHAR16; };
	template<> struct BuildInEnum<char32_t> { static inline constexpr BuildIn value = BuildIn::CHAR32; };
	template<> struct BuildInEnum<int32_t> { static inline constexpr BuildIn value = BuildIn::INT32; };
	template<> struct BuildInEnum<uint32_t> { static inline constexpr BuildIn value = BuildIn::UINT32; };
	template<> struct BuildInEnum<float> { static inline constexpr BuildIn value = BuildIn::FLOAT; };
	template<> struct BuildInEnum<double> { static inline constexpr BuildIn value = BuildIn::DOUBLE; };
	template<> struct BuildInEnum<int64_t> { static inline constexpr BuildIn value = BuildIn::INT64; };
	template<> struct BuildInEnum<uint64_t> { static inline constexpr BuildIn value = BuildIn::UINT64; };
	template<typename Type> constexpr BuildIn BuildInEnumV = BuildInEnum<Type>::value;

	struct Layout
	{
		size_t align = 0;
		size_t size = 0;
		constexpr Layout(size_t i_align = 0, size_t i_size = 0)
			: align(i_align), size(i_size)
		{
			assert(((align - 1) & align) == 0);
			assert(align == 0 || size % align == 0);
		};
		constexpr Layout(Layout const&) = default;
		constexpr Layout& operator=(Layout const&) = default;
	};

	template<typename Type> requires (BuildInEnumV<Type> != BuildIn::UNKNOW) 
	struct BuildInLayout { static inline constexpr Layout value{alignof(Type), sizeof(Type)}; };
	template<typename Type> constexpr Layout BuildInLayoutV = BuildInLayout<Type>::value;

	struct LayoutStyle
	{
		struct Result
		{
			size_t member_offset;
			Layout owner;
		};
		virtual size_t MinAlign() const noexcept;
		virtual Result InsertMember(Layout owner, Layout member) noexcept = 0;
		virtual Layout Finalize(Layout owner) noexcept;
	};

	struct LayoutFactory
	{
		LayoutFactory(LayoutStyle& ref) : style_ref(ref), scope(ref.MinAlign(), 0){}
		LayoutFactory(LayoutFactory const&) = default;
		LayoutFactory(LayoutFactory&&) = default;
		LayoutStyle::Result InserMember(Layout member);
		Layout Finalize() const noexcept { return style_ref.Finalize(scope); }
	private:
		LayoutStyle& style_ref;
		Layout scope;
	};

	struct CLikeLayoutStyle : LayoutStyle
	{
		virtual Result InsertMember(Layout owner, Layout member) noexcept override;
	};

	struct HlslLikeLayOutStyle : LayoutStyle
	{
		static constexpr size_t MinAlign() noexcept { return alignof(uint32_t); }
		virtual auto InsertMember(Layout owner, Layout member) noexcept -> Result override;
	};

	struct Modifier
	{
		enum class Type
		{
			CONST,
			REFERENCE,
			POINTER,
			ARRAY,
		};
		Type type;
		size_t parameter;
	};

}
