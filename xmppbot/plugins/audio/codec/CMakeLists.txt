IF(NOT WIN32)
add_subdirectory( xspeex )
ENDIF(NOT WIN32)

add_subdirectory( ilbc )
link_directories( ${lib_contrib_dir}  )
INCLUDE(CPack)
ENABLE_TESTING()

