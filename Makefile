ver = debug


ALLBIN = chat_release chat_debug
OBJDIR = ./OBJS
VPATH = $(OBJDIR)
#vpath %.o $(OBJDIR)


ifeq ($(ver), debug)
ALL: chat_debug
CXXFLAGS = -c -g -Ddebug
OBJ = demo.o websocket.o base64.o connection.o user.o frame.o
BIN = chat_debug
else
ALL: chat_release
CXXFLAGS = -c -O3
OBJ = demo.ro websocket.ro base64.ro connection.ro user.ro frame.ro
BIN = chat_release
endif


INCLUDE = -I./libevent/include \
		  -I./libopenssl/include
LIBRARY = -L./libevent/lib -levent \
		  -L./libopenssl/lib -lcrypto -lssl \
		  -lrt


chat_debug: $(OBJ)
	make tmp ver=$(ver)

chat_release: $(OBJ)
	make tmp ver=$(ver)

%.o: %.cpp
	g++ $(CXXFLAGS) $< -o $(OBJDIR)/$@ $(INCLUDE)

%.ro: %.cpp
	g++ $(CXXFLAGS) $< -o $(OBJDIR)/$@ $(INCLUDE)


.PHONY: clean
clean:
	rm -f $(ALLBIN) $(OBJDIR)/*

.PHONY: tmp
tmp: $(OBJ)
	g++ -o $(BIN) $^ $(LIBRARY)
