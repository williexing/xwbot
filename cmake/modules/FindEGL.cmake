# - Try to find EGL
# Once done this will define
#  
#  EGL_FOUND        - system has EGL
#  EGL_INCLUDE_DIR  - the EGL include directory
#  EGL_LIBRARIES    - Link these to use EGL

IF( WIN32 )

	FIND_PATH( EGL_INCLUDE_DIR EGL/egl.h
		$ENV{PROGRAMFILES}/EGL/include
	)

	FIND_LIBRARY( EGL_LIBRARIES
		NAMES egl libEGL
		PATHS
		$ENV{PROGRAMFILES}/EGL/lib
	)

ELSE()

	FIND_PATH( EGL_INCLUDE_DIR EGL/egl.h
	  /usr/openwin/share/include
	  /opt/graphics/OpenGL/include /usr/X11R6/include
	  /usr/include
	)

	FIND_LIBRARY( EGL_LIBRARIES
	  NAMES EGL
	  PATHS /opt/graphics/OpenGL/lib
			/usr/openwin/lib
			/usr/shlib /usr/X11R6/lib
			/usr/lib
	)

ENDIF()

SET( EGL_FOUND "NO" )
IF( EGL_LIBRARIES AND EGL_INCLUDE_DIR )

    SET( EGL_FOUND "YES" )

ENDIF()

include( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EGL  DEFAULT_MSG  EGL_LIBRARIES EGL_INCLUDE_DIR)

MARK_AS_ADVANCED(
  EGL_INCLUDE_DIR
  EGL_LIBRARIES
)

