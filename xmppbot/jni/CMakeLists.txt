INCLUDE(${PROJECT_SOURCE_DIR}/INCLUDES.cmk)

link_directories( ${lib_contrib_dir}  )

add_library(xwjni mygl.c) 
target_link_libraries(xwjni xmppbot GLESv2 log m)

INSTALL(TARGETS xwjni DESTINATION lib)

ENABLE_TESTING()

