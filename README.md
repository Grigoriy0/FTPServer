# FTP server

Multithreaded FTP server for linux implemented in C++



#### compile
```
sudo apt install -y\
        libmysql++-dev\
        libmysqlclient-dev

        
git clone https://github.com/Grigriy0/FTPServer
make all
```
#### prepare mysql

the table should look like this:

![](https://s8.hostingkartinok.com/uploads/images/2020/06/7a65207f37e2a38cf1fd05d513b172f1.png)


#### run

```
export FTP_DB_NAME=<database name>
export FTP_DB_USER=<mysql user>
export FTP_DB_PASS=<user password>
export FTP_DB_HOST=<host address>

./Server <port> [ip_address] [max_connections]

```
