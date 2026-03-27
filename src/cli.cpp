#include "cli.hpp"
#include <CLI11.hpp>

void course::handle_command_line_args(int argc, char **argv, config &config) {
  CLI::App cli_args_controller{"Modified Einstein puzzle solver"};
  argv = cli_args_controller.ensure_utf8(argv);

  cli_args_controller.add_option(
      "-o, --objects", config.object_count,
      "Set object count (should be representable as a square; defaults to 9)");

  cli_args_controller.add_option(
      "-p, --properties", config.properties_count,
      "Set properties count (should be < object count; defaults 4)");

  cli_args_controller.add_option(
      "-n, --nodes", config.node_number,
      "Set node number for the BuDDy library (defaults to )");
  cli_args_controller.add_option("-c, --cache", config.cache_size,
                                 "Set cache size for the BuDDy library");

  cli_args_controller.add_option("-r, --config", config.constraint_config_path,
                                 "Set path to constraints.ini config (by "
                                 "default config is expected to be in the same "
                                 "directory as a binary)");

  cli_args_controller.add_flag("-s, --split",
                               "Enable gluing for neighbours calculation");

  cli_args_controller.parse(argc, argv);
}
