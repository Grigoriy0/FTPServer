#include "FileExplorer.h"
#include "defines.h"
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <libgen.h>

std::vector<std::string> FileExplorer::ls(const std::string &selected_dir) {
    std::vector<std::string> result{};
    DIR* dirStream;
    struct dirent *dp;

    dirStream = opendir((root + dir + selected_dir).c_str());
    if (dirStream == nullptr) {
        return {"~~~"}; // error signal - unknown directory
    }
    while ((dp = readdir(dirStream)) != nullptr) {
        result.emplace_back(std::string(dp->d_name) + (dp->d_type == 4 ? "/" : ""));
    }
    closedir(dirStream);
    for (unsigned i = 0; i < result.size(); ++i){
        if (result[i] == "../" || result[i] == "./"){
            result.erase(result.begin() + i);
            --i;
        }
    }
    return result;
}

bool FileExplorer::rm(const std::string &filename) {
    if (::remove((root + dir + filename).c_str()) == -1) {
        print_error("E: removing file %s" + filename);
        return false;
    }
    return true;
}


bool FileExplorer::rmdir(std::string path_name) { // with all nested files
    auto nested_files = ls(path_name);
    if (!nested_files.empty() && nested_files[0] != "~~~") // has nested files or directories
    {
        for (auto & nested_file : nested_files) {
            if (nested_file[nested_file.size() - 1] == '/') { // directory

                cd(path_name);

                bool res = rmdir(nested_file);

                cd_up();
                if (!res)
                    return false;
            }
            else {
                // it's a simple file
                if (path_name[path_name.size() - 1] != '/')
                    nested_file = "/" + nested_file;                
                nested_file = path_name + nested_file;
                printf("remove file %s\n", (root + dir + nested_file).c_str());
                if (::remove((root + dir + nested_file).c_str())) {
                    print_error("E: removing filepath " + nested_file);
                    return false;
                }
            }
        }
    } // empty directory
    printf("remove dir %s\n", (root + dir + path_name).c_str());
    if (::remove((root + dir + path_name).c_str())) {
        print_error("E: removing directory " + dir + path_name);
        return false;
    }
    return true;
}


uint64_t FileExplorer::size(const std::string &filename) {
    struct stat file_stat{};
    if (stat((root + dir + filename).c_str(), &file_stat) == -1) {
        print_error("stat failed")
    }
    return file_stat.st_size;
}

bool FileExplorer::mkdir(std::string path_namedir) {
    if (path_namedir.empty())
        return false;
    while (path_namedir[0] == '/' || path_namedir[0] == '.') {
        path_namedir = path_namedir.substr(1);
        if (path_namedir.empty())
            return false;
    }
    while (path_namedir[path_namedir.size() - 1] == '/') {
        path_namedir.pop_back();
        if (path_namedir.empty())
            return false;
    }
    if (path_namedir.empty())
        return false;

    std::string folder = root;
    folder += dir + path_namedir;
    printf("creating %s\n", folder.c_str());
    if (::mkdir(folder.c_str(), S_IRWXU | S_IRWXG | S_IRGRP))
    {
        print_error("E: Error mkdir" + folder);
        return false;
    }
    return true;
}

bool FileExplorer::move(const std::string &what_path, std::string &path_to) {
    if(::rename((root + what_path).c_str(), (root + path_to).c_str()))
    {
        print_error("Error renaming file %s to " + what_path + path_to);
        return false;
    }
    return true;
}


std::string FileExplorer::pwd() {
    return dir;
}


std::string FileExplorer::root_dir() {
    return root;
}


std::string FileExplorer::cd_up() {
    if (dir == "/")
        return root;
    char buffer[100];
    strcpy(buffer, root.c_str());
    strcpy(buffer, dir.c_str());
    dir = dirname(buffer);
    if (dir[dir.size() - 1] != '/')
        dir += '/';
    return dir;
}


bool FileExplorer::cd(std::string next_dir) {
    if (next_dir.empty())
        return false;
    while (next_dir[0] == '/' || next_dir[0] == '.') {
        next_dir = next_dir.substr(1);
        if (next_dir.empty())
            return false;
    }
    while (next_dir[next_dir.size() - 1] == '/') {
        next_dir.pop_back();
        if (next_dir.empty())
            return false;
    }
    if (next_dir.empty())
        return false;
    std::string result_dir = root + dir;
    result_dir += next_dir;

    DIR *d = opendir(result_dir.c_str());
    if (d) {
        dir += next_dir + "/";
        closedir(d);
        return true;
    } else return false;
}
