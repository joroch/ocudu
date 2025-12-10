#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#
set(OCUDU_ENTERPRISE_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Links an OCUDU Enterprise library to an existing OCUDU Project library.
#
# The macro does two actions:
# - Adds the OCUDU Enterprise library to an existing OCUDU library target; and
# - Adds the compile definition OCUDU_HAS_ENTERPRISE to the source files that make up the existing library.
#
# OCUDU Enterprise libraries shall be linked to each one of the Project libraries containing functions to
# be replaced by their Enterprise version. Otherwise, the compile definitions might not take place in the source files and cause undefined behavior
# depending on the compilation tools.
#
# The usage for non-exported libraries:
#   add_library(ocudu_enterprise_lib OBJECT enterprise_source.cpp)
#   enterprise_link_library(ocudu_lib1  $<TARGET_OBJECTS:ocudu_enterprise_lib>)
#   enterprise_link_library(ocudu_lib2  $<TARGET_OBJECTS:ocudu_enterprise_lib>)
#
# For exported libraries:
#   add_library(ocudu_enterprise_lib OBJECT enterprise_source.cpp)
#   enterprise_link_library(ocudu_lib1 ocudu_enterprise_lib)
#   enterprise_link_library(ocudu_lib2 ocudu_enterprise_lib)
#   add_to_exported_libs(ocudu_enterprise_lib)
macro(ENTERPRISE_LINK_LIBRARY OCUDU_LIBRARY ENTERPRISE_LIBRARY)
  # Link enterprise library to the given OCUDU library.
  target_link_libraries(${OCUDU_LIBRARY} ${ENTERPRISE_LIBRARY})

  # Ensure the OCUDU target library is compiled with the definition.
  target_compile_definitions(${OCUDU_LIBRARY} PRIVATE -DOCUDU_HAS_ENTERPRISE)
endmacro(ENTERPRISE_LINK_LIBRARY)

########################################################################
# Plugins
########################################################################
add_subdirectory(lib)
if (BUILD_TESTING)
  add_subdirectory(tests)
endif()
# Hardware acceleration for both PUSCH and PDSCH is enabled by default when using DPDK.
if (ENABLE_DPDK)
  SET(ENABLE_PDSCH_HWACC ON CACHE BOOL "Enable PDSCH hardware-acceleration")
  message(STATUS "BBDEV hardware-acceleration enabled for PDSCH.")
  SET(ENABLE_PUSCH_HWACC ON CACHE BOOL "Enable PUSCH hardware-acceleration")
  message(STATUS "BBDEV hardware-acceleration enabled for PUSCH.")
else (ENABLE_DPDK)
  unset(ENABLE_PDSCH_HWACC CACHE)
  unset(ENABLE_PUSCH_HWACC CACHE)
endif (ENABLE_DPDK)
