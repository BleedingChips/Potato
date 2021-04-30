#pragma once
#include <cassert>
#include <optional>
namespace Potato::MemoryModel
{

	struct Property
	{
		size_t align = 0;
		size_t size = 0;
		Property(size_t i_align = 0, size_t i_size = 0)
			: align(i_align), size(i_size)
		{
			assert(((align - 1) & align) == 0);
			assert(align == 0 || size % align == 0);
		};
		Property(Property const&) = default;
		Property& operator=(Property const&) = default;
	};

	struct Style
	{
		struct Result
		{
			size_t member_offset;
			Property owner;
		};
		virtual size_t MinAlign() const noexcept;
		virtual Result InsertMember(Property owner, Property member) noexcept = 0;
		virtual Property Finalize(Property owner) noexcept;
	};

	struct Factory
	{
		Factory(Style& ref) : style_ref(ref), scope(ref.MinAlign(), 0){}
		Factory(Factory const&) = default;
		Factory(Factory &&) = default;
		Style::Result InserMember(Property member);
		Property Finalize() const noexcept { return style_ref.Finalize(scope); }
	private:
		Style& style_ref;
		Property scope;
	};

	struct CLikeStyle : Style
	{
		virtual Result InsertMember(Property owner, Property member) override;
	};

	struct HlslStyle : Style
	{
		static constexpr size_t MinAlign() noexcept { return alignof(uint32_t); }
		virtual auto InsertMember(Property owner, Property member) -> Result override;
	};

	

}
