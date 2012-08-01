
#SET(BUILD_SHARED_LIBS OFF)

add_subdirectory( auth_plain )
#add_subdirectory( auth_md5 )
add_subdirectory( crypt )
#add_subdirectory( ft )
add_subdirectory( disco )
add_subdirectory( bind )
#add_subdirectory( inband_shell )
add_subdirectory( core )
add_subdirectory( sessions_api )
add_subdirectory( ice )
add_subdirectory( video )
add_subdirectory( audio )
#add_subdirectory( xiapi )
add_subdirectory( cmd_api )
add_subdirectory( jingle )
add_subdirectory( public_api )
add_subdirectory( eventdomains )

#ADD_LIBRARY( plugins SHARED fakeplugin.c)
#TARGET_LINK_LIBRARIES(plugins auth_plain xwcrypt discovery bind core xiapi commands)
#SET_TARGET_PROPERTIES(plugins PROPERTIES LINK_FLAGS "-Wl,--whole-archive -llog -lssl -lcrypto")
#SET_TARGET_PROPERTIES(plugins PROPERTIES LINK_FLAGS "${LINK_FLAGS} -llog")
#LINK_DIRECTORIES( ${lib_contrib_dir}  )
#LINK_DIRECTORIES( ${include_top_dir}/3dparty/openssl  )

link_directories( ${lib_contrib_dir}  )

INCLUDE(CPack)

ENABLE_TESTING()

