add_subdirectory( codec )
add_subdirectory( vcam )

link_directories( ${lib_contrib_dir}  )
INCLUDE(CPack)
ENABLE_TESTING()

