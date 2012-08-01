IF(NOT WIN32 AND NOT IOS AND NOT APPLE)
add_subdirectory( speex )
ENDIF(NOT WIN32 AND NOT IOS AND NOT APPLE)
link_directories( ${lib_contrib_dir}  )
INCLUDE(CPack)
ENABLE_TESTING()

