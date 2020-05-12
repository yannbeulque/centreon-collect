# Find Perl.
include(FindPerlLibs)
if (NOT PERLLIBS_FOUND)
  message(FATAL_ERROR "Could not find Perl libraries.")
endif ()
execute_process(COMMAND "${PERL_EXECUTABLE}" "-MExtUtils::Embed" "-e" "ldopts"
  RESULT_VARIABLE PERL_LDFLAGS_ERROR
  OUTPUT_VARIABLE PERL_LIBRARIES)
string(STRIP "${PERL_LIBRARIES}" PERL_LIBRARIES)
if (PERL_LDFLAGS_ERROR)
  set(PERL_LIBRARIES ${PERL_LIBRARY})
endif ()
execute_process(COMMAND "${PERL_EXECUTABLE}" "-MExtUtils::Embed" "-e" "ccopts"
  RESULT_VARIABLE PERL_CFLAGS_ERROR
  OUTPUT_VARIABLE PERL_CFLAGS)
if (NOT PERL_CFLAGS_ERROR)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PERL_CFLAGS}")
endif ()

# Disable some warnings generated by Embedded Perl.
get_property(EMBEDDED_PERL_CXXFLAGS SOURCE ${CMAKE_SOURCE_DIR}/perl/src/embedded_perl.cc
  PROPERTY COMPILE_FLAGS)
if (EMBEDDED_PERL_CXXFLAGS)
  string(REGEX REPLACE "-pedantic *"
    EMBEDDED_PERL_CXXFLAGS "${EMBEDDED_PERL_CXXFLAGS}")
  set_property(SOURCE "${CMAKE_SOURCE_DIR}/perl/src/embedded_perl.cc"
    PROPERTY COMPILE_FLAGS "${EMBEDDED_PERL_CXXFLAGS}")
endif ()

message(INFO ${PERL_CFLAGS})