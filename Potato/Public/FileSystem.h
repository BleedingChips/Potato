#pragma once
#include <filesystem>
#include <shared_mutex>
#include <optional>
#include <thread>
#include <future>
#include <map>
#include "../Public/Misc.h"
#include "../Public/IntrusivePointer.h"

namespace Potato::FileSystem
{
	
	struct Path
	{
		enum class Style : uint8_t
		{
			Unknow = 0,

			DosAbsolute,
			UnixAbsolute,
			Relative,
			Mapping,
		};

		enum class ElementStyle : uint8_t
		{
			Root = 0,
			File,
			Self,
			Upper,
		};
		
		size_t Size() const noexcept { return path_ite.size(); }
		std::u32string_view operator[](size_t index) const { return path_ite[index](path); }
		operator bool() const noexcept{ return style != Style::Unknow; }
		Style GetType() const noexcept {return style; }

		Path() = default;
		Path(std::u32string_view InputPath);
		Path(Path&&) = default;
		Path(Path const&) = default;
		Path& operator=(Path const&) = default;
		Path& operator=(Path&&) = default;
		Path Append(Path const&) const;
		
		std::u32string ToU32String() const;
		std::u16string ToU16String() const;
		std::u8string ToU8String() const;
		
	private:

		struct Element
		{
			ElementStyle type;
			size_t start;
			size_t size;
			std::u32string_view operator()(std::u32string_view ref) const { return {ref.data() + start, size}; }
		};

		bool Insert(Path& pat, std::u32string_view ref, Element const* tup, size_t length);
		
		Style style = Style::Unknow;
		std::u32string path;
		std::vector<Element> path_ite;
		friend  struct PathMapping;
	};

	struct PathMapping
	{
		bool Regedit(std::u32string_view, Path path);
		void Unregedit(std::u32string_view);
		Path operator()(Path const&) const;
	private:
		mutable std::shared_mutex mutex;
		std::map<std::u32string, Path> mapping;
	};

	PathMapping& GobalPathMapping();

	Path FindUpperFileName(std::u32string_view Target);

	

	/*
	struct ReadableFile
	{
	private:
		virtual size_t Read(std::byte* output, uint64_t length) = 0;
	};

	struct WriteableFile
	{
	private:
		virtual size_t Write(std::byte* output, uint64_t length) = 0;
	};
	*/
	
	
}