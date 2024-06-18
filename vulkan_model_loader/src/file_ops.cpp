#include "file_ops.h"

#include <fstream>
#include <stdexcept>

namespace baas::file_ops
{
    // Possibly make this generic to read either text or binary data.
    FileData read_file(const std::string_view file_path)
    {
        std::ifstream file(file_path.data(), std::ios::ate | std::ios::binary);
        if (file.is_open())
        {
            throw std::runtime_error("Failed to open file!");
        }
        std::size_t file_size = static_cast<std::size_t>(file.tellg());
        file.seekg(0);

        auto file_data = FileData(file_size);

        file.read(file_data.data(), file_size);
        return file_data;
    }
}