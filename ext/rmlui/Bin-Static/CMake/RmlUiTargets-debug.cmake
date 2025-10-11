#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "RmlUi::Core" for configuration "Debug"
set_property(TARGET RmlUi::Core APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(RmlUi::Core PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/Bin-Static/Debug/rmlui.lib"
  )

list(APPEND _cmake_import_check_targets RmlUi::Core )
list(APPEND _cmake_import_check_files_for_RmlUi::Core "${_IMPORT_PREFIX}/Bin-Static/Debug/rmlui.lib" )

# Import target "RmlUi::Debugger" for configuration "Debug"
set_property(TARGET RmlUi::Debugger APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(RmlUi::Debugger PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/Bin-Static/Debug/rmlui_debugger.lib"
  )

list(APPEND _cmake_import_check_targets RmlUi::Debugger )
list(APPEND _cmake_import_check_files_for_RmlUi::Debugger "${_IMPORT_PREFIX}/Bin-Static/Debug/rmlui_debugger.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
