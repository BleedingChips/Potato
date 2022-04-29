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
}