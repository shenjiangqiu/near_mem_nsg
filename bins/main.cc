#define TOML_HEADER_ONLY 1
#include <iostream>
#include <toml.hpp>
int main(int argc, char **argv) {

  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
    return 1;
  }
  auto config_path = argv[1];
  toml::table tbl;
  try {
    tbl = toml::parse_file(config_path);
    std::cout << tbl << "\n";
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

}