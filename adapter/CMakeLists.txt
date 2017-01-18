########################################################################
# Build gnuradio block adapter module
#
# This module contains a Pothos::Block which can wrap a gr::block
########################################################################

if (MSVC)
    #fix The C++ Standard Library forbids macroizing keywords
    add_definitions(-D_XKEYCHECK_H)
endif (MSVC)

POTHOS_MODULE_UTIL(
    TARGET GrPothosBlock
    SOURCES
        pothos_block.cc
        pothos_block_executor.cc
        pothos_pmt_helper.cc
        pothos_infer_dtype.cc
        test_pmt_helper.cc
    LIBRARIES
        ${GNURADIO_LIBRARIES}
        ${Boost_LIBRARIES}
    DESTINATION blocks/gnuradio
)
