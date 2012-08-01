#CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
#PROJECT(theora)
INCLUDE(CheckIncludeFile)

IF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
ENDIF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")

CHECK_INCLUDE_FILE("dlfcn.h" HAVE_DLFCN_H)
CHECK_INCLUDE_FILE("inttypes.h" HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE("memory.h" HAVE_MEMORY_H)
CHECK_INCLUDE_FILE("poll.h" HAVE_POLL_H)
CHECK_INCLUDE_FILE("sys/epoll.h" HAVE_SYS_EPOLL_H)
CHECK_INCLUDE_FILE("sys/eventfd.h" HAVE_SYS_EVENTFD_H)
CHECK_INCLUDE_FILE("sys/event.h" HAVE_SYS_EVENT_H)
CHECK_INCLUDE_FILE("sys/inotify.h" HAVE_SYS_INOTIFY_H)
CHECK_INCLUDE_FILE("sys/select.h" HAVE_SYS_SELECT_H)
CHECK_INCLUDE_FILE("sys/signalfd.h" HAVE_SYS_SIGNALFD_H)
CHECK_INCLUDE_FILE("sys/stat.h" HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE("sys/types.h" HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE("unistd.h" HAVE_UNISTD_H)

INCLUDE(CheckFunctionExists)

CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)
CHECK_FUNCTION_EXISTS(epoll_ctl HAVE_EPOLL_CTL)
CHECK_FUNCTION_EXISTS(eventfd HAVE_EVENTFD)
CHECK_FUNCTION_EXISTS(floor HAVE_FLOOR)
CHECK_FUNCTION_EXISTS(inotify_init HAVE_INOTIFY_INIT)
CHECK_FUNCTION_EXISTS(kqueue HAVE_KQUEUE)
CHECK_FUNCTION_EXISTS(nanosleep HAVE_NANOSLEEP)
CHECK_FUNCTION_EXISTS(poll HAVE_POLL)
CHECK_FUNCTION_EXISTS(port_create HAVE_PORT_CREATE)

SET(STDC_HEADERS 1)

ADD_DEFINITIONS(-DHAVE_CONFIG_H)

CONFIGURE_FILE(config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

IF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86")

SET( THEORADEC_OPT_SRC lib/x86/mmxidct.c
	lib/x86/mmxfrag.c
	lib/x86/mmxstate.c
	lib/x86/x86state.c
	)

SET(THEORAENC_OPT_SRC 	
	lib/x86/mmxencfrag.c
	lib/x86/mmxfdct.c
	lib/x86/x86enc.c
	lib/x86/mmxfrag.c
	lib/x86/mmxidct.c
	lib/x86/mmxstate.c
	lib/x86/x86state.c
	)
	
ENDIF()

ADD_LIBRARY(theoradec 
	lib/apiwrapper.c
	lib/bitpack.c
	lib/decapiwrapper.c
	lib/decinfo.c
	lib/decode.c
	lib/dequant.c
	lib/fragment.c
	lib/huffdec.c
	lib/idct.c
	lib/info.c
	lib/internal.c
	lib/quant.c
	lib/state.c
	${THEORADEC_OPT_SRC}
)

ADD_LIBRARY(theoraenc 
	lib/apiwrapper.c
	lib/fragment.c
	lib/idct.c
	lib/internal.c
	lib/state.c
	lib/quant.c
	lib/analyze.c
	lib/fdct.c
	lib/encfrag.c
	lib/encapiwrapper.c
	lib/encinfo.c
	lib/info.c
	lib/encode.c
	lib/enquant.c
	lib/huffenc.c
	lib/mathops.c
	lib/mcenc.c
	lib/rate.c
	lib/tokenize.c 
	${THEORAENC_OPT_SRC}
)

INCLUDE_DIRECTORIES ( ${CMAKE_CURRENT_SOURCE_DIR} )
INCLUDE_DIRECTORIES ( ${CMAKE_CURRENT_SOURCE_DIR}/include )
INCLUDE_DIRECTORIES ( ${OGG_INCLUDES} )

ADD_DEPENDENCIES(theoraenc ogg)
ADD_DEPENDENCIES(theoradec ogg)

TARGET_LINK_LIBRARIES(theoraenc ogg)
TARGET_LINK_LIBRARIES(theoradec ogg)
