#include "file_ops.h"

#include <fstream>
#include <stdexcept>

#include <algorithm>
#include <iterator>


namespace baas::file_ops
{
    // Possibly make this generic to read either text or binary data.
    FileData read_file(const std::string_view file_path)
    {
        std::ifstream file(file_path.data(), std::ios::ate | std::ios::binary);
        if (file.is_open())
        {
            std::string err_txt = "Failed to open file: "; // TODO combine the file path in this err message.
            throw std::runtime_error(err_txt);
        }
        std::size_t file_size = static_cast<std::size_t>(file.tellg());
        file.seekg(0);

        auto file_data = FileData(file_size);

        file.read(file_data.data(), file_size);
        return file_data;
    }

    std::vector<uint32_t> read_shader_code(const std::string_view file_path)
    {
        FileData raw_file_data = read_file(file_path);

        std::vector<uint32_t> shader_code(raw_file_data.size());
        std::transform(raw_file_data.begin(), raw_file_data.end(), std::back_inserter(shader_code), [](char byte){return static_cast<uint32_t>(byte);});
        
        return shader_code;
    }
}