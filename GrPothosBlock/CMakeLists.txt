########################################################################
# Build gnuradio block adapter module
#
# This module contains a Pothos::Block which can wrap a gr::block
########################################################################

#create headers to extend friend access to the GrPothosBlock() class
#buffer.h: Allow access to buffer members to point to a custom location
#basic_block.h: Allow access to has_msg_handler and dispatch_msg
#block.h: Allow access to loggers
unset(header_contents)
foreach(header "buffer.h" "basic_block.h" "block.h")
    file(READ "${GR_INCLUDE_ROOT}/gnuradio/${header}" _header_contents)
    set(header_contents "${header_contents}\n${_header_contents}")
endforeach(header)

string(REPLACE "public:" "public:\n    friend class ::GrPothosBlock;" header_contents "${header_contents}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gr_pothos_access.h" "${header_contents}")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

#gr libs may or be compiled with counters
#specify this flag just in-case because
#the block_executor may use an extra field
add_definitions(-DGR_PERFORMANCE_COUNTERS)

include_directories(${JSON_HPP_INCLUDE_DIR})
list(APPEND test_sources test_simple_blocks.cc)

POTHOS_MODULE_UTIL(
    TARGET GrPothosBlock
    SOURCES
        pothos_block.cc
        pothos_pmt_helper.cc
        pothos_infer_dtype.cc
        pothos_log4cpp_appender.cc
        gnuradio_info.cc
        test_dtype.cc
        test_logging.cc
        test_pmt_helper.cc
        ${appender_sources}
        ${test_sources}
    LIBRARIES
        gnuradio::gnuradio-runtime
        gnuradio::gnuradio-blocks
    DESTINATION blocks/gnuradio
)
