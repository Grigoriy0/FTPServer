//
// Created by grigoriy on 24.02.20.
//

#ifndef FILECOMMANDER_H
#define FILECOMMANDER_H


#include <string>
#include <vector>
#include <utility>
#define download_n_upload_files public

class FileCommander {
public:

    explicit FileCommander(std::string root_dir): root(std::move(root_dir)){}

    std::vector<std::string> ls(const std::string &dir) ;
    /*LIST
    * */

    void rm(const std::string &filename);
    /* DELE
        * */

    uint64_t size(const std::string &filename);
    /* SIZE
    * */

    void mkdir(const std::string &path_namedir);
    /* MKD
    * */

    void rmdir(const std::string &path_namedir);
    /* RMD
    * */

    void move(const std::string &what_path, std::string &path_to);
    /* RNFR RNTO
    * */

download_n_upload_files:

    void write(const std::string &file, char *data, size_t size);

    char *read(const std::string &file, size_t size, int offset);

private:
    std::string root;


};


#endif // FILECOMMANDER_H
