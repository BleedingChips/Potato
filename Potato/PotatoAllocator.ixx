module;

export module Potato.Allocator;
export import Potato.SmartPtr;
export import Potato.STD;

export namespace Potato::Misc
{
	struct AllocatorInterfaceT
	{

		virtual void AddRef() const = 0;
		virtual void SubRef() const = 0;
		virtual ~AllocatorInterfaceT() = default;

		using PtrT = IntrusivePtr<AllocatorInterfaceT>;

	protected:

		virtual std::byte* Allocate(std::size_t Align, std::size_t Size, std::size_t Count) = 0;
		virtual void Deallocate(std::byte const* MemoryBegin, std::size_t Align, std::size_t Size, std::size_t Count) = 0;
		virtual std::tuple<std::byte*, std::size_t> AllocateAtLast(std::size_t Align, std::size_t Size, std::size_t Count) = 0;

		template<typename Type>
		friend struct AllocatorT;
	};

	template<typename Type>
	struct AllocatorT : public std::allocator<Type>
	{
		using value_type = Type;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using propagate_on_container_move_assignment = std::true_type;

		constexpr Type* allocate(std::size_t Num) {
			if (!Ptr)
			{
				return std::allocator<Type>::allocate(Num);
			}
			else
				return reinterpret_cast<Type*>(Ptr->Allocate(
					alignof(Type),
					sizeof(Type),
					Num
				));
		}

		constexpr void deallocate(Type* const Adress, std::size_t Num) {
			if (!Ptr)
			{
				return std::allocator<Type>::deallocate(Adress, Num);
			}
			else
				return Ptr->Deallocate(
					reinterpret_cast<std::byte const*>(Adress),
					alignof(Type),
					sizeof(Type),
					Num
				);
		}

		constexpr std::allocation_result<Type*> allocate_at_least(std::size_t Num) {
			if (!Ptr)
			{
				return std::allocator<Type>::allocate_at_least(Num);
			}
			else
			{
				auto [Adress, Count] = Ptr->AllocateAtLast(
					alignof(Type),
					sizeof(Type),
					Num
				);

				return { reinterpret_cast<Type*>(Adress), Count };
			}
		}
		AllocatorT() = default;
		AllocatorT(AllocatorInterfaceT::PtrT InputPtr) : Ptr(std::move(InputPtr)) {}
		AllocatorT(AllocatorT const&) = default;
		AllocatorT(AllocatorT&&) = default;
		template<typename OT>
		AllocatorT(AllocatorT<OT> Allocator)
			: AllocatorT(std::move(Allocator.Ptr)) {}

	protected:

		AllocatorInterfaceT::PtrT Ptr;

		template<typename T>
		friend struct AllocatorT;
	};
}