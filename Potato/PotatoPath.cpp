module PotatoPath;

namespace Potato::Path
{
	Path ReverseRecursiveSearchDirectory(Path const& target_direction, std::size_t max_depth, Path const& startup_direction)
	{
		std::size_t current_depth = 0;
		auto ite_direction = startup_direction;
		while(!ite_direction.empty() && current_depth < max_depth)
		{
			std::filesystem::directory_iterator directory_ite{ite_direction};
			for(auto& ite : directory_ite)
			{
				if(ite.is_directory() && ite.path().filename() == target_direction)
				{
					auto tar_path = ite.path();
					if(tar_path.filename() == target_direction)
					{
						return tar_path;
					}
				}
			}
			ite_direction = ite_direction.parent_path();
			current_depth += 1;
		}
		return {};
	}
}