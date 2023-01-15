#include "Potato/PotatoReg.h"
#include "Potato/PotatoSLRX.h"
#include "Potato/PotatoVirtualPath.h"

#include <filesystem>
#include <fstream>
/*
namespace
{
	Potato::EbnfTable const& FSEbnf()
	{
		static Potato::EbnfTable fs_ebnf = Potato::CreateEbnfTable(
UR"(
NormalPath := '[^/]*' : [0]
DosRoot := '[a-zA-Z\-0-9_]*:/' : [1]
Delimiter := '/'
SelfPath := '\.' : [2]
UpperPath := '\.\.' : [3]
MappingRoot := '$[a-zA-Z\-0-9_ ]+:/' : [4]
%%%
$ := <Ste>
<File> := NormalPath | SelfPath | UpperPath : [0]
<FilePath> := <File> { Delimiter <File> } : [1]
<Ste> := DosRoot [ <FilePath> ] : [2]
<Ste> := DosRoot <FilePath> Delimiter : [2]
<Ste> := <FilePath> : [3]
<Ste> := MappingRoot [<FilePath>] : [4]
<Ste> := Delimiter <FilePath> [ Delimiter ] : [5]
)"
		);
		return fs_ebnf;
	}

	std::u32string_view Self = UR"(.)";
	std::u32string_view Upper = UR"(..)";
}
*/

namespace Potato::VirtualPath
{

	enum class T
	{
		NormalPath = 0,
		DosRoot,
		Delimiter,
		SelfPath,
		UpperPath,
		MappingRoot,
		UnixRoot,
	};

	inline constexpr SLRX::Symbol operator*(T Input) { return SLRX::Symbol::AsTerminal(static_cast<SLRX::StandardT>(Input)); }

	enum class NT
	{
		Delimiter = 0,
		NormalFile,
		NormalRoot,
		Root,
		MappingRoot,
		Path,
	};

	inline constexpr SLRX::Symbol operator*(NT Input) { return SLRX::Symbol::AsNoTerminal(static_cast<SLRX::StandardT>(Input)); }


	Reg::TableWrapper FSRegWrapper() {
		static auto Tables = [](){
			Reg::MulityRegCreater Creator;
			Creator.LowPriorityLink(u8R"([^/\\]+(\.[^/\\])?)", false,  {static_cast<Reg::StandardT>(T::NormalPath)});
			Creator.LowPriorityLink(u8R"($[^\./\\]+:)", false, {static_cast<Reg::StandardT>(T::MappingRoot)});
			Creator.LowPriorityLink(u8R"([^\./\\]+:)", false, {static_cast<Reg::StandardT>(T::DosRoot)});
			Creator.LowPriorityLink(u8R"(/|\\)", false, {static_cast<Reg::StandardT>(T::Delimiter)});
			Creator.LowPriorityLink(u8R"(\.)", false, {static_cast<Reg::StandardT>(T::SelfPath)});
			Creator.LowPriorityLink(u8R"(\.\.)", false, {static_cast<Reg::StandardT>(T::UpperPath)});
			return *Creator.GenerateTableBuffer();
		}();
		return Reg::TableWrapper{Tables};
	}

	SLRX::TableWrapper FSSLRXWrapper() {
		static SLRX::Table Tables(
			*NT::Path,
			{
				{*NT::Delimiter, {*T::Delimiter}, 0},
				{*NT::Delimiter, {*NT::Delimiter, *T::Delimiter}, 1},
				{*NT::NormalFile, {*T::NormalPath}, 2},
				{*NT::NormalFile, {*T::SelfPath}, 3},
				{*NT::NormalFile, {*T::UpperPath}, 4},
				{*NT::NormalFile, {*NT::NormalFile, *NT::Delimiter, *NT::NormalFile, 5}, 5},
				{*NT::NormalRoot, {*NT::Delimiter, *NT::NormalFile}, 6},
				{*NT::Root, {*NT::Root, 7, *NT::NormalFile}, 7},
				{*NT::Root, {*T::DosRoot}, 8},
				{*NT::Root, {*NT::Root, *NT::Delimiter, *NT::NormalFile}, 16},
				{*NT::MappingRoot, {*NT::MappingRoot, 9, *NT::NormalFile}, 9},
				{*NT::MappingRoot, {*NT::MappingRoot, *NT::Delimiter, *NT::NormalFile}, 18},
				{*NT::MappingRoot, {*T::MappingRoot}, 10},
				{*NT::Path, {*NT::NormalRoot}, 11},
				{*NT::Path, {*NT::Root}, 12},
				{*NT::Path, {*NT::MappingRoot}, 13},
				{*NT::Path, {*NT::NormalFile}, 20},
				{*NT::Path, {*NT::Path, *NT::Delimiter}, 14}
			}, {}
		);
		return Tables.Wrapper;
	}



