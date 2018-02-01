AUTOMAKE_OPTIONS=foreign 

BOOSTINCLUDEDIR ?= /usr/local/include

BOOSTLIBDIR ?= /usr/local/lib

BMCOINCLUDEDIR ?= /usr/local/include

BMCOLIBDIR ?= /usr/local/lib


PUBINCLUDE_DIR ?= ../protocol/

../protocol/BolService.pb.cc:
	protoc --cpp_out=../protocol/ --proto_path=../protocol/ ../protocol/BolService.proto 
	
bin_PROGRAMS=BolService

THECPPS =   ../protocol/BolService.pb.cc  MemTaskLoop.cpp KernelGuard.cpp  KernelTaskLoop.cpp \
					  RegularProcessTaskLoop.cpp   BolServiceRequestHandler.cpp \
					  BolServiceServer.cpp ResponseNotify.cpp ResponseSend.cpp TaskNotification.cpp \
					  WorkTask.cpp DataAPI.cpp DB.cpp ReDoLog.cpp sqlite3.c  MemTableOp.cpp BackupTask.cpp \
					  OSInfoInterface.cpp ShmSegment.cpp ShmMappedFile.cpp 

BolService_SOURCES = $(THECPPS)

THEHEADERS =   MemTaskLoop.h   KernelTaskLoop.h \
						  RegularProcessTaskLoop.h   BolServiceAcceptor.h \
						  BolServiceRequestHandler.h BolServiceServer.h ResponseNotify.h \
						  ResponseSend.h TaskNotification.h WorkTask.h DataAPI.h DB.h ReDoLog.h \
						  sqlite3.h  MemTableOp.h BackupTask.h MetaBpcbInfo.h MetaProgramDef.h \
						  MetaRegularProcessTask.h OSInfoInterface.h MemUtil.h ShmMappedFile.h \
						  ShmSegment.h  VirtualShm.h 
include_HEADERS = $(THEHEADERS)

BolService_LDADD =

BolService_CFLAGS = -DSQLITE_ENABLE_COLUMN_METADATA -DBOL_OMC_WRITER

BolService_CPPFLAGS = -DBOL_OMC_WRITER ${DEV_INCLUDES} -I../BolPublic -I$(BOOSTINCLUDEDIR) -I$(BMCOINCLUDEDIR)  -I$(PUBINCLUDE_DIR)

BolService_LDFLAGS =  ${DEV_LDADDS} -L../BolPublic/.libs -lBolPublic -L$(BOOSTLIBDIR) -lboost_system -lboost_date_time \
										-lboost_locale -L$(BMCOLIBDIR) -lBmcoUtil -lBmcoData -lBmcoFoundation -lBmcoXML -lBmcoNet -lprotobuf