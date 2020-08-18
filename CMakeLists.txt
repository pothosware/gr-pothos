########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8.12)
project(GrPothos CXX C)
enable_testing()

########################################################################
# Setup the python interpreter:
# This allows the user to specify a specific interpreter,
# or finds the interpreter via the built-in cmake module.
########################################################################
#this allows the user to override PYTHON_EXECUTABLE
if(PYTHON_EXECUTABLE)

    set(PYTHONINTERP_FOUND TRUE)

#otherwise if not set, try to automatically find it
else(PYTHON_EXECUTABLE)

    #use the built-in find script
    find_package(PythonInterp 2)

    #and if that fails use the find program routine
    if(NOT PYTHONINTERP_FOUND)
        find_program(PYTHON_EXECUTABLE NAMES python python2 python2.7 python2.6 python2.5)
        if(PYTHON_EXECUTABLE)
            set(PYTHONINTERP_FOUND TRUE)
        endif(PYTHON_EXECUTABLE)
    endif(NOT PYTHONINTERP_FOUND)

endif(PYTHON_EXECUTABLE)

if (NOT PYTHONINTERP_FOUND)
    message(FATAL "Failed to find Python2 interpreter - override PYTHON_EXECUTABLE")
endif()
message(STATUS "PYTHON_EXECUTABLE: ${PYTHON_EXECUTABLE}")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
include(FindPythonModule)
# TODO: check for BlockTool

list(APPEND COMPONENT_BLACKLIST
    uhd fcd comedi #ignore devices, pothos-sdr
    wxgui qtgui #ignore graphics, pothos-widgets
    messages thread swig #runtime headers
)

########################################################################
# Setup pothos dependency
########################################################################
message(STATUS "")
message(STATUS "######################################################")
message(STATUS "## Configuring Pothos dependency...")
message(STATUS "######################################################")
find_package(Pothos 0.7 CONFIG)
if (NOT Pothos_FOUND)
    message(FATAL "Requires Pothos")
endif()

#hide lots of warnings caused by logger on every file
if (CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wno-unused-parameter)
endif()

if(MSVC)
    add_definitions(/bigobj) #very large generated files
    add_definitions(/MP) #multi-core build
endif(MSVC)

########################################################################
# json.hpp header
########################################################################
find_path(JSON_HPP_INCLUDE_DIR NAMES json.hpp PATH_SUFFIXES nlohmann)

if (NOT JSON_HPP_INCLUDE_DIR)
    message(WARNING "gr-pothos requires json.hpp, skipping...")
endif (NOT JSON_HPP_INCLUDE_DIR)

########################################################################
# We need our components before looking for GNU Radio, so just check
# for a header to get the directory beforehand.
########################################################################
if(NOT GR_INCLUDE_ROOT)
    find_path(GR_INCLUDE_ROOT
        NAMES gnuradio/config.h
        PATHS /usr/local/include /usr/include)
    if(NOT GR_INCLUDE_ROOT)
        message(FATAL "Could not find GNU Radio headers")
    endif()
endif()

get_filename_component(GR_ROOT ${GR_INCLUDE_ROOT} DIRECTORY CACHE "GR_ROOT")

########################################################################
# Discover installed GNU Radio components
########################################################################
message(STATUS "")
message(STATUS "######################################################")
message(STATUS "## Locate installed GNU Radio components...")
message(STATUS "######################################################")

file(GLOB inc_dirs "${GR_INCLUDE_ROOT}/gnuradio/*")
foreach (inc_dir ${inc_dirs})
    get_filename_component(comp_name ${inc_dir} NAME)
    list(FIND COMPONENT_BLACKLIST ${comp_name} blacklisted)
    if (IS_DIRECTORY ${inc_dir} AND ${blacklisted} EQUAL -1)
        message(STATUS "Found ${comp_name} - OK")

        string(REPLACE "_" "-" comp_name ${comp_name})
        list(APPEND COMP_NAMES ${comp_name})
    elseif(IS_DIRECTORY ${inc_dir})
        message(STATUS "Found ${comp_name} - skipped: blacklisted")
    endif()
endforeach (inc_dir)

########################################################################
# Find GNU Radio
########################################################################
message(STATUS "")
find_package(Gnuradio 3.8 REQUIRED COMPONENTS ${COMP_NAMES})

########################################################################
# Build block adapter
########################################################################
add_subdirectory(GrPothosBlock)

########################################################################
# Build modules
########################################################################
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

foreach (comp_name ${COMP_NAMES})

    # Not all versions search for GSL when needed, so do so here.
    if("${comp_name}" STREQUAL "fec")
        find_package(GSL)
    elseif("${comp_name}" STREQUAL "wavelet")
        find_package(GSL)
    endif()

    set(blocktool_output_dir ${CMAKE_CURRENT_BINARY_DIR}/blocktool_output/${comp_name})
    set(blocktool_output_path ${blocktool_output_dir}/make_target)
    add_custom_target(${comp_name}_blocktool_output ALL DEPENDS ${blocktool_output_path})

    add_custom_command(
        OUTPUT ${blocktool_output_path}
        COMMENT "Generating ${comp_name} JSON (this may take a while...)"
        COMMAND ${PYTHON_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/GenerateBlockToolJSON.py
            --prefix=${GR_ROOT}
            --output-dir=${blocktool_output_dir}
            ${comp_name}
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/GenerateBlockToolJSON.py
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/common/JSONGenerator.py)

    set(doc_sources "")
    set(wrapper_output
        ${CMAKE_CURRENT_BINARY_DIR}/${comp_name}_wrapper.cc)

    #custom target for manual build of wrapper for debugging
    add_custom_target(${comp_name}_wrapper DEPENDS ${wrapper_output})

    add_custom_command(
        OUTPUT ${wrapper_output}
        COMMAND ${PYTHON_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/GenerateWrapperCCFromJSON.py
            --prefix=${GR_ROOT}
            --input-dir=${blocktool_output_dir}
            --out=${wrapper_output}
        DEPENDS
            ${comp_name}_blocktool_output
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/GenerateWrapperCCFromJSON.py
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/common/JSONParser.py
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/registration.tmpl.cpp
            ${headers}
    )

    if ("${comp_name}" STREQUAL "blocks")
        list(APPEND doc_sources
            blocks/convert.cc
        )
        list(APPEND wrapper_output
            blocks/convert.cc
        )
    endif()

    if ("${comp_name}" STREQUAL "channels")
        list(APPEND doc_sources
            channels/amp_bal.cc
            channels/conj_fs_iqcorr.cc
            channels/distortion_2_gen.cc
            channels/distortion_3_gen.cc
            channels/impairments.cc
            channels/iqbal_gen.cc
            channels/phase_bal.cc
            channels/phase_noise_gen.cc
            channels/quantizer.cc)
        list(APPEND wrapper_output
            channels/amp_bal.cc
            channels/conj_fs_iqcorr.cc
            channels/distortion_2_gen.cc
            channels/distortion_3_gen.cc
            channels/impairments.cc
            channels/iqbal_gen.cc
            channels/phase_bal.cc
            channels/phase_noise_gen.cc
            channels/quantizer.cc)
    endif()

    if ("${comp_name}" STREQUAL "fec")
        list(APPEND wrapper_output
            fec/generic_decoder.cc
            fec/generic_encoder.cc
            fec/mtrx.cc
        )
    endif()

    if ("${comp_name}" STREQUAL "filter")
        list(APPEND doc_sources
            filter/firdes.cc
            filter/optfirdes.cc)
        list(APPEND wrapper_output
            filter/firdes.cc
            filter/optfir.cc
            filter/optfirdes.cc)
    endif()

    if ("${comp_name}" STREQUAL "trellis")
        list(APPEND wrapper_output
            trellis/fsm.cc
        )
    endif()

    POTHOS_MODULE_UTIL(
        TARGET ${comp_name}Support
        SOURCES ${wrapper_output}
        DOC_SOURCES ${doc_sources}
        LIBRARIES gnuradio::gnuradio-${comp_name}
        DESTINATION gnuradio
    )

endforeach (comp_name)

POTHOS_MODULE_UTIL(
    TARGET runtimeSupport
    SOURCES runtime_wrapper.cc
    LIBRARIES gnuradio::gnuradio-runtime
    DESTINATION gnuradio
)

message(STATUS "")
message(STATUS "")