	Path::Path(std::u8string_view InputPath)
	{
		auto Wrapper = FSRegWrapper();

		Reg::HeadMatchProcessor Pro(FSRegWrapper(), true);

		struct PathElement
		{
			T Value;
			std::u8string_view Str;
			Misc::IndexSpan<> StrIndex;
		};

		std::vector<PathElement> Symbols;

		auto ItePath = InputPath;

		while (!ItePath.empty())
		{
			auto Re = Reg::HeadMatch(Pro, ItePath);
			if (Re)
			{
				Pro.Clear();
				Symbols.push_back({
					static_cast<T>(Re->AcceptData.Mask),
					Re->GetCaptureWrapper().Slice(ItePath),
					{InputPath.size() - ItePath.size(), Re->GetCaptureWrapper().Count()}
				});
				ItePath = ItePath.substr(Re->GetCaptureWrapper().Count());
			}
			else {
				return;
			}
		}

		SLRX::SymbolProcessor Pro2(FSSLRXWrapper());

		std::size_t TokenIndex = 0;
		for (auto Ite : Symbols)
		{
			if(!Pro2.Consume(SLRX::Symbol::AsTerminal(static_cast<SLRX::StandardT>(Ite.Value)), TokenIndex))
				return;
			++TokenIndex;
		}

		if(!Pro2.EndOfFile())
			return;

		SLRX::ProcessParsingStep(Pro2.GetSteps(), [&](SLRX::VariantElement Ele) -> std::any {
			if (Ele.IsTerminal())
			{
				auto TEle = Ele.AsTerminal();
				auto Val = Symbols[TEle.Shift.TokenIndex];

				switch (Val.Value)
				{
				case T::DosRoot:
				case T::UnixRoot:
				case T::MappingRoot:
					return Element{ ElementStyle::Root, Val.StrIndex};
				case T::NormalPath:
					return Element{ ElementStyle::File, Val.StrIndex };
				case T::SelfPath:
					return Element{ ElementStyle::Self, Val.StrIndex };
				case T::UpperPath:
					return Element{ ElementStyle::Upper, Val.StrIndex };
				default:
					return {};
				}
			}
			else {
				auto NTEle = Ele.AsNoTerminal();
				switch (NTEle.Reduce.Mask)
				{
				case 0:
				case 1:
					return {};
				case 2:
					return NTEle[0].Consume();
				case 3:
					return {};
				}
			}
			return {};
			}
		);
	}


