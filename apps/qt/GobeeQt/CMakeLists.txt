cmake_minimum_required(VERSION 2.6)

set(PROJECT GobeeQt)


SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/output CACHE PATH "Path for libraries")
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/output CACHE PATH "Path for libraries")

FIND_PACKAGE(Threads)

SET(DEBUG ON)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/xmppbot)

#SET( libev_include_dir ${PROJECT_SOURCE_DIR}/xmppbot/3dparty/libev )
#include_directories( ${libev_include_dir} )

include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_contrib_dir} )
 
include_directories(${CMAKE_CURRENT_SOURCE_DIR} )
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../xmppbot )
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../xmppbot/3dparty/libev )
link_directories( ${LIBRARY_OUTPUT_PATH}  )
link_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../../../qt/output  )

set(CMAKE_C_FLAGS "-Wall -pthread -g -gdwarf-2")
set(SHARE_DIR "${CMAKE_INSTALL_PREFIX}/share/xwclient")

set(HEADERS gobeeapplication.h mainwindow.h gobeerostermodel.h displayframe.h
         gobeewrap.h gobee/gobeedisplay.h gobee/gobeehid.h
            gobeegraphicsscene.h gobeewrap.h ui/optionsframe.h
        ui/calldialog.h ui/messageform.h
        gobee/gobeemsg.h     )

set(SOURCES gobeeapplication.cpp mainwindow.cpp main.cpp gobeerostermodel.cpp
            displayframe.cpp gobee/gobeedisplay.cpp gobee/gobeehid.cpp
            gobeegraphicsscene.cpp gobeewrap.cpp ui/optionsframe.cpp
            ui/calldialog.cpp ui/messageform.cpp
            gobee/gobeemsg.cpp )
            
IF(APPLE AND NOT IOS)
SET(SOURCES ${SOURCES} gobee/osx/audio/gobee_mac_dev.c 
    gobee/osx/audio/gobee_mac_vmic.c gobee/osx/audio/gobee_mac_vspk.c
    gobee/osx/video/gobee_mac_vcam.m gobee/osx/video/gobee_mac_vcam_plugin.c)

   #SET(GUI_TYPE MACOSX_BUNDLE)
   #INCLUDE_DIRECTORIES ( /Developer/Headers/FlatCarbon )
   FIND_LIBRARY(CORE_VIDEO_LIBRARY CoreVideo)
   FIND_LIBRARY(CORE_AUDIO_LIBRARY CoreAudio)
   FIND_LIBRARY(CORE_MEDIA_LIBRARY CoreMedia)
   FIND_LIBRARY(CORE_GRAPHICS_LIBRARY CoreGraphics)
   FIND_LIBRARY(AV_FOUNDATION_LIBRARY AVFoundation)
   FIND_LIBRARY(AUDIO_TOOLBOX_LIBRARY AudioToolbox)
   FIND_LIBRARY(AUDIO_UNIT_LIBRARY AudioUnit)
   FIND_LIBRARY(FOUNDATION_LIBRARY Foundation)
   FIND_LIBRARY(OPEN_AL_LIBRARY OpenAL)
   FIND_LIBRARY(CORE_DATA_LIBRARY CoreData)

   MARK_AS_ADVANCED (CORE_VIDEO_LIBRARY
                     CORE_AUDIO_LIBRARY
                     CORE_MEDIA_LIBRARY
                     CORE_GRAPHICS_LIBRARY
                     AV_FOUNDATION_LIBRARY
                     AUDIO_TOOLBOX_LIBRARY
                     FOUNDATION_LIBRARY
                     OPEN_AL_LIBRARY
                     CORE_DATA_LIBRARY
                     AUDIO_UNIT_LIBRARY)
                     
   SET(EXTRA_LIBS ${CORE_VIDEO_LIBRARY} ${CORE_AUDIO_LIBRARY} ${CORE_MEDIA_LIBRARY}
    ${CORE_GRAPHICS_LIBRARY} ${AV_FOUNDATION_LIBRARY} ${AUDIO_TOOLBOX_LIBRARY} 
    ${FOUNDATION_LIBRARY} ${OPEN_AL_LIBRARY} ${CORE_DATA_LIBRARY} ${AUDIO_UNIT_LIBRARY})

