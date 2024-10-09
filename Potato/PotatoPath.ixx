export module PotatoPath;
import std;

export namespace Potato::Path
{
	using Path = std::filesystem::path;

	Path ReverseRecursiveSearchDirectory(Path const& target_direction, std::size_t max_depth = std::numeric_limits<std::size_t>::max(), Path const& startup_direction = std::filesystem::current_path());
	
}