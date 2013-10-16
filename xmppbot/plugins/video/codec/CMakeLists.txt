#IF(NOT IOS)
 add_subdirectory( xvpx )
#ENDIF(NOT IOS)
add_subdirectory( xtheora )
add_subdirectory( xhid )


link_directories( ${lib_contrib_dir}  )

INCLUDE(CPack)

ENABLE_TESTING()

