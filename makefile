ALL : server.exe
myArgs = -lpthread -lmysqlclient -g
server.exe : ./pool/threadpool.h ./pool/sqlQueryQueue.h ./someipParse/someipParse.h ./buffer/buffer.h ./buffer/buffer.cpp ./epoller/epoller.h ./epoller/epoller.cpp ./epoller/server.h ./epoller/server.cpp  ./conn/conn.h ./conn/conn.cpp ./pool/sqlconnpool.h ./pool/sqlconnpool.cpp ./epoller/main.c  
	g++ $^ -o $@ $(myArgs)

clean:
	-rm -rf server.exe 
.PHONY: clean ALL