add_subdirectory( codec )

IF(LINUX AND NOT WITHOUT_AUDIO)
add_subdirectory( vsiri )
add_subdirectory( vspkr )
add_subdirectory( vmic )
ENDIF(LINUX AND NOT WITHOUT_AUDIO)

IF(ANDROID)
add_subdirectory( androidspk )
ENDIF(ANDROID)

link_directories( ${lib_contrib_dir}  )
INCLUDE(CPack)
ENABLE_TESTING()


