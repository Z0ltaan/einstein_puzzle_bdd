#ifndef CLI_HPP
#define CLI_HPP

#include <filesystem>

namespace course {
struct config {
  bool split_allowed;
  size_t object_count;
  size_t properties_count;

  size_t node_number;
  size_t cache_size;

  std::filesystem::path constraint_config_path;
};

void handle_command_line_args(int argc, char **argv, config & config);
} // namespace course

#endif
