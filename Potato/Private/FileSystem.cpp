#include "../Public/FileSystem.h"
#include "../Public/Ebnf.h"
#include "../Public/StrEncode.h"

namespace
{
	Potato::Ebnf::Table const& FSEbnf()
	{
		static Potato::Ebnf::Table fs_ebnf = Potato::Ebnf::CreateTable(
UR"(
NormalPath := '[^/\:]*' : [0]
DosRoot := '[a-zA-Z\-0-9_]*:' : [0]
Delimiter := '/'
SelfPath := '\.' : [1]
UpperPath := '\.\.' : [2]
MappingRoot := '$[a-zA-Z\-0-9_ ]+:' : [0]
%%%
$ := <Ste>
<File> := (NormalPath | SelfPath | UpperPath) : [0]
<FilePath> := <File> { Delimiter <File> } : [1]
<Ste> := DosRoot Delimiter [ <FilePath> ] : [2]
<Ste> := <FilePath> : [3]
<Ste> := MappingRoot Delimiter [<FilePath>] : [4]
<Ste> := Delimiter <FilePath> : [5]
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
			
		}


		
		size_t Min = (style == Style::Relative ? 0 : 1);
		for (size_t i = 0; i < length; ++i)
		{
			auto ite = tup[i];
			if (ite == StrTup::MakeSelf())
			{
				if (result.path_ite.size() == 0)
					result.path_ite.push_back(ite);
			}
			else if (ite == StrTup::MakeUpper())
			{
				if (result.path_ite.size() > Min)
				{
					auto last = result.path_ite[result.path_ite.size() - 1];
					if (last.IsStr())
					{
						result.path.resize(result.path.size() - last.size);
						result.path_ite.pop_back();
					}
					else if (last == StrTup::MakeSelf())
					{
						result.path_ite.pop_back();
						result.path_ite.push_back(ite);
					}
					else if (last == StrTup::MakeUpper())
						result.path_ite.push_back(ite);
				}
				else
					result.path_ite.push_back(ite);
			}
			else
				result.Insert(ref, ite);
		}
	}
	
	void Path::Insert(std::u32string_view Table, Ele ref)
	{
		if (ref.IsStr())
		{
			size_t old = path.size();
			path += ref(Table);
			path_ite.push_back({ old, ref.size });
		}else
		{
			path_ite.push_back(ref);
		}
	}

	void Path::Insert(std::u32string_view Table, std::vector<StrTup> const& ref)
	{
		for(auto& ite : ref)
		{
			Insert(Table, ite);
		}
	}
	
