IF(NOT IOS AND NOT APPLE)
add_subdirectory( xvpx )
ENDIF(NOT IOS AND NOT APPLE)
add_subdirectory( xtheora )


link_directories( ${lib_contrib_dir}  )

INCLUDE(CPack)

ENABLE_TESTING()