ENDIF(APPLE AND NOT IOS)

set(LIBRARIES )

set(LANGUAGES rus eng)

set(RESOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/resources)

set(RESOURCES ${RESOURCE_PATH}/resources.qrc)

set(TRANSLATIONS_PATH ${RESOURCE_PATH}/translations)

project(${PROJECT})

SET(QT_USE_QTNETWORK TRUE)
SET(QT_USE_QTOPENGL TRUE)
SET(QT_USE_QTWEBKIT TRUE)
#SET(QT_USE_PHONON TRUE)
SET(QT_USE_QTSQL TRUE)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${LIBRARY_OUTPUT_PATH}/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/xmppbot/xwlib)

find_package(Qt4 REQUIRED)

include(${QT_USE_FILE})

qt4_add_resources(QRC_SOURCES ${RESOURCES})

qt4_wrap_cpp(MOC_SOURCES ${HEADERS})

foreach(LANGUAGE ${LANGUAGES})
        set (TS ${TRANSLATIONS_PATH}/${LANGUAGE}.ts)
        set (QM ${TRANSLATIONS_PATH}/${LANGUAGE}.qm)
        set (TRANSLATIONS ${TRANSLATIONS} ${TS})
        set (TRANSLATIONS_BINARY ${TRANSLATIONS_BINARY} ${QM})
        add_custom_command (
                OUTPUT ${QM}
                COMMAND ${QT_LRELEASE_EXECUTABLE} ${TS}
                MAIN_DEPENDENCY ${TS})
endforeach()
add_custom_target(
        translations
        COMMAND ${QT_LUPDATE_EXECUTABLE} ${HEADERS} ${SOURCES} -ts ${TRANSLATIONS})
add_custom_command(
        TARGET translations
        COMMAND ${QT_LRELEASE_EXECUTABLE} ${TRANSLATIONS})

#foreach(LIBRARY ${LIBRARIES})
#        add_subdirectory (${LIBRARY}/build bin/${LIBRARY})
#endforeach()


set(FORMS ui/mainwindow.ui ui/displayframe.ui
        ui/optionsframe.ui ui/calldialog.ui ui/messageform.ui)

qt4_wrap_ui( UI_HEADERS ${FORMS} )

if(MSVC)
        add_definitions(/W3)
elseif(CMAKE_COMPILER_IS_GNUCXX)
        add_definitions(-Wall -pedantic)
else()
        message("Unknown compiler")
endif()

source_group("Header Files" FILES ${HEADERS})
source_group("Source Files" FILES ${SOURCES})
source_group("Generated Files" FILES ${MOC_SOURCES})
source_group("Resource Files" FILES ${QRC_SOURCES})

set(CORELIBS  m c stdc++ )
set(CMAKE_C_FLAGS "-Wall -pthread -O0 -g3 -gdwarf-2")
set(CMAKE_CXX_FLAGS "-Wall -pthread -O0 -g3 -gdwarf-2 -fno-rtti -fno-exceptions")

add_executable(${PROJECT} ${HEADERS}  ${UI_HEADERS} ${SOURCES}
${MOC_SOURCES} ${QRC_SOURCES} ${TRANSLATIONS})

target_link_libraries(${PROJECT} ${CORELIBS} ${LIBRARIES} ${QT_LIBRARIES}
 ${QT_QTMAIN_LIBRARY} ${EXTRA_LIBS} xw xw++ sessions ev)

INSTALL(TARGETS GobeeQt DESTINATION bin)

SET(TARGET_VERSION_MAJOR 1)
SET(TARGET_VERSION_MINOR 2)
SET(CPACK_PACKAGE_VERSION "1.0.2")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "2")
SET(CPACK_STRIP_FILES xwclient)
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "GobeeQt")

INCLUDE(CPack)

ENABLE_TESTING()
