/*
* MIT License
*
* Tool to generate C++ files with binary resources (cpp-generes)
*
* Copyright (c) 2022 Golubchikov Mihail <https://github.com/rue-ryuzaki>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include <algorithm>
#include <fstream>
#include <iostream>

#if __cplusplus >= 201703L  // C++17+
#include <filesystem>
#endif  // C++17+

#include <argparse/argparse.hpp>

#include <sys/types.h>
#include <sys/stat.h>

namespace detail {
inline bool
_ends_with(std::string const& s, std::string const& value)
{
#if __cplusplus >= 202002L  // C++20+
    return s.ends_with(value);
#else
    return s.size() >= value.size()
            && 0 == s.compare(s.size() - value.size(), value.size(), value);
#endif  // C++20+
}

inline std::string
_directory_name(std::string const& path)
{
    return path.substr(0, path.find_last_of("/\\"));
}

inline std::string
_file_name(std::string const& path)
{
#if __cplusplus >= 201703L  // C++17+
    return std::filesystem::path(path.c_str()).filename().string();
#else
    return path.substr(path.find_last_of("/\\") + 1);
#endif  // C++17+
}

inline std::string
_replace(std::string s, char old, std::string const& value)
{
    auto pos = s.find(old);
    while (pos != std::string::npos) {
        s.replace(pos, 1, value);
        pos = s.find(old, pos + value.size());
    }
    return s;
}

inline std::string
_replace(std::string s,
         std::function<bool(unsigned char)> func, std::string const& value)
{
    std::string res;
    for (auto c : s) {
        if (func(static_cast<unsigned char>(c))) {
            res += value;
        } else {
            res += c;
        }
    }
    return res;
}

inline std::string
_to_upper(std::string s)
{
    std::transform(std::begin(s), std::end(s), std::begin(s),
                   [] (unsigned char c)
    { return static_cast<char>(std::toupper(c)); });
    return s;
}

inline int
_run_command(std::string const& command)
{
    return system(command.c_str());
}

inline int
_make_directory(std::string const& path)
{
    return _run_command("mkdir -p " + path);
}

inline bool
_is_directory_exists(std::string const& path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    } else {
        return (info.st_mode & S_IFDIR);
    }
}
}  // namespace detail

int main(int argc, char const* argv[])
{
    auto const default_namespace = "resources";
    auto const default_name = "resources";
    auto const default_output = "resources.hpp";

    auto parser = argparse::ArgumentParser(argc, argv)
            .description("Tool to generate C++ files with binary resources")
            .epilog("by rue-ryuzaki (c) 2022")
            .fromfile_prefix_chars("@")
            .formatter_class(argparse::ArgumentDefaultsHelpFormatter);
    parser.add_argument("resources")
            .action("extend")
            .nargs("*")
            .metavar("file:alias")
            .type<std::pair<std::string, std::string> >()
            .help("list of resources");
    parser.add_argument("--guards")
            .type<std::string>()
            .choices({ "define", "pragma" })
            .default_value("define")
            .help("include guards");
    parser.add_argument("--name")
            .default_value(default_name)
            .help("name for resources");
    parser.add_argument("--namespace")
            .type<std::string>()
            .default_value(default_namespace)
            .help("namespace for resources");
    parser.add_argument("-o", "--output")
            .metavar("file")
            .type<std::string>()
            .default_value(default_output)
            .help("output file name");

    auto const args = parser.parse_args();

    auto guards = args.get<std::string>("guards");
    auto name = args.get<std::string>("name");
    if (name.empty()) {
        name = default_name;
    }
    auto output = args.get<std::string>("output");
    if (output.empty()) {
        output = default_output;
    }
    if (!detail::_ends_with(output, ".h")
            && !detail::_ends_with(output, ".hpp")) {
        output += ".hpp";
    }
    auto space = args.get<std::string>("namespace");
    if (space.empty()) {
        space = default_namespace;
    }
    auto vec = args.get<
          std::vector<std::pair<std::string, std::string> > >("resources", ':');

    auto define = detail::_file_name(output);
    define = detail::_replace(
                define, [] (unsigned char c) { return std::iscntrl(c); }, "");
    define = detail::_replace(
                define, [] (unsigned char c) { return std::ispunct(c); }, "_");
    define = detail::_replace(define, ' ', "_");
    define = "_" + detail::_to_upper(space)
            + "_" + detail::_to_upper(define) + "_";

    auto dir = detail::_directory_name(output);
    if (!detail::_is_directory_exists(dir)) {
        auto res = detail::_make_directory(dir);
        if (res != 0) {
            std::cerr << "[FAIL] Can't create directory '" << dir
                      << "' for output file '" << output << "'" << std::endl;
            return res;
        }
    }
    std::ofstream file(output);

    file << "// this file is auto-generated by the cpp-generes program\n";
    file << "// see https://github.com/rue-ryuzaki/cpp-generes\n";
    file << "\n";
    if (guards == "define") {
        file << "#ifndef " + define + "\n";
        file << "#define " + define + "\n";
    } else {
        file << "#pragma once\n";
    }
    file << "\n";
    file << "#include <cstdint>\n";
    file << "#include <string>\n";
    file << "#include <vector>\n";
    file << "#include <unordered_map>\n";
    file << "\n";
    file << "namespace " << space << " {\n";
    file << "std::unordered_map<std::string, std::vector<uint8_t> > "
            "static const " << name << " =\n";
    file << "{\n";
    for (auto const& pair : vec) {
        std::ifstream in(pair.first, std::ios::binary);
        if (in.is_open()) {
            auto buffer = std::vector<char>(std::istreambuf_iterator<char>(in),
                                            std::istreambuf_iterator<char>());
            file << "    { \"" << pair.second << "\", { ";
            for (std::size_t i = 0; i < buffer.size(); ++i) {
                file << uint32_t(uint8_t(buffer[i])) << ",";
            }
            file << " } },\n";
        }
    }
    file << "};\n";
    file << "}  // namespace " << space << "\n";
    if (guards == "define") {
        file << "\n";
        file << "#endif  // " + define + "\n";
    }
    file.flush();
    file.close();

    std::cout << "[ OK ] File '" << output << "' generated" << std::endl;

    return 0;
}
