#define TOML_HEADER_ONLY 1
#include <fmt/format.h>
#include <iostream>
#include <toml.hpp>
int main(int argc, char **argv) {
  // read config file path, for example: "./config.toml"
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
    return 1;
  }
  auto config_path = argv[1];
  toml::table tbl;
  // build the config table
  try {
    tbl = toml::parse_file(config_path);
    std::cout << tbl << "\n";
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }


  
  // get the config value
  auto numers = tbl["numbers"];
  std::cout << numers << std::endl;

  auto number=tbl["number"];
  int &num=number.ref<int>();

}