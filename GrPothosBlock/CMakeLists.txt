########################################################################
# Build gnuradio block adapter module
#
# This module contains a Pothos::Block which can wrap a gr::block
########################################################################

# This will already be included in the GNURADIO_LIBRARIES CMake variable,
# but we need to look for it here to recreate the #define needed to enable logging.
find_package(Log4cpp)
if(LOG4CPP_FOUND)
    add_definitions(-DENABLE_GR_LOG -DHAVE_LOG4CPP)
    set(appender_sources
        pothos_log4cpp_appender.cc
        test_logging.cc
    )
endif()

#create headers to extend friend access to the GrPothosBlock() class
#buffer.h: Allow access to buffer members to point to a custom location
#basic_block.h: Allow access to has_msg_handler and dispatch_msg
unset(header_contents)
foreach(header "buffer.h" "basic_block.h" "block.h")
    file(READ "${GR_INCLUDE_ROOT}/${header}" _header_contents)
    set(header_contents "${header_contents}\n${_header_contents}")
endforeach(header)

string(REPLACE "public:" "public:\n    friend class ::GrPothosBlock;" header_contents "${header_contents}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gr_pothos_access.h" "${header_contents}")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

#gr libs may or be compiled with counters
#specify this flag just in-case because
#the block_executor may use an extra field
add_definitions(-DGR_PERFORMANCE_COUNTERS)

#enable testing - due to API changes we require json.hpp
#and Pothos 0.6 for changes in the unit test blocks
find_path(JSON_HPP_INCLUDE_DIR NAMES json.hpp PATH_SUFFIXES nlohmann)
if (JSON_HPP_INCLUDE_DIR AND NOT "${Pothos_VERSION}" VERSION_LESS "0.6.0")
    include_directories(${JSON_HPP_INCLUDE_DIR})
    list(APPEND test_sources test_simple_blocks.cc)
else()
    message(WARNING "Block tests disabled (require Pothos>=0.6.0 and json.hpp")
endif()

POTHOS_MODULE_UTIL(
    TARGET GrPothosBlock
    SOURCES
        pothos_block.cc
        pothos_pmt_helper.cc
        pothos_infer_dtype.cc
        gnuradio_info.cc
        test_pmt_helper.cc
        ${appender_sources}
        ${test_sources}
    LIBRARIES
        ${GNURADIO_LIBRARIES}
        ${Boost_LIBRARIES}
    DESTINATION blocks/gnuradio
)
