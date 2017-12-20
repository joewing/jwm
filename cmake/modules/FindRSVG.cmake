include(FindPkgConfig OPTIONAL)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(RSVG librsvg-2.0)
endif()