	Path::Path(std::u32string_view Input)
	{
		Ebnf::Table const& ref = FSEbnf();
		try
		{
			Path Result;
			auto His = Ebnf::Process(ref, Input);
			His([&](Ebnf::Element& Ele) -> std::any
			{
				if(Ele.IsTerminal())
				{
					switch(Ele.shift.mask)
					{
					case 0:
						return StrTup{ static_cast<size_t>(Ele.shift.capture.data()- Input.data()), Ele.shift.capture.size() };
					case 1:
						return StrTup::MakeSelf();
					case 2:
						return StrTup::MakeUpper();
					default:
						return {};
					}
				}else if(Ele.IsNoterminal())
				{
					switch (Ele.reduce.mask)
					{
					case 0:
					{
						return Ele[0].MoveRawData();
					}
					case 1:
					{
						std::vector<StrTup> Res;
						for(size_t i = 0; i < Ele.reduce.production_count; ++i)
						{
							if(Ele[i].IsNoterminal())
							{
								auto P = Ele[i].GetData<StrTup>();
								if(P == StrTup::MakeSelf())
								{
									if(Res.empty())
										Res.push_back(P);
								}else if(P == StrTup::MakeUpper())
								{
									if(Res.empty())
										Res.push_back(P);
									else
										Res.pop_back();
								}else
									Res.push_back(P);
							}
								
						}
						return std::move(Res);
					}
					case 2:
					{
						Result.style = Style::DosAbsolute;
						Result.Insert(Input, Ele[0].GetData<StrTup>());
						if(Ele.reduce.production_count >= 3)
						{
							auto P = Ele[2].MoveData<std::vector<StrTup>>();
							Result.Insert(Input, P);
						}
					}break;
					case 3:
					{
						Result.style = Style::Relative;
						auto P = Ele[0].MoveData<std::vector<StrTup>>();
						Result.Insert(Input, P);
					}break;
					case 4:
					{
						Result.style = Style::Mapping;
						Result.Insert(Input, Ele[0].GetData<StrTup>());
						if(Ele.reduce.production_count == 3)
							Result.Insert(Input, Ele[2].GetData<std::vector<StrTup>&>());
					}break;
					case 5:
					{
						Result.style = Style::UnixAbsolute;
						auto P = Ele[1].MoveData<std::vector<StrTup>>();
						Result.Insert(Input, P);
					}break;
					}
					return {};
				}
				return {};
			});
			*this = std::move(Result);
		}catch (Ebnf::Exception::Interface const&)
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
				if(AppendImp(result, Input.path, Input.path_ite.data(), Input.path_ite.size()))
					return result;
			}
			return {};
		}else
		{
			return Input;
		}
		
	}

	bool Path::AppendImp(Path& result, std::u32string const& ref, StrTup const* tup, size_t length)
	{
		size_t Min = (result.style == Style::Relative ? 0 : 1);
		for (size_t i =0; i < length; ++ i)
		{
			auto ite = tup[i];
			if (ite == StrTup::MakeSelf())
			{
				if (result.path_ite.size() == 0)
					result.path_ite.push_back(ite);
			}
			else if (ite == StrTup::MakeUpper())
			{
				if (result.path_ite.size() > Min)
				{
					auto last = result.path_ite[result.path_ite.size() - 1];
					if (last.IsStr())
					{
						result.path.resize(result.path.size() - last.size);
						result.path_ite.pop_back();
					}
					else if (last == StrTup::MakeSelf())
					{
						result.path_ite.pop_back();
						result.path_ite.push_back(ite);
					}
					else if (last == StrTup::MakeUpper())
						result.path_ite.push_back(ite);
				}
				else
					result.path_ite.push_back(ite);
			}
			else
				result.Insert(ref, ite);
		}
		switch (result.style)
		{
		case Style::Relative:
			return true;
		case Style::Mapping:
		case Style::DosAbsolute:
		case Style::UnixAbsolute:
		{
			if (result.path_ite.size() == 1 || result.path_ite[1].IsStr())
				return true;
		}
		};
		return false;
	}

	std::u32string Path::ToU32String() const
	{
		std::u32string re;
		size_t require_size = 0;
		for(auto& ite : path_ite)
			require_size+=ite(path).size();
		re.reserve(require_size);
		for (auto& ite : path_ite)
			re += ite(path);
		return std::move(re);
	}

	std::u16string Path::ToU16String() const
	{
		std::u16string re;
		size_t require_size = 0;
		for (auto& ite : path_ite)
		{
			auto str = ite(path);
			require_size += StrEncode::Encode<char32_t, char16_t>{}.Request(str.data(), str.size()).require_length;
		}
		re.resize(require_size);
		size_t index = 0;
		for (auto& ite : path_ite)
		{
			auto str = ite(path);
			index += StrEncode::Encode<char32_t, char16_t>{}.Decode(str.data(), str.size(), re.data() + index, require_size - index).used_target_length;
		}
		return std::move(re);
	}

	std::u8string Path::ToU8String() const
	{
		if(*this)
		{
			std::u8string re;
			size_t require_size = 0;
			for (auto& ite : path_ite)
			{
				auto str = ite(path);
				require_size += StrEncode::Encode<char32_t, char8_t>{}.Request(str.data(), str.size()).require_length;
			}
			re.resize(require_size);
			size_t index = 0;
			for (auto& ite : path_ite)
			{
				auto str = ite(path);
				index += StrEncode::Encode<char32_t, char8_t>{}.Decode(str.data(), str.size(), re.data() + index, require_size - index).used_target_length;
			}
			return std::move(re);
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
			auto re = mapping.find(std::u32string{P.data() + 1, P.size() - 2});
			if(re != mapping.end())
			{
				auto result = re->second;
				if(Path::AppendImp(result, path.path, path.path_ite.data() + 1, path.path_ite.size() - 1))
					return result;
			}
		}
		return path;
	}

	PathMapping& GobalPathMapping()
	{
		static PathMapping instance;
		return instance;
	}

}