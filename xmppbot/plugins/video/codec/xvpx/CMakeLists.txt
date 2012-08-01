INCLUDE(${PROJECT_SOURCE_DIR}/INCLUDES.cmk)

INCLUDE_DIRECTORIES ( ${CMAKE_CURRENT_SOURCE_DIR}/vpx/${X_ARCH}/include )
LINK_DIRECTORIES ( ${CMAKE_CURRENT_SOURCE_DIR}/vpx/${X_ARCH}/ )

SET(XVPX_SRC x_vpx_plugin.c x_vpx.c x_vp8.c ${PROJECT_SOURCE_DIR}/x_dll.c)

IF(ANDROID)
INCLUDE_DIRECTORIES(/DATA/android/android-ndk-r5/sources/android/cpufeatures/)
SET(XVPX_SRC ${XVPX_SRC} "/DATA/android/android-ndk-r5/sources/android/cpufeatures/cpu-features.c")
ENDIF(ANDROID)

ADD_LIBRARY ( xvpx ${XVPX_SRC} )
TARGET_LINK_LIBRARIES( xvpx vpx xw )

IF(IOS)
ADD_LIBRARY ( xvpx_rtp_send main.c udp.c )
ELSE(IOS)
ADD_EXECUTABLE ( xvpx_rtp_send main.c udp.c )
ENDIF(IOS)

TARGET_LINK_LIBRARIES( xvpx_rtp_send xvpx )

IF(WIN32)
TARGET_LINK_LIBRARIES( xvpx_rtp_send ws2_32 )
ELSEIF(ANDROID)
TARGET_LINK_LIBRARIES( xvpx_rtp_send log m )
TARGET_LINK_LIBRARIES( xvpx log m )
ENDIF(WIN32)

add_dependencies( xvpx_rtp_send xvpx )


INCLUDE(CPack)
INCLUDE(CTest)

ENABLE_TESTING()

