include(FindPkgConfig OPTIONAL)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CAIRO cairo QUIET)
endif()

find_path(CAIRO_INCLUDE_DIR NAMES cairo/cairo.h
                              PATHS ${PC_CAIRO_INCLUDEDIR})
find_library(CAIRO_LIBRARY NAMES cairo libcairo
                             PATHS ${PC_CAIRO_LIBDIR})

set(CAIRO_VERSION ${PC_CAIRO_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cairo
                                  REQUIRED_VARS CAIRO_LIBRARY CAIRO_INCLUDE_DIR
                                  VERSION_VAR CAIRO_VERSION)

if(CAIRO_FOUND)
  set(CAIRO_LIBRARIES ${CAIRO_LIBRARY})
  set(CAIRO_INCLUDE_DIRS ${CAIRO_INCLUDE_DIR})
  if(PC_CAIRO_CFLAGS)
    set(CAIRO_DEFINITIONS ${PC_CAIRO_CFLAGS})
  endif()

  if(NOT TARGET Cairo::Cairo)
    add_library(Cairo::Cairo UNKNOWN IMPORTED)
    set_target_properties(Cairo::Cairo PROPERTIES
                                           IMPORTED_LOCATION "${CAIRO_LIBRARY}"
                                           INTERFACE_INCLUDE_DIRECTORIES "${CAIRO_INCLUDE_DIR}"
                                           INTERFACE_COMPILE_OPTIONS "${CAIRO_DEFINITIONS}")
  endif()
endif()

mark_as_advanced(CAIRO_INCLUDE_DIR CAIRO_LIBRARY)

