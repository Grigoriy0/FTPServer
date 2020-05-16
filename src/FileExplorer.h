//
// Created by grigoriy on 24.02.20.
//

#ifndef FILECOMMANDER_H
#define FILECOMMANDER_H


#include <string>
#include <vector>
#include <utility>
#define download_n_upload_files public

class FileExplorer {
public:

    explicit FileExplorer(std::string root_dir): dir("/"), root(std::move(root_dir)){}

    std::vector<std::string> ls(const std::string &dir) ;
    /*LIST
    * */

    bool rm(const std::string &filename);
    /* DELE
        * */

    uint64_t size(const std::string &filename);
    /* SIZE
    * */

    bool mkdir(const std::string &path_namedir);
    /* MKD
    * */

    bool rmdir(const std::string &path_namedir);
    /* RMD
    * */

    bool move(const std::string &what_path, std::string &path_to);
    /* RNFR RNTO
    * */

    std::string pwd();

download_n_upload_files:

    void write(const std::string &file, char *data, size_t size);

    char *read(const std::string &file, size_t size, int offset);

private:
    std::string root;
    std::string dir;
};


#endif // FILECOMMANDER_H
