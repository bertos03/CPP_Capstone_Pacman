#include "paths.h"

#include <SDL.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
#include <unistd.h>

namespace {

namespace fs = std::filesystem;

std::string ConsumeSdlPath(char *raw_path) {
  if (raw_path == nullptr) {
    return "";
  }

  std::string path(raw_path);
  SDL_free(raw_path);
  return path;
}

fs::path NormalizePath(const fs::path &path) {
  if (path.empty()) {
    return {};
  }

  std::error_code error;
  const fs::path normalized = fs::weakly_canonical(path, error);
  if (!error) {
    return normalized;
  }
  return path.lexically_normal();
}

bool IsExistingDirectory(const fs::path &path) {
  std::error_code error;
  return fs::exists(path, error) && fs::is_directory(path, error);
}

bool IsExistingRegularFile(const fs::path &path) {
  std::error_code error;
  return fs::exists(path, error) && fs::is_regular_file(path, error);
}

bool EnsureDirectoryIsWritable(const fs::path &path) {
  if (path.empty()) {
    return false;
  }

  std::error_code create_error;
  fs::create_directories(path, create_error);
  if (create_error) {
    return false;
  }

  return ::access(path.c_str(), W_OK) == 0;
}

void AppendUniquePath(std::vector<fs::path> &paths, const fs::path &candidate) {
  if (candidate.empty()) {
    return;
  }

  const fs::path normalized_candidate = NormalizePath(candidate);
  const auto already_present =
      std::find(paths.begin(), paths.end(), normalized_candidate);
  if (already_present == paths.end()) {
    paths.push_back(normalized_candidate);
  }
}

fs::path GetExecutableBasePath() {
  static const fs::path executable_base_path = [] {
    return NormalizePath(fs::path(ConsumeSdlPath(SDL_GetBasePath())));
  }();
  return executable_base_path;
}

fs::path GetSourceRootPath() {
#ifdef PACMAN_SOURCE_DIR
  static const fs::path source_root = NormalizePath(fs::path(PACMAN_SOURCE_DIR));
  return source_root;
#else
  return {};
#endif
}

bool IsPathWithin(const fs::path &path, const fs::path &base_directory) {
  if (path.empty() || base_directory.empty()) {
    return false;
  }

  const fs::path normalized_path = NormalizePath(path);
  const fs::path normalized_base = NormalizePath(base_directory);
  const auto mismatch = std::mismatch(normalized_base.begin(),
                                      normalized_base.end(),
                                      normalized_path.begin(),
                                      normalized_path.end());
  return mismatch.first == normalized_base.end();
}

std::vector<fs::path> BuildDataDirectoryCandidates() {
  std::vector<fs::path> candidates;
  const fs::path executable_base = GetExecutableBasePath();
  if (!executable_base.empty()) {
    AppendUniquePath(candidates, executable_base / ".." / "Resources" / "data");
    AppendUniquePath(candidates, executable_base / ".." / "data");
    AppendUniquePath(candidates,
                     executable_base / ".." / "share" / "bobman" / "data");
  }

  std::error_code current_dir_error;
  const fs::path current_directory = fs::current_path(current_dir_error);
  if (!current_dir_error) {
    AppendUniquePath(candidates, current_directory / "data");
  }

  const fs::path source_root = GetSourceRootPath();
  if (!source_root.empty()) {
    AppendUniquePath(candidates, source_root / "data");
  }

  return candidates;
}

const fs::path &ResolveDataDirectory() {
  static const fs::path data_directory = [] {
    const std::vector<fs::path> candidates = BuildDataDirectoryCandidates();
    for (const fs::path &candidate : candidates) {
      if (IsExistingDirectory(candidate)) {
        return candidate;
      }
    }

    if (!candidates.empty()) {
      return candidates.front();
    }
    return fs::path("data");
  }();
  return data_directory;
}

const fs::path &ResolveWritableMapsDirectory() {
  static const fs::path writable_maps_directory = [] {
    const fs::path source_root = GetSourceRootPath();
    const fs::path executable_base = GetExecutableBasePath();
    const fs::path bundled_maps_directory =
        NormalizePath(ResolveDataDirectory() / "maps");

    if (!source_root.empty() && !executable_base.empty() &&
        IsPathWithin(executable_base, source_root)) {
      const fs::path source_maps_directory =
          NormalizePath(source_root / "data" / "maps");
      EnsureDirectoryIsWritable(source_maps_directory);
      return source_maps_directory;
    }

    if (EnsureDirectoryIsWritable(bundled_maps_directory)) {
      return bundled_maps_directory;
    }

    const std::string preference_root =
        ConsumeSdlPath(SDL_GetPrefPath("BobMan", "BobMan"));
    if (!preference_root.empty()) {
      const fs::path preference_maps_directory =
          NormalizePath(fs::path(preference_root) / "maps");
      if (EnsureDirectoryIsWritable(preference_maps_directory)) {
        return preference_maps_directory;
      }
    }

    EnsureDirectoryIsWritable(bundled_maps_directory);
    return bundled_maps_directory;
  }();
  return writable_maps_directory;
}

std::string PathListToString(const std::vector<std::string> &paths) {
  if (paths.empty()) {
    return "(none)";
  }

  std::ostringstream stream;
  for (size_t i = 0; i < paths.size(); ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << paths[i];
  }
  return stream.str();
}

} // namespace

namespace Paths {

std::string GetDataFilePath(const std::string &relative_path) {
  const fs::path relative(relative_path);
  for (const fs::path &candidate_directory : BuildDataDirectoryCandidates()) {
    const fs::path candidate_file = NormalizePath(candidate_directory / relative);
    if (IsExistingRegularFile(candidate_file)) {
      return candidate_file.string();
    }
  }

  return NormalizePath(ResolveDataDirectory() / relative).string();
}

std::string GetWritableMapsDirectory() {
  return ResolveWritableMapsDirectory().string();
}

std::vector<std::string> GetMapSearchDirectories() {
  std::vector<std::string> directories;
  const fs::path writable_maps_directory = ResolveWritableMapsDirectory();
  if (IsExistingDirectory(writable_maps_directory)) {
    directories.push_back(writable_maps_directory.string());
  }

  const fs::path bundled_maps_directory = ResolveDataDirectory() / "maps";
  if (IsExistingDirectory(bundled_maps_directory) &&
      NormalizePath(bundled_maps_directory) !=
          NormalizePath(writable_maps_directory)) {
    directories.push_back(NormalizePath(bundled_maps_directory).string());
  }

  return directories;
}

std::string DescribeMapSearchDirectories() {
  return PathListToString(GetMapSearchDirectories());
}

bool IsWritableMapPath(const std::string &map_path) {
  return IsPathWithin(fs::path(map_path), ResolveWritableMapsDirectory());
}

} // namespace Paths
