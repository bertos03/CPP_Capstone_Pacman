#ifndef PATHS_H
#define PATHS_H

#include <string>
#include <vector>

namespace Paths {

std::string GetDataFilePath(const std::string &relative_path);
std::string GetWritableMapsDirectory();
std::vector<std::string> GetMapSearchDirectories();
std::string DescribeMapSearchDirectories();
bool IsWritableMapPath(const std::string &map_path);

} // namespace Paths

#endif
