#pragma once
#include <filesystem>
#include <shared_mutex>
#include <optional>
#include <thread>
#include <future>
#include <map>

#include "Misc.h"
#include "IntrusivePointer.h"
#include "StrEncode.h"

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
			Root,
			File,
			Self,
			Upper,
		};
		
		size_t Size() const noexcept { return elements.size(); }
		std::u32string_view operator[](size_t index) const { return elements[index](path); }
		operator bool() const noexcept{ return style != Style::Unknow; }
		Style GetType() const noexcept {return style; }

		Path() = default;
		Path(std::u32string_view InputPath);
		Path(Path&&) = default;
		Path(Path const&) = default;
		Path(const char32_t input[]) : Path(std::u32string_view(input)){}
		Path& operator=(Path const&) = default;
		Path& operator=(Path&&) = default;
		Path Append(Path const&) const;
		size_t LocateFile(std::u32string_view) const;
		Path SubPath(size_t start, size_t count) const;
		Path Parent() const;

		Path FindFileFromParent(std::u32string_view Target, size_t MaxStack = 0) const;
		Path FindFileFromChild(std::u32string_view Target) const;
		std::vector<Path> FindAllFileFromChild(std::u32string_view Target) const;

		std::u32string_view Last() const;
		std::u32string ToU32String() const{ return path; }
		std::u16string ToU16String() const { return StrEncode::AsWrapper(path.data(), path.size()).ToString<char16_t>(); }
		std::u8string ToU8String() const { return StrEncode::AsWrapper(path.data(), path.size()).ToString<char8_t>(); }
		
	private:

		struct Element
		{
			ElementStyle type;
			size_t start;
			size_t size;
			std::u32string_view operator()(std::u32string_view ref) const { return {ref.data() + start, size}; }
		};

		bool Insert(std::u32string_view ref, Element const* tup, size_t length);
		
		Style style = Style::Unknow;
		std::u32string path;
		std::vector<Element> elements;
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

	std::vector<std::byte> LoadEntireFile(Path const& ref);
	bool SaveFile(Path const& ref, std::byte const* data, size_t size);
	template<typename Type>
	bool SaveFile(Path const& ref, Type const* data, size_t size){ return SaveFile(ref, reinterpret_cast<std::byte const*>(data), size * sizeof(Type)); }

	Path Current();

	namespace Implement
	{
		struct FileInterface
		{
			virtual void AddRef() const = 0;
			virtual bool SubRef() const = 0;
			virtual ~FileInterface() = default;
		};
	}

	struct File
	{
		static File OpenFile(Path const&);
	private:
		Potato::IntrusivePtr<Implement::FileInterface> ptr;
	};

	

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