IF (CMAKE_COMPILER_IS_GNUCC)
set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall  -g -gdwarf-2 -O2 -DPIC -fPIC -I./")
ENDIF (CMAKE_COMPILER_IS_GNUCC)

#SET(BUILD_SHARED_LIBS ON)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS} ${DEBUG_CFLAGS}")
SET ( X_CORE_LIBS xw xofactory )

INCLUDE(${PROJECT_SOURCE_DIR}/INCLUDES.cmk)

LINK_DIRECTORIES( ${lib_contrib_dir}  )

LINK_DIRECTORIES( ${CMAKE_BINARY_DIR}/output/lib )
LINK_DIRECTORIES( ${include_top_dir}/3dparty/openssl/${X_ARCH}/ )
INCLUDE_DIRECTORIES( ${include_top_dir}/3dparty/openssl/${X_ARCH}/include )
INCLUDE_DIRECTORIES( ${include_contrib_dir} )
INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR}/output/include )

if(BUILD_SHARED_LIBS)

add_library(features features.c ${PROJECT_SOURCE_DIR}/x_dll.c) 
add_library(procsys procsys.c ${PROJECT_SOURCE_DIR}/x_dll.c) 
add_library(stream stream.c ${PROJECT_SOURCE_DIR}/x_dll.c) 
add_library(sasl sasl.c ${PROJECT_SOURCE_DIR}/x_dll.c) 
add_library(tls tls.c ${PROJECT_SOURCE_DIR}/x_dll.c) 
add_library(iq iq.c ${PROJECT_SOURCE_DIR}/x_dll.c) 
add_library(presence presence.c ${PROJECT_SOURCE_DIR}/x_dll.c) 
add_library(message message.c ${PROJECT_SOURCE_DIR}/x_dll.c) 

target_link_libraries(features ${X_CORE_LIBS} tls xbus ) 
target_link_libraries(stream ${X_CORE_LIBS} xbus )  
target_link_libraries(sasl ${X_CORE_LIBS} xbus ) 
target_link_libraries(tls ${X_CORE_LIBS}) 
target_link_libraries(iq ${X_CORE_LIBS}) 
target_link_libraries(procsys ${X_CORE_LIBS} xbus ) 
target_link_libraries(presence ${X_CORE_LIBS} xbus ) 
target_link_libraries(message ${X_CORE_LIBS} sessions ) 

IF(UNIX)
target_link_libraries(sasl ssl crypto)
target_link_libraries(tls ssl crypto)
ENDIF(UNIX)

IF(APPLE)
target_link_libraries(sasl ssl crypto)
target_link_libraries(tls ssl crypto)
ENDIF(APPLE)

IF(WIN32)
target_link_libraries(sasl ssleay32 libeay32)
target_link_libraries(tls ssleay32 libeay32)
ENDIF(WIN32)

if(ANDROID)
target_link_libraries(features log)
target_link_libraries(iq log)
target_link_libraries(stream log)
target_link_libraries(sasl ssl crypto log)
target_link_libraries(tls ssl crypto log)
target_link_libraries(presence log)
target_link_libraries(message log)
endif(ANDROID)

#INSTALL(TARGETS features stream sasl tls iq 
#	presence message DESTINATION ${PLUGINS_DIR})

else(BUILD_SHARED_LIBS)

add_library(core STATIC procsys.c features.c stream.c sasl.c tls.c iq.c presence.c message.c) 
IF(UNIX)
target_link_libraries(core ssl crypto xbus sessions)
ENDIF(UNIX)

IF(WIN32)
target_link_libraries(core ssleay32 libeay32 xbus sessions)
ENDIF(WIN32)

if(ANDROID)
target_link_libraries(core ssl crypto log xbus sessions)
endif(ANDROID)

IF(APPLE)
target_link_libraries(core ssl crypto xbus sessions)
ENDIF(APPLE)

endif(BUILD_SHARED_LIBS)

#add_dependencies(tls xw openssl)

INCLUDE(CPack)

ENABLE_TESTING()

