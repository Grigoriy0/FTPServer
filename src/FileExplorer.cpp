//
// Created by grigoriy on 24.02.20.
//

#include "FileExplorer.h"
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <fstream>

std::vector<std::string> FileExplorer::ls(const std::string &dir) {
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

void FileExplorer::rm(const std::string &filename) {
    if (::remove((root + filename).c_str()) == -1)
    {
        printf("Error removing file %s\n", filename.c_str());
    }
}


void FileExplorer::rmdir(const std::string &path_namedir) { // with all nested files
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


uint64_t FileExplorer::size(const std::string &filename) {
    struct stat file_stat{};
    stat((root + filename).c_str(), &file_stat);
    return file_stat.st_size;
}

void FileExplorer::mkdir(const std::string &path_namedir)
{
    if (::mkdir((root + path_namedir).c_str(), S_IRWXU | S_IRWXG | S_IRGRP))
    {
        printf("Error mkdir %s\n", path_namedir.c_str());
    }
}

void FileExplorer::move(const std::string &what_path, std::string &path_to) {
    if(::rename((root + what_path).c_str(), (root + path_to).c_str()))
    {
        printf("Error renaming file %s to %s\n", what_path.c_str(), path_to.c_str());
    }
}

void FileExplorer::write(const std::string &file, char *data, size_t size) {
    std::ofstream out;
    out.open(file, std::ios::binary | std::ios::app);
    if (!out.good())
        out.open(file);
    out.write(data, size);
    out.close();
}


char *FileExplorer::read(const std::string &file, size_t size, int offset) {
    std::ifstream in;
    in.open(file, std::ios_base::binary);
    if(!in.good())
    {
        std::ofstream of(file); // create new
        of.close();
        printf("File %s doesn't exist!\n", file.c_str());
        return nullptr;
    }
    in.seekg(offset);
    char *result = (char *)malloc(size);
    in.read(result, size);
    in.close();
    return result;
}
