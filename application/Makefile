BASE		= ..

TOOLS		= ./tools
SRC			= ./squeeze2cast
SQUEEZETINY	= ./squeezetiny
JANSSON		= $(BASE)/jansson
NANOPB		= $(BASE)/nanopb
MDNSSD		= $(BASE)/mdnssd
VALGRIND	= $(BASE)/valgrind
UPNP 		= $(BASE)/libupnp/1.6.19

DEFINES 	= -DNO_CODEC -D_FILE_OFFSET_BITS=64 -DPB_FIELD_16BIT -DHAVE_STDINT_H

vpath %.c $(TOOLS):$(SRC):$(SQUEEZETINY):$(JANSSON)/src:$(NANOPB):$(MDNSSD)

INCLUDE = -I. \
		  -I$(VALGRIND)/memcheck -I$(VALGRIND)/include \
		  -I$(SQUEEZETINY) \
		  -I$(TOOLS) \
		  -I$(SRC) -I$(SRC)/inc \
		  -I$(UPNP)/ixml/inc -I$(UPNP)/upnp/inc -I$(UPNP)/build/inc -I$(UPNP)/threadutil/inc \
		  -I$(JANSSON)/src \
		  -I$(NANOPB) \
		  -I$(MDNSSD)
		  		  
DEPS	= $(SQUEEZETINY)/squeezedefs.h
				  
SOURCES = slimproto.c buffer.c stream.c utils.c output_mr.c decode.c main.c \
		  util_common.c cast_util.c util.c webserver.c log_util.c \
		  castcore.c castmessage.pb.c squeeze2cast.c \
		  pb_common.c pb_decode.c pb_encode.c dump.c error.c hashtable.c strconv.c \
		  hashtable_seed.c load.c memory.c pack_unpack.c strbuffer.c utf.c value.c \
	  	  mdnssd-min.c
		
SOURCES_LIBS = 
		
OBJECTS 		= $(patsubst %.c,$(OBJ)/%.o,$(SOURCES)) $(patsubst %.c,$(OBJ)/%.o,$(SOURCES_LIBS)) 
OBJECTS_STATIC 	= $(patsubst %.c,$(OBJ)/%.o,$(SOURCES)) $(patsubst %.c,$(OBJ)/%-static.o,$(SOURCES_LIBS)) 

LIBRARY 		= $(OBJ)/libupnp.a $(OBJ)/libixml.a $(OBJ)/libthreadutil.a
LIBRARY_STATIC 	= $(LIBRARY) 

all: $(EXECUTABLE) $(EXECUTABLE_STATIC)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBRARY) $(LDFLAGS) -o $@

$(EXECUTABLE_STATIC): $(OBJECTS_STATIC)
	$(CC) $(OBJECTS_STATIC) $(LIBRARY_STATIC) $(LDFLAGS) -static -o $@	

$(OBJECTS): $(DEPS) | bin $(OBJ) 

$(OBJ):
	@mkdir -p $@
	
bin:	
	@mkdir -p bin

$(OBJ)/%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDE) $< -c -o $@

$(OBJ)/%-static.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DLINKALL $(INCLUDE) $< -c -o $(OBJ)/$*-static.o	
	
clean:
	rm -f $(OBJECTS) $(EXECUTABLE) $(OBJECTS_STATIC) $(EXECUTABLE_STATIC) 

