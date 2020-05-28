# compile

git clone https://github.com/Grigoriy0/FTPServer
cd FTPServer
make

# run
cp Server ../Server
cd ..
mkdir -p /ftpd/Vlad
mkdir -p /ftpd/Lexys
./Server $(cat port.txt) $(cat my_ftp_pub_ip)

# good idea - run it in a docker container, but later...