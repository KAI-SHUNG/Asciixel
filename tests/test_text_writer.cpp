#include "asciixel/io/text_writer.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 2) throw std::runtime_error("Expected output path");
    const auto path = std::filesystem::u8path(argv[1]);
    std::filesystem::remove(path);
    asciixel::AsciiFrame frame(3, 2);
    const std::string chars = "A  #. ";
    for (std::size_t i = 0; i < chars.size(); ++i) frame.pixels[i].character = chars[i];
    asciixel::writeAsciiFile(frame, argv[1]);
    bool rejected = false;
    try { asciixel::writeAsciiFile(frame, argv[1]); }
    catch (const std::runtime_error&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Existing output must be preserved");
    std::ifstream input(path, std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(input)), {});
    if (bytes != "A  \n#. \n") throw std::runtime_error("TXT bytes differ: trailing spaces/LF/BOM");
    input.close();
    std::filesystem::remove(path);
}
