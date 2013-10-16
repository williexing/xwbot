INCLUDE(${PROJECT_SOURCE_DIR}/INCLUDES.cmk)

link_directories( ${lib_contrib_dir}  )

#IF(GLES2)
set(MYGLSOURCES droid_gles2_renderer.c)
set(GLES_LIBS GLESv2)
#ENDIF(GLES2)

add_library(xwjni droid_common.c droid_cam_jni.c 
	droid_cam_plugin.c droid_display_plugin.c
	droid_hid_jni.c ${MYGLSOURCES} )
	 
target_link_libraries(xwjni xmppbot log m ${GLES_LIBS})

INSTALL(TARGETS xwjni DESTINATION lib)

ENABLE_TESTING()

