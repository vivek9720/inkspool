#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size);

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: run_one_input <input>\n";
    return 2;
  }
  std::ifstream in(argv[1], std::ios::binary);
  if (!in) {
    std::cerr << "failed to open input\n";
    return 1;
  }
  std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(in),
                                  std::istreambuf_iterator<char>()};
  return LLVMFuzzerTestOneInput(bytes.data(), bytes.size());
}
