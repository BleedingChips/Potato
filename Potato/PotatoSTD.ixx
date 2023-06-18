export module Potato.STD;

export import std.core;
export import std.filesystem;
export import std.threading;
export import std.memory;

export namespace Potato
{

	using MemoryResource = std::pmr::memory_resource;

	template<typename Type = std::byte>
	using Allocator = std::pmr::polymorphic_allocator<Type>;

	inline MemoryResource* DefaultMemoryResource() noexcept { return std::pmr::new_delete_resource(); }
}