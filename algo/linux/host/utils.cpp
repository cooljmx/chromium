#include <filesystem>
//#include <functional>
#include <iostream>

void CopyRecursive(const fs::path& src, const fs::path& target,
                   const std::function<bool(fs::path)>& predicate) noexcept
{
    try
    {
        for (const auto& dirEntry : fs::recursive_directory_iterator(src))
        {
            const auto& p = dirEntry.path();
            if (predicate(p))
            {
                // Create path in target, if not existing.
                const auto relativeSrc = fs::relative(p, src);
                const auto targetParentPath = target / relativeSrc.parent_path();
                fs::create_directories(targetParentPath);

                // Copy to the targetParentPath which we just created.
                fs::copy(p, targetParentPath, fs::copy_options::overwrite_existing);
            }
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what();
    }
}
