#pragma once

#include <optional>
#include <map>
#include <deque>
#include <string_view>
#include "PotatoMisc.h"
#include "PotatoIntrusivePointer.h"

namespace Potato::Path
{
	
	struct Path
	{
		enum class Style : uint8_t
		{
			Unknow = 0,

			Absolute,
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

		Path(std::wstring_view InputPath);
		/*
		Path(Path&&) = default;
		Path(Path const&) = default;
		Path& operator=(Path const&) = default;
		Path& operator=(Path&&) = default;

		bool operator==(Path const& I2) const { return Style == I2.Style && Paths == I2.Paths; }
		bool operator==(std::wstring_view path) const { return Paths == path; }
		size_t ElemntSize() const noexcept { return Elements.size(); }
		std::wstring_view operator[](size_t index) const { return Elements[index](Paths); }
		operator bool() const noexcept{ return Style != Style::Unknow; }

		Style GetType() const noexcept {return Style; }
		bool IsAbsolute() const noexcept {return GetType() == Path::Style::DosAbsolute || GetType() == Path::Style::UnixAbsolute;}
		operator std::wstring_view() const noexcept {return Paths;}
		
		Path Append(Path const&) const;
		size_t LocateFile(std::wstring String) const;
		Path SubPath(size_t start, size_t count = std::numeric_limits<size_t>::max()) const;
		Path Parent() const;
		*/

		/*
		Path FindCurentDirectory(std::function<bool (Path&)> funcobj) const;
		Path FindChildDirectory(std::function<bool(Path&)> funcobj, size_t stack = std::numeric_limits<size_t>::max()) const;
		Path FindParentDirectory(std::function<bool(Path&)> funcobj, size_t stack = std::numeric_limits<size_t>::max()) const;
		*/
		/*
		std::vector<Path> FindAllCurentDirectory(std::function<bool(Path&)> funcobj) const;
		std::vector<Path> FindAllChildDirectory(std::function<bool(Path&)> funcobj, size_t stack = std::numeric_limits<size_t>::max()) const;
		std::vector<Path> FindAllParentDirectory(std::function<bool(Path&)> funcobj, size_t stack = std::numeric_limits<size_t>::max()) const;

		std::wstring_view GetFilename() const;
		std::wstring_view GetFilenameWithoutExtension() const;
		std::wstring_view GetExtension() const;
		*/

		static std::vector<std::byte> LoadEntireFile(Path const& ref);
		static bool SaveFile(Path const& ref, std::span<std::byte const> data);

		static Path Current();
		
	private:

		struct Element
		{
			ElementStyle type;
			Misc::IndexSpan<> Index;
			std::wstring_view operator()(std::wstring_view ref) const { return ref.substr(Index.Begin(), Index.Count()); }
		};
		
		Style Style = Style::Unknow;
		std::wstring Paths;
		std::vector<Element> Elements;
		Misc::IndexSpan<> FileName;
		Misc::IndexSpan<> Extension;
		friend  struct PathMapping;
	};

	/*
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
	*/

	

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