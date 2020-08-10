// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

/// \file
/// This is a very primitive version of the cppast tool.
///
/// Given an input file it will print the AST.

#include <cppast/visitor.hpp> // visit()
#include <cppast/cpp_entity_kind.hpp>

#include "example_parser.hpp"

#include <map>
#include <fstream>
#include <filesystem>
#include <sstream>


std::string prettify(std::string name)
{
    std::stringstream buf;
    bool last_char_was_uppercase = false;
    char last_char = 0;
    for(int i = 0;i != name.length();i++)
    {
        char c = name[i];
        bool is_up = isupper(c);
        if((!i || last_char == '_') && c == '_')
        {
            continue;
        }
        if(is_up && !last_char_was_uppercase && i && last_char != '_')
        {
            buf << "_";
            last_char = '_';
        }
        if(is_up)
        {
            buf << (char)tolower(c);
            last_char = (char)tolower(c);

            if(i == 0 && c == 'I')
            {
                buf << "_";
                last_char = '_';
            }
            last_char_was_uppercase = true;
        }
        /*else if(!is_up && last_char_was_uppercase)
        {
            if(i)
                buf << "_";
            buf << c;
            last_char = c;
            last_char_was_uppercase = false;
        }*/
        else
        {
            buf << c;
            last_char = c;
            last_char_was_uppercase = false;
        }
    }

    auto tmp = buf.str();
    return tmp;
}

std::filesystem::path prettify_file(std::filesystem::path path)
{
    path.replace_filename(prettify(path.filename()));
    return path;
}

std::ofstream file_rename_list;
std::ofstream symbol_rename_list;
std::map<std::string, std::filesystem::path> file_names;
std::map<std::string, std::string> type_names;

void print_ast(const cppast::cpp_file& file)
{
    // visit each entity in the file
    cppast::visit(file, [&](const cppast::cpp_entity& e, cppast::visitor_info info) {
        if (e.kind() == cppast::cpp_entity_kind::file_t)
        {
        }
        else if (e.kind() == cppast::cpp_entity_kind::include_directive_t)
        {
            auto& directive = static_cast<const cppast::cpp_include_directive&>(e);

            if(directive.full_path().find("secretarium") != std::string::npos)
            {
                auto source_path = std::filesystem::path(directive.full_path());
                auto pretty_file = prettify_file(source_path);
                if(pretty_file != directive.full_path())
                {
                    auto x = source_path.c_str();
                    file_names[source_path] = pretty_file;
                    type_names[source_path.filename()] = pretty_file.filename();
                }
            }
        }

        else if (info.event == cppast::visitor_info::container_entity_exit) // exiting an old container
        {
        }
        else if (
            e.kind() == cppast::cpp_entity_kind::class_t ||
            e.kind() == cppast::cpp_entity_kind::function_t
        )
        {
            auto name = prettify(e.name());
            if(name != e.name())
            {
                type_names[e.name()] = name;
            }
        }
    });
}

int main(int argc, char* argv[])
{
    auto ret = example_main(argc, argv, {}, &print_ast);

    if(ret)
        return ret;
    
    auto path = std::filesystem::path("/home/edward/Projects/core/secretarium");

    for(auto& p: std::filesystem::recursive_directory_iterator(path))
    {
        if(p.path().filename() == "CMakeLists.txt")
            continue;

        std::string source_path = p.path().c_str();
        auto pretty_file = prettify_file(p.path());
        std::string dest_file = pretty_file.c_str();
        if(dest_file != source_path)
        {
            file_names[source_path] = pretty_file;
        }
    }

    file_rename_list.open("filenames.txt", std::ofstream::out);
    symbol_rename_list.open("symbolnames.txt", std::ofstream::out);

    std::for_each(
        file_names.begin(), 
        file_names.end(), 
        [&](std::pair<std::string, std::filesystem::path> it)
    {
        auto x = it.second.c_str();
        file_rename_list << it.first << " = " << x << "\n";
    });

    std::for_each(
        type_names.begin(), 
        type_names.end(), 
        [&](std::pair<std::string, std::string> it)
    {
        symbol_rename_list << it.first << " = " << it.second << "\n";
    });

    return ret;
}
