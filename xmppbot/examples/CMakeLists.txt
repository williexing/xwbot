IF(NOT ANDROID)
add_subdirectory( simplevideoserver )
ENDIF(NOT ANDROID)

link_directories( ${lib_contrib_dir}  )

INCLUDE(CPack)
INCLUDE(CTest)

ENABLE_TESTING()
