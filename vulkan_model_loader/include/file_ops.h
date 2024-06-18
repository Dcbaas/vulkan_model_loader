#ifndef FILE_OPS_H
#define FILE_OPS_H

#include <vector>
#include <string>

namespace baas::file_ops
{
    using FileData = std::vector<char>;

    FileData read_file(const std::string_view file_path);

    std::vector<uint32_t> read_shader_code(const std::string_view file_path);

}
#endif // !FILE_OPS_H
