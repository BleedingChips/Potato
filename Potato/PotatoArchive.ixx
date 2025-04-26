export module PotatoArchive;
import std;
import PotatoTMP;
import PotatoMemLayout;


export namespace Potato::Misc
{
	struct Archive
	{
		constexpr Archive(std::size_t written_byte, bool is_writing = true) : written_byte(written_byte), is_writing(is_writing){}
		constexpr Archive(Archive const&) = default;
		template<typename OutputType>
		std::size_t operator<<(OutputType& output_type);
	protected:
		constexpr virtual std::span<void> Allocate(MemLayout::Layout layout) = 0;
		std::size_t written_byte = false;
		bool is_writing = true;

		template<typename Type>
		constexpr std::size_t Write(Type&& source_type)
		{
			return Write(std::span(&source_type, 1));
		}
		constexpr std::size_t Write(std::span<Type const> source_type)
		{
			auto written = written_byte;
			auto span = WriteWithoutConstruction(1);
			std::copy_n(&source_type, source_type.size(), span.data());
			return written;
		}
		constexpr std::span<SourceType> WriteWithoutConstruction(std::size_t allocate_size)
		{
			auto span = Allocate(allocate_size);
			written_byte += span;
			return span;
		}

		template<typename FuncT>
		struct Wrapper : public FormatWriter
		{
			constexpr virtual std::span<SourceType> Allocate(std::size_t allocate_size) { return func(allocate_size); }
			constexpr Wrapper(FuncT& func, std::size_t written_byte) : func(func), FormatWriter(written_byte) {}
			constexpr Wrapper(Wrapper const&) = default;
		protected:
			FuncT& func;
		};

		template<typename FuncT>
		auto GetWrapper(FuncT&& funcT, std::size_t written_byte = 0) requires(std::is_invocable_r_v<std::span<SourceType>, FuncT, std::size_t>)
		{
			using Type = std::remove_cvref_t<FuncT>;
			Wrapper<Type> wrapper{ funcT, written_byte };
			return wrapper;
		}
	};



	template<typename SourceType>
	struct FormatWriter
	{
		constexpr FormatWriter(std::size_t written_byte) : written_byte(written_byte) {}
		constexpr FormatWriter(FormatWriter const&) = default;
		constexpr std::size_t Write(SourceType&& source_type)
		{
			return Write(std::span(&source_type, 1));
		}
		constexpr std::size_t Write(std::span<SourceType const> source_type)
		{
			auto written = written_byte;
			auto span = WriteWithoutConstruction(1);
			std::copy_n(&source_type, source_type.size(), span.data());
			return written;
		}
		constexpr std::span<SourceType> WriteWithoutConstruction(std::size_t allocate_size)
		{
			auto span = Allocate(allocate_size);
			written_byte += span;
			return span;
		}

		template<typename FuncT>
		struct Wrapper : public FormatWriter
		{
			constexpr virtual std::span<SourceType> Allocate(std::size_t allocate_size) { return func(allocate_size); }
			constexpr Wrapper(FuncT& func, std::size_t written_byte) : func(func), FormatWriter(written_byte) {}
			constexpr Wrapper(Wrapper const&) = default;
		protected:
			FuncT& func;
		};

		template<typename FuncT>
		auto GetWrapper(FuncT&& funcT, std::size_t written_byte = 0) requires(std::is_invocable_r_v<std::span<SourceType>, FuncT, std::size_t>)
		{
			using Type = std::remove_cvref_t<FuncT>;
			Wrapper<Type> wrapper{ funcT, written_byte };
			return wrapper;
		}

	protected:
		constexpr virtual std::span<SourceType> Allocate(std::size_t allocate_size) = 0;
		std::size_t written_byte = false;
	};
}



export namespace Potato::Log
{
	enum class Level
	{
		Log,
	};

	template<std::size_t N>
	struct TypeStringW : public TMP::TypeString<wchar_t, N>
	{
		using TMP::TypeString<wchar_t, N>::TypeString;
	};

	template<std::size_t N>
	TypeStringW(const wchar_t(&str)[N]) -> Potato::Log::TypeStringW<N>;
}

export namespace Potato::Format
{
	export template<>
	struct Formatter<Log::Level, char8_t>
	{
		bool operator()(Format::FormatWriter<Log::Level>& Writer, std::basic_string_view<char8_t> Parameter, Log::Level const& Input);
	};

	
}



export namespace Potato::Log
{

	



	struct TerminalOutput
	{
		void operator()(std::wstring_view output);
	};

	struct DefaultPrinter
	{
		template<TypeStringW Category, Level Level, TypeStringW Pattern, typename ...Parameters>
		void operator()(Parameters&& ...parameters)
		{
			auto str = Format::StaticFormatToString<Pattern>(std::forward<Parameters>(parameters)...);
			if(str)
			{
				TerminalOutput{}(*str);
			}
		}
	};

	template<TypeStringW Category, Level Level>
	struct LogPrinter
	{
		template<TypeStringW Pattern, typename ...Parameters>
		void operator()(Parameters&& ...parameters)
		{
			DefaultPrinter{}.operator()< Category, Level, Pattern >(std::forward<Parameters>(parameters)...);
		}
	};

	template<TypeStringW Category, Level Level, TypeStringW Pattern, typename ...Parameters>
	void Log(Parameters&& ...parameters)
	{
		LogPrinter<Category, Level>{}.operator()<Pattern>(std::forward<Parameters>(parameters)...);
	}

	template<TypeStringW Category, Level Level, TypeStringW Pattern>
	void Log()
	{
		LogPrinter<Category, Level>{}.operator() < Pattern > ();
	}
}