	/*
	bool Path::Insert(std::u32string_view ref, Element const* tup, size_t length)
	{
		std::vector<std::tuple<size_t, Element>> element_list;
		element_list.reserve(Size() + length);
		for(auto& ite : elements)
			element_list.push_back({0, ite});
		for(size_t i = 0; i < length; ++i)
		{
			if (element_list.size() == 0)
				element_list.push_back({1, tup[i]});
			else {
				auto Cur = tup[i];
				if (Cur.type == ElementStyle::Upper)
				{
					auto [Index, Last] = element_list[element_list.size() - 1];
					switch (Last.type)
					{
					case ElementStyle::File:
						element_list.pop_back();
						break;
					case ElementStyle::Root:
						return false;
					case ElementStyle::Self:
						element_list.pop_back();
					default:
						element_list.push_back({1, Last});
					}
				}
				else if (Cur.type == ElementStyle::Self)
				{
					if(element_list.size() == 0)
						element_list.push_back({1, Cur});
				}
				else if (Cur.type == ElementStyle::File)
				{
					auto [Index, Last] = element_list[element_list.size() - 1];
					if (Last.type == ElementStyle::Self)
						element_list.pop_back();
					element_list.push_back({ 1, Cur });
				}
				else if(Cur.type  == ElementStyle::Root)
					return false;
			}
		}
		size_t require = 0;
		for (auto& ite : element_list)
			require += std::get<1>(ite).size;
		if(element_list.size() > 1)
		{
			if(std::get<1>(element_list[0]).type == ElementStyle::Root)
			{
				require += element_list.size() - 2;
			}else
				require += element_list.size() - 1;
		}
		path.resize(require);
		elements.resize(element_list.size());
		size_t Used = 0;
		for (size_t i = 0; i < element_list.size(); ++i)
		{
			auto [Index, Last] = element_list[i];
			if (Index == 0)
				Used = Last.start + Last.size;
			else {
				if (i != 0 && std::get<1>(element_list[i - 1]).type != ElementStyle::Root)
					path[Used++] = U'/';
				std::memcpy(path.data() + Used, ref.data() + Last.start, sizeof(char32_t) * Last.size);
				elements[i] = {Last.type, Used, Last.size};
				Used+= Last.size;
			}
		}

		return true;
	}
	
	Path::Path(std::u32string_view Input)
	{
		EbnfTable const& ref = FSEbnf();
		try
		{
			Path Result;
			auto His = Process(ref, Input);
			His([&](EbnfNTElement& Ele) -> std::any
			{
				if(Ele.IsPredefine()) return {};
				switch (Ele.mask)
					{
					case 0:
					{
						return Ele[0].Consume<EbnfSequencer>()[0].Consume();
					}
					case 1:
					{
						std::vector<Element> Res;
						Res.push_back(Ele[0].Consume<Element>());
						auto Ref = Ele[1].Consume<EbnfSequencer>();
						for (auto& Ite : Ref)
						{
							if (Ite.IsNoterminal())
							{
								auto P = Ite.Consume<Element>();
								Res.push_back(std::move(P));
							}
						}
						return std::move(Res);
					}
					case 2:
					{
						Result.style = Style::DosAbsolute;
						auto Temp = Ele[0].Consume<Element>();
						Result.elements.push_back(Temp);
						Result.path = Temp(Input);
						auto Seq = Ele[1].Consume<EbnfSequencer>();
						if(!Seq.empty())
						{
							auto P = Seq[0].Consume<std::vector<Element>>();
							if (!Result.Insert(Input, P.data(), P.size()))
								Result = {};
						}
					}break;
					case 3:
					{
						Result.style = Style::Relative;
						auto P = Ele[0].Consume<std::vector<Element>>();
						if(!Result.Insert(Input, P.data(), P.size()))
							Result = {};
					}break;
					case 4:
					{
						Result.style = Style::Mapping;
						auto Temp = Ele[0].Consume<Element>();
						Result.elements.push_back(Temp);
						Result.path = Temp(Input);
						auto Seq = Ele[1].Consume<EbnfSequencer>();
						if (!Seq.empty())
						{
							auto P = Seq[0].Consume<std::vector<Element>>();
							if (!Result.Insert(Input, P.data(), P.size()))
								Result = {};
						}
					}break;
					case 5:
					{
						Result.style = Style::UnixAbsolute;
						auto P = Ele[1].Consume<std::vector<Element>>();
						P[0].type = ElementStyle::Root;
						P[0].start -=1;
						P[0].size +=2;
						Result.elements.push_back(P[0]);
						Result.path = P[0](Input);
						if(!Result.Insert(Input, P.data() + 1, P.size() - 1))
							Result = {};
					}break;
					}
					return {};
			},
			[&](EbnfTElement& Ele) -> std::any
			{
				switch(Ele.mask)
				{
				case 0:
					return Element{ ElementStyle::File, static_cast<size_t>(Ele.capture.data() - Input.data()), Ele.capture.size() };
				case 1:
					return Element{ ElementStyle::Root, static_cast<size_t>(Ele.capture.data() - Input.data()), Ele.capture.size() };
				case 2:
					return Element{ ElementStyle::Self, static_cast<size_t>(Ele.capture.data() - Input.data()), Ele.capture.size() };
				case 3:
					return Element{ ElementStyle::Upper, static_cast<size_t>(Ele.capture.data() - Input.data()), Ele.capture.size() };
				case 4:
					return Element{ ElementStyle::Root, static_cast<size_t>(Ele.capture.data() - Input.data()), Ele.capture.size() };
				default:
					return {};
				}
			}
			);
			*this = std::move(Result);
		}catch (Exception::EbnfInterface const&)
		{
			
		}
	}

	Path Path::Append(Path const& Input) const
	{
		if(*this)
		{
			if(Input.style == Style::Relative)
			{
				Path result = *this;
				if(result.Insert(Input.path, Input.elements.data(), Input.elements.size()))
					return result;
			}
			return {};
		}else
		{
			return Input;
		}
		
	}

	bool Path::operator==(Path const& path) const noexcept
	{
		return GetType() == path.GetType() && elements.size() == path.elements.size() && this->path == path.path;
	}

	Path Path::Parent() const
	{
		if(elements.size() >= 2)
		{
			Path result;
			result.style = style;
			auto ref = *(elements.rbegin() + 1);
			result.path.insert(result.path.end(), path.begin(), path.begin() + ref.start + ref.size);
			result.elements.insert(result.elements.end(), elements.begin(), elements.begin() + (elements.size() - 1));
			return result;
		}
		return {};
	}

	std::u32string_view Path::Filename() const
	{
		if (elements.size() > 0)
		{
			auto ref = *elements.rbegin();
			return ref(path);
		}
		return {};
	}

	std::u32string_view Path::FilenameWithoutExtension() const
	{
		return std::get<0>(FilenameWithoutExtensionAndExtension());
	}
	std::u32string_view Path::Extension() const
	{
		return std::get<1>(FilenameWithoutExtensionAndExtension());
	}

	std::tuple<std::u32string_view, std::u32string_view> Path::FilenameWithoutExtensionAndExtension() const
	{
		auto result = Filename();
		auto file_index = result.find_last_of(U'.');
		return { result.substr(0, file_index), result.substr(file_index + 1) };
	}
	
	bool PathMapping::Regedit(std::u32string_view name, Path path)
	{
		if(path.GetType() == Path::Style::DosAbsolute || path.GetType() == Path::Style::UnixAbsolute)
		{
			std::unique_lock lg(mutex);
			auto re = mapping.insert({std::u32string(name), std::move(path)});
			return re.second;
		}
		return false;
	}

	void PathMapping::Unregedit(std::u32string_view name)
	{
		std::unique_lock lg(mutex);
		mapping.erase(std::u32string(name));
	}

	Path PathMapping::operator()(Path const& path) const
	{
		if(path.GetType() == Path::Style::Mapping)
		{
			assert(path.Size() >= 1);
			std::shared_lock lg(mutex);
			auto P = path[0];
			auto re = mapping.find(std::u32string{P.data() + 1, P.size() - 3});
			if(re != mapping.end())
			{
				auto result = re->second;
				if(result.Insert(path.path, path.elements.data() + 1, path.elements.size() - 1))
					return result;
			}
		}
		return {};
	}

	PathMapping& PathMapping::GobalPathMapping()
	{
		static PathMapping instance;
		return instance;
	}

	Path Path::Current()
	{
		auto CurPath = std::filesystem::current_path().u32string();
		std::replace(CurPath.begin(), CurPath.end(), U'\\', U'/');
		return Path(CurPath);
	}

	Path Path::FindCurentDirectory(std::function<bool(Path&)> funcobj) const
	{
		if (funcobj && IsAbsolute())
		{
			for (auto& ite : std::filesystem::directory_iterator(path, std::filesystem::directory_options::skip_permission_denied))
			{
				auto CurPath = ite.path().u32string();
				std::replace(CurPath.begin(), CurPath.end(), U'\\', U'/');
				Path result(CurPath);
				if (funcobj(result))
					return result;
			}
		}
		return {};
	}

	Path Path::FindChildDirectory(std::function<bool(Path&)> funcobj, size_t stack) const
	{
		if (funcobj && stack > 0 && IsAbsolute())
		{
			std::vector<std::tuple<size_t, std::filesystem::path>> searching_stack;
			searching_stack.push_back({stack, path});
			while (!searching_stack.empty())
			{
				auto [stack, path] = *searching_stack.rbegin();
				searching_stack.pop_back();
				if (stack > 0)
				{
					stack -= 1;
					for (auto& ite : std::filesystem::directory_iterator(path, std::filesystem::directory_options::skip_permission_denied))
					{
						auto sys_path = ite.path();
						auto user_path = sys_path.u32string();
						std::replace(user_path.begin(), user_path.end(), U'\\', U'/');
						Path result(user_path);
						if (funcobj(result))
							return result;
						if (stack > 0 && std::filesystem::is_directory(sys_path))
							searching_stack.insert(searching_stack.begin(), { stack, std::move(sys_path) });
					}
				}
			}
		}
		return {};
	}

	Path Path::FindParentDirectory(std::function<bool(Path&)> funcobj, size_t stack) const
	{
		if (funcobj && stack > 0 && IsAbsolute())
		{
			std::filesystem::path searching_stack{path};
			auto root = searching_stack.root_directory();
			while (true)
			{
				for (auto& ite : std::filesystem::directory_iterator(searching_stack, std::filesystem::directory_options::skip_permission_denied))
				{
					auto sys_path = ite.path();
					auto user_path = sys_path.u32string();
					std::replace(user_path.begin(), user_path.end(), U'\\', U'/');
					Path result(user_path);
					if (funcobj(result))
						return result;
				}
				if (stack > 0 && searching_stack != root)
				{
					--stack;
					searching_stack = searching_stack.parent_path();
				}else
					break;
			}
		}
		return {};
	}

	std::vector<Path> Path::FindAllCurentDirectory(std::function<bool(Path&)> funcobj) const
	{
		std::vector<Path> result;
		FindCurentDirectory([&](Path& pa) -> bool {
			if(funcobj(pa))
				result.push_back(pa);
			return false;
		});
		return result;
	}
	std::vector<Path> Path::FindAllChildDirectory(std::function<bool(Path&)> funcobj, size_t stack) const
	{
		std::vector<Path> result;
		FindChildDirectory([&](Path& pa) -> bool {
			if (funcobj(pa))
				result.push_back(pa);
			return false;
		}, stack);
		return result;
	}
	std::vector<Path> Path::FindAllParentDirectory(std::function<bool(Path&)> funcobj, size_t stack) const
	{
		std::vector<Path> result;
		FindParentDirectory([&](Path& pa) -> bool {
			if (funcobj(pa))
				result.push_back(pa);
			return false;
		}, stack);
		return result;
	}

	size_t Path::LocateFile(std::u32string_view tar) const
	{
		size_t i = 0;
		for(auto& ite : elements)
		{
			if(ite(path) == tar)
				return i;
			++i;
		}
		return i;
	}

	Path Path::SubPath(size_t start, size_t count) const
	{
		start = std::min(start, Size());
		count = std::min(Size() - start, count);
		Path result;
		result.elements.insert(result.elements.end(), elements.begin() + start, elements.begin() + start + count);
		if (!result.elements.empty())
		{
			auto star = result.elements.begin();
			auto end = result.elements.rbegin();
			result.path.insert(result.path.end(), path.begin() + star->start, path.begin() + end->start + end->size);
			size_t offset = star->start;
			for (auto& ite : result.elements)
				ite.start -= offset;
			if (start == 0)
				result.style = style;
			else
				result.style = Path::Style::Relative;
		}
		return result;
	}

	std::vector<std::byte> Path::LoadEntireFile(Path const& ref)
	{
		if (ref.GetType() == Path::Style::DosAbsolute || ref.GetType() == Path::Style::UnixAbsolute)
		{
			std::filesystem::path tar(ref.ToString<char32_t>());
			std::ifstream file(tar, std::ios::binary);
			if (file.good()) 
			{
				size_t require_size = std::filesystem::file_size(tar);
				std::vector<std::byte> datas;
				datas.resize(require_size);
				file.read(reinterpret_cast<char*>(datas.data()), require_size);
				return datas;
			}
		}
		return {};
	}

	bool Path::SaveFile(Path const& ref, std::span<std::byte const> data)
	{
		if (ref.GetType() == Path::Style::DosAbsolute || ref.GetType() == Path::Style::UnixAbsolute)
		{
			std::filesystem::path tar(ref.ToString<wchar_t>());
			std::ofstream file(tar, std::ios::binary);
			if (file.good())
			{
				file.write(reinterpret_cast<char const*>(data.data()), data.size());
				return true;
			}
		}
		return false;
	}

	struct LocalFile : Implement::FileInterface
	{
		Potato::AtomicRefCount ref;
	};
	*/

}