CXXFLAGS 	:= -I/usr/include/mysql -I/usr/local/include/mysql++ -I/usr/include/mysql++
LDFLAGS 	:= -L/usr/local/lib
LDLIBS 		:= -lmysqlpp -lmysqlclient -lpthread

#If you’re building a threaded program, use -lmysqlclient_r instead of -lmysqlclient here.
EXECUTABLE := src/test.server.cpp src/MySqlClient.cpp src/tcpsocket.cpp
all: $(EXECUTABLE)
	g++ -o Server $(EXECUTABLE) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f Server
