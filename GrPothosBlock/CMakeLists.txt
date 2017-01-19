########################################################################
# Build gnuradio block adapter module
#
# This module contains a Pothos::Block which can wrap a gr::block
########################################################################

#create headers to extend friend access to the GrPothosBlock() class
#buffer.h: Allow access to buffer members to point to a custom location
#basic_block.h: Allow access to has_msg_handler and dispatch_msg
unset(header_contents)
foreach(header "buffer.h" "basic_block.h")
    file(READ "${GR_INCLUDE_ROOT}/${header}" _header_contents)
    set(header_contents "${header_contents}\n${_header_contents}")
endforeach(header)

string(REPLACE "  };" "    friend class ::GrPothosBlock;\n  };" header_contents "${header_contents}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gr_pothos_access.h" "${header_contents}")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

#gr libs may or be compiled with counters
#specify this flag just in-case because
#the block_executor may use an extra field
add_definitions(-DGR_PERFORMANCE_COUNTERS)

POTHOS_MODULE_UTIL(
    TARGET GrPothosBlock
    SOURCES
        pothos_block.cc
        pothos_pmt_helper.cc
        pothos_infer_dtype.cc
        test_pmt_helper.cc
        test_simple_blocks.cc
    LIBRARIES
        ${GNURADIO_LIBRARIES}
        ${Boost_LIBRARIES}
    DESTINATION blocks/gnuradio
)
