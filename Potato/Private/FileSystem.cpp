#include "../Public/FileSystem.h"
#include "../Public/Ebnf.h"
#include <filesystem>
#include <fstream>

namespace
{
	Potato::Ebnf::Table const& FSEbnf()
	{
		static Potato::Ebnf::Table fs_ebnf = Potato::Ebnf::CreateTable(
UR"(
NormalPath := '[^/]*' : [0]
DosRoot := '[a-zA-Z\-0-9_]*:/' : [1]
Delimiter := '/'
SelfPath := '\.' : [2]
UpperPath := '\.\.' : [3]
MappingRoot := '$[a-zA-Z\-0-9_ ]+:/' : [4]
%%%
$ := <Ste>
<File> := (NormalPath | SelfPath | UpperPath) : [0]
<FilePath> := <File> { Delimiter <File> } : [1]
<Ste> := DosRoot [ <FilePath> ] : [2]
<Ste> := <FilePath> : [3]
<Ste> := MappingRoot [<FilePath>] : [4]
<Ste> := Delimiter <FilePath> Delimiter : [5]
)"
		);
		return fs_ebnf;
	}

	std::u32string_view Self = UR"(.)";
	std::u32string_view Upper = UR"(..)";
}

namespace Potato::FileSystem
{


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
		Ebnf::Table const& ref = FSEbnf();
		try
		{
			Path Result;
			auto His = Ebnf::Process(ref, Input);
			His([&](Ebnf::NTElement& Ele) -> std::any
			{
				switch (Ele.mask)
					{
					case 0:
					{
						return Ele[0].MoveRawData();
					}
					case 1:
					{
						std::vector<Element> Res;
						for(size_t i = 0; i < Ele.production_count; ++i)
						{
							if(Ele[i].IsNoterminal())
							{
								auto P = Ele[i].GetData<Element>();
								Res.push_back(std::move(P));
							}	
						}
						return std::move(Res);
					}
					case 2:
					{
						Result.style = Style::DosAbsolute;
						auto Temp = Ele[0].GetData<Element>();
						Result.elements.push_back(Temp);
						Result.path = Temp(Input);
						if(Ele.production_count >= 2)
						{
							auto P = Ele[1].MoveData<std::vector<Element>>();
							if (!Result.Insert(Input, P.data(), P.size()))
								Result = {};
						}
					}break;
					case 3:
					{
						Result.style = Style::Relative;
						auto P = Ele[0].MoveData<std::vector<Element>>();
						if(!Result.Insert(Input, P.data(), P.size()))
							Result = {};
					}break;
					case 4:
					{
						Result.style = Style::Mapping;
						auto Temp = Ele[0].GetData<Element>();
						Result.elements.push_back(Temp);
						Result.path = Temp(Input);
						if (Ele.production_count >= 2)
						{
							auto P = Ele[1].MoveData<std::vector<Element>>();
							if(!Result.Insert(Input, P.data(), P.size()))
								Result = {};
						}
					}break;
					case 5:
					{
						Result.style = Style::UnixAbsolute;
						auto P = Ele[1].MoveData<std::vector<Element>>();
						P[0].type = ElementStyle::Root;
						P[0].start -=1;
						P[0].size +=2;
						Result.elements.push_back(P[0]);
						Result.path = P[0](Input);
						if(!Result.Insert(Input, P.data() + 1, P.size()))
							Result = {};
					}break;
					}
					return {};
			},
			[&](Ebnf::TElement& Ele) -> std::any
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
		}catch (Exception::Ebnf::Interface const&)
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

	Path Path::Parent() const
	{
		if(elements.size() >= 2)
		{
			Path result;
			result.style = style;
			auto ref = elements[elements.size() - 2];
			result.path.insert(result.path.end(), path.begin(), path.begin() + ref.start + ref.size);
			result.elements.insert(result.elements.end(), elements.begin(), elements.begin() + (elements.size() - 1));
			return result;
		}
		return {};
	}

	std::u32string_view Path::Last() const
	{
		if(elements.size() > 0)
		{
			auto ref = elements[elements.size() - 1];
			return ref(path);
		}
		return {};
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

	PathMapping& GobalPathMapping()
	{
		static PathMapping instance;
		return instance;
	}

	Path Current()
	{
		auto CurPath = std::filesystem::current_path().u32string();
		std::replace(CurPath.begin(), CurPath.end(), U'\\', U'/');
		return Path(CurPath);
	}

	Path Path::FindFileFromParent(std::u32string_view Target, size_t MaxStack) const
	{
		if (GetType() == Path::Style::DosAbsolute || GetType() == Path::Style::UnixAbsolute)
		{
			std::filesystem::path tar(Target);
			size_t SearchingStack = 0;
			while (SearchingStack < MaxStack && Size() > SearchingStack + 1)
			{
				auto Last = elements[Size() - SearchingStack - 1];
				std::u32string_view cur(path.data(), Last.start + Last.size);
				for (auto& ite : std::filesystem::directory_iterator(cur, std::filesystem::directory_options::skip_permission_denied))
				{
					if (ite.path().filename() == tar)
					{
						auto CurPath = ite.path().u32string();
						std::replace(CurPath.begin(), CurPath.end(), U'\\', U'/');
						return Path(CurPath);
					}
				}
				++SearchingStack;
			}
		}
		return {};
	}

	Path Path::FindFileFromChild(std::u32string_view Target) const
	{
		if (GetType() == Path::Style::DosAbsolute || GetType() == Path::Style::UnixAbsolute)
		{
			std::filesystem::path tar(Target);
			for (auto& ite : std::filesystem::recursive_directory_iterator(path, std::filesystem::directory_options::skip_permission_denied))
			{
				if (ite.path().filename() == tar)
				{
					auto CurPath = ite.path().u32string();
					std::replace(CurPath.begin(), CurPath.end(), U'\\', U'/');
					return Path(CurPath);
				}
			}
		}
		return {};
	}

	std::vector<Path> Path::FindAllFileFromChild(std::u32string_view Target) const
	{
		std::vector<Path> paths;
		if (GetType() == Path::Style::DosAbsolute || GetType() == Path::Style::UnixAbsolute)
		{
			std::filesystem::path tar(Target);
			for (auto& ite : std::filesystem::recursive_directory_iterator(path, std::filesystem::directory_options::skip_permission_denied))
			{
				if (ite.path().filename() == tar)
				{
					auto CurPath = ite.path().u32string();
					std::replace(CurPath.begin(), CurPath.end(), U'\\', U'/');
					paths.push_back(Path(CurPath));
				}
			}
		}
		return std::move(paths);
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
		if(count > 0)
		{
			Path result;
			result.elements.insert(result.elements.end(), elements.begin() + start, elements.begin() + start + count);
			auto s = elements[start];
			auto e = elements[start + count];
			result.path.insert(result.path.end(), path.begin() + s.start, path.begin() + e.start + e.size);
			for(auto& ite : result.elements)
				ite.start -= s.start;
			if(start == 0)
				result.style = style;
			else
				result.style = Path::Style::Relative;
			return std::move(result);
		}
		return {};		
	}

	std::vector<std::byte> LoadEntireFile(Path const& ref)
	{
		if (ref.GetType() == Path::Style::DosAbsolute || ref.GetType() == Path::Style::UnixAbsolute)
		{
			std::filesystem::path tar(ref.ToU32String());
			std::ifstream file(tar, std::ios::binary);
			if (file.good()) 
			{
				size_t require_size = std::filesystem::file_size(tar);
				std::vector<std::byte> datas;
				datas.resize(require_size);
				file.read(reinterpret_cast<char*>(datas.data()), require_size);
				return std::move(datas);
			}
		}
		return {};
	}

	bool SaveFile(Path const& ref, std::byte const* data, size_t size)
	{
		if (ref.GetType() == Path::Style::DosAbsolute || ref.GetType() == Path::Style::UnixAbsolute)
		{
			std::filesystem::path tar(ref.ToU32String());
			std::ofstream file(tar, std::ios::binary);
			if (file.good())
			{
				file.write(reinterpret_cast<char const*>(data), size);
				return true;
			}
		}
		return false;
	}

	struct LocalFile : Implement::FileInterface
	{
		Potato::AtomicRefCount ref;
	};

}