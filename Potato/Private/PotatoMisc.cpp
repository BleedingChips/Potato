#include "../Public/PotatoMisc.h"

#include <cassert>

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

	ClassLayoutAssembler::ClassLayoutAssembler(ClassLayoutSetting Setting)
		: Setting(Setting)
	{
		assert((Setting.MinAlign % 2) == 0);
		if (Setting.MemebrFiledAlign.has_value())
		{
			assert((*Setting.MemebrFiledAlign % 2) == 0);
			CurrentLayout.Align = std::max(Setting.MinAlign, *Setting.MemebrFiledAlign);
		}
		else {
			CurrentLayout.Align = Setting.MinAlign;
		}
	}

	std::size_t ClassLayoutAssembler::InsertMember(ClassLayout MemberLayout)
	{
		assert(MemberLayout.Size % MemberLayout.Align == 0);
		assert((MemberLayout.Align % 2) == 0);
		if (CurrentLayout.Align < MemberLayout.Align)
			CurrentLayout.Align = MemberLayout.Align;
		assert(CurrentLayout.Size % MemberLayout.Align == 0);
		if (!Setting.MemebrFiledAlign.has_value() || (CurrentLayout.Size % *Setting.MemebrFiledAlign) == 0)
		{
			if (CurrentLayout.Size % MemberLayout.Align != 0)
				CurrentLayout.Size += MemberLayout.Align - (CurrentLayout.Size % MemberLayout.Align);
			std::size_t Offset = CurrentLayout.Size;
			CurrentLayout.Size += MemberLayout.Size;
			return Offset;
		}
		else {
			std::size_t RequireAlign = std::max(MemberLayout.Align, *Setting.MemebrFiledAlign);
			if (CurrentLayout.Size % RequireAlign != 0)
				CurrentLayout.Size += MemberLayout.Align - (CurrentLayout.Size % MemberLayout.Align);
			if(CurrentLayout.Size + MemberLayout.Size >= *Setting.MemebrFiledAlign)
				CurrentLayout.Size += *Setting.MemebrFiledAlign - (CurrentLayout.Size % *Setting.MemebrFiledAlign);

			if ((CurrentLayout.Size % *Setting.MemebrFiledAlign) != 0)
				CurrentLayout.Size += *Setting.MemebrFiledAlign - (CurrentLayout.Size % *Setting.MemebrFiledAlign);



			if ((CurrentLayout.Size % *Setting.MemebrFiledAlign) == 0)
			{
				if (CurrentLayout.Size % MemberLayout.Align != 0)
					CurrentLayout.Size += MemberLayout.Align - (CurrentLayout.Size % MemberLayout.Align);
				std::size_t Offset = CurrentLayout.Size;
				CurrentLayout.Size += MemberLayout.Size;
				return Offset;
			}
			else {

			}


			CurrentLayout.Size += *Setting.MemebrFiledAlign - (CurrentLayout.Size % *Setting.MemebrFiledAlign);


			if ((CurrentLayout.Size % *Setting.MemebrFiledAlign) + MemberLayout.Size > *Setting.MemebrFiledAlign)
			{
				
				std::size_t Offset = CurrentLayout.Size;
				CurrentLayout.Size += MemberLayout.Size;
				return Offset;
			}
			else {

			}
		}
		return 0;
	}

	ClassLayout ClassLayoutAssembler::GetFinalLayout() const 
	{
		return {0, 0};
	}
}