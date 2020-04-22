//
// Created by grigoriy on 24.02.20.
//

#include "FileCommander.h"
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <fstream>

std::vector<std::string> FileCommander::ls(const std::string &dir) {
    std::vector<std::string> result = {};
    DIR* dirStream;
    struct dirent *dp;

    dirStream = opendir((root + dir).c_str());
    while ((dp = readdir(dirStream)) != nullptr) {
        result.emplace_back(std::string(dp->d_name) + (dp->d_type == 4 ? "/" : ""));
    }
    closedir(dirStream);
    return result;
}

void FileCommander::rm(const std::string &filename) {
    if (::remove((root + filename).c_str()) == -1)
    {
        printf("Error removing file %s\n", filename.c_str());
    }
}


void FileCommander::rmdir(const std::string &path_namedir) { // with all nested files
    auto nested_files = ls(path_namedir);
    if (nested_files.size() != 2) // "." and ".."
    {
        for (const auto & nested_file : nested_files) {
            if (nested_file == "./" || nested_file == "../")
                continue;
            if (nested_file[nested_file.size() - 1] == '/') {
                // it's a directory
                std::string dir = path_namedir;
                if (path_namedir[path_namedir.size() - 1] != '/')
                    dir += '/';
                rmdir(dir + nested_file);
            }
            else {
                // it's a simple file

                std::string filepath = path_namedir;
                if (path_namedir[path_namedir.size() - 1] != '/')
                    filepath += '/';
                if (::remove((root + filepath + nested_file).c_str())) {
                    printf("Error removing filepath %s\n", (filepath + nested_file).c_str());
                }
            }
        }
    }
    if (::remove((root + path_namedir).c_str())) { // empty directory
        printf("Error removing directory %s\n", path_namedir.c_str());
    }
}


uint64_t FileCommander::size(const std::string &filename) {
    struct stat file_stat{};
    stat((root + filename).c_str(), &file_stat);
    return file_stat.st_size;
}

void FileCommander::mkdir(const std::string &path_namedir)
{
    if (::mkdir((root + path_namedir).c_str(), S_IRWXU | S_IRWXG | S_IRGRP))
    {
        printf("Error mkdir %s\n", path_namedir.c_str());
    }
}

void FileCommander::move(const std::string &what_path, std::string &path_to) {
    if(::rename((root + what_path).c_str(), (root + path_to).c_str()))
    {
        printf("Error renaming file %s to %s\n", what_path.c_str(), path_to.c_str());
    }
}

void FileCommander::write(const std::string &file, char *data, size_t size) {
    std::ofstream out;
    if(!out.open(file, std::ios::binary | std::ios::app))
    {
        //file doesn't exist
        out.open(file, std::ios::binary | std::ios::trunc);
    }
    out.write(data, size);
    out.close();
}


char *FileCommander::read(const std::string &file, size_t size, int offset) {
    std::ifstream in;
    if(!in.open(file, std::ios_base::binary))
    {
        printf("File %s doesn't exist!\n", file.c_str());
        return nullptr;
    }
    in.seekg(offset);
    char *result = (char *)malloc(size);
    in.read(result, size);
    in.close();
    return result;
}