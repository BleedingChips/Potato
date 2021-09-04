#pragma once
#include <filesystem>
#include <shared_mutex>
#include <optional>
#include <thread>
#include <future>
#include <map>
#include <deque>

#include "Misc.h"
#include "IntrusivePointer.h"
#include "StrEncode.h"

namespace Potato
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
		bool operator==(Path const& path) const noexcept;
		bool operator==(std::u32string_view path) const noexcept { return this->path == path;}
		bool operator==(std::u32string const& path) const noexcept { return this->path == path; }
		Style GetType() const noexcept {return style; }
		bool IsAbsolute() const noexcept {return GetType() == Path::Style::DosAbsolute || GetType() == Path::Style::UnixAbsolute;}
		operator std::u32string_view() const noexcept {return path;}
		Path() = default;
		Path(std::u32string_view InputPath);
		Path(Path&&) = default;
		Path(Path const&) = default;
		Path(const char32_t input[]) : Path(std::u32string_view(input)){}
		Path& operator=(Path const&) = default;
		Path& operator=(Path&&) = default;
		Path Append(Path const&) const;
		size_t LocateFile(std::u32string_view) const;
		Path SubPath(size_t start, size_t count = std::numeric_limits<size_t>::max()) const;
		Path Parent() const;

		Path FindCurentDirectory(std::function<bool (Path&)> funcobj) const;
		Path FindChildDirectory(std::function<bool(Path&)> funcobj, size_t stack = std::numeric_limits<size_t>::max()) const;
		Path FindParentDirectory(std::function<bool(Path&)> funcobj, size_t stack = std::numeric_limits<size_t>::max()) const;
		std::vector<Path> FindAllCurentDirectory(std::function<bool(Path&)> funcobj) const;
		std::vector<Path> FindAllChildDirectory(std::function<bool(Path&)> funcobj, size_t stack = std::numeric_limits<size_t>::max()) const;
		std::vector<Path> FindAllParentDirectory(std::function<bool(Path&)> funcobj, size_t stack = std::numeric_limits<size_t>::max()) const;

		std::u32string_view Filename() const;
		std::u32string_view FilenameWithoutExtension() const;
		std::u32string_view Extension() const;
		std::tuple<std::u32string_view, std::u32string_view> FilenameWithoutExtensionAndExtension() const;

		template<typename CharType>
		decltype(auto) ToString() const { return ToStringImp(ItSelf<CharType>{}); }

		static std::vector<std::byte> LoadEntireFile(Path const& ref);
		static bool SaveFile(Path const& ref, std::span<std::byte const> data);

		static Path Current();
		
	private:

		template<typename InputType>
		decltype(auto) ToStringImp(ItSelf<InputType>) const { return AsStrWrapper(path.data(), path.size()).ToString<InputType>(); }
		decltype(auto) ToStringImp(ItSelf<char32_t>) const { return path; }

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
		static PathMapping& GobalPathMapping();
	private:
		mutable std::shared_mutex mutex;
		std::map<std::u32string, Path> mapping;
	};

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