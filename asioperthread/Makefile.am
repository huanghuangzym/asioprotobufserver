AUTOMAKE_OPTIONS=foreign

bin_PROGRAMS=AtServer

BOOSTINCLUDEDIR ?= /usr/local/include

BOOSTLIBDIR ?= /usr/local/lib

BMCOINCLUDEDIR ?= /usr/local/include

BMCOLIBDIR ?= /usr/local/lib

PUBINCLUDE_DIR ?= ../protocol/

../protocol/BolService.pb.cc:
	protoc --cpp_out=../protocol/ --proto_path=../protocol/ ../protocol/BolService.proto 


AtServer_THECPPS = ../protocol/BolService.pb.cc connection.cpp io_service_pool.cpp main.cpp mime_types.cpp reply.cpp \
										request_handler.cpp \
										request_parser.cpp server.cpp logger.cpp

AtServer_SOURCES = $(AtServer_THECPPS)

THEHEADERS =

include_HEADERS = $(THEHEADERS)

AtServer_LDADD =

AtServer_CPPFLAGS = -I../client ${DEV_INCLUDES}  -I$(PUBINCLUDE_DIR) \
								 -I ${BOOSTINCLUDEDIR} -I ${BMCOINCLUDEDIR} -DBOOST_LOG_DYN_LINK

AtServer_LDFLAGS = ${DEV_LDADDS} -L../client/.libs  -L/usr/local/lib -DBOOST_LOG_DYN_LINK \
    -lboost_date_time  -lboost_regex -lboost_system -lboost_thread -lboost_log -lboost_filesystem \
    -lboost_chrono  -lprotobuf
