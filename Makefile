CXXFLAGS 	:= -I/usr/include/mysql -I/usr/local/include/mysql++ -I/usr/include/mysql++
LDFLAGS 	:= -L/usr/local/lib
LDLIBS 		:= -lmysqlpp -lmysqlclient -pthread -lrt

#If you’re building a threaded program, use -lmysqlclient_r instead of -lmysqlclient here.
EXECUTABLE := src/main.cpp src/MySqlClient.cpp src/TcpSocket.cpp src/DataThread.cpp \
src/FileExplorer.cpp src/FTPServer.cpp src/Client.cpp
all: $(EXECUTABLE)
	g++ -o Server $(EXECUTABLE) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)
	echo "\n\n"
clean:
	rm -f Server
