ALL : server.exe
myArgs = -lpthread -lmysqlclient -g
server.exe : ./pool/threadpool.h ./pool/sqlQueryQueue.h ./someipParse/someipParse.h ./buffer/buffer.h ./buffer/buffer.cpp ./epoller/epoller.h ./epoller/epoller.cpp ./epoller/server.h ./epoller/server.cpp  ./conn/conn.h ./conn/conn.cpp ./pool/sqlconnpool.h ./pool/sqlconnpool.cpp ./epoller/main.c ./timer/heaptimer.h ./timer/heaptimer.cpp ./cereal/cereal.hpp ./cereal/macros.hpp ./cereal/details/traits.hpp ./cereal/details/helpers.hpp ./cereal/types/base_class.hpp ./cereal/access.hpp ./cereal/specialize.hpp ./cereal/archives/binary.hpp ./cereal/types/string.hpp ./cereal/types/memory.hpp ./cereal/details/static_object.hpp ./cereal/details/polymorphic_impl_fwd.hpp ./cereal/types/common.hpp ./cereal/types/polymorphic.hpp ./cereal/details/util.hpp
	g++ $^ -o $@ $(myArgs)

clean:
	-rm -rf server.exe 
.PHONY: clean ALL