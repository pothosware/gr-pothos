########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8)
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
find_python_module(ply REQUIRED)
find_python_module(mako REQUIRED)

########################################################################
# Setup gnuradio dependency
########################################################################
message(STATUS "")
message(STATUS "")
message(STATUS "######################################################")
message(STATUS "## Configuring GNU Radio dependency...")
message(STATUS "######################################################")
find_package(Gnuradio CONFIG)
if (NOT Gnuradio_FOUND)
    message(FATAL "Requires GNU Radio")
endif()

include_directories(${GNURADIO_PMT_INCLUDE_DIRS})
include_directories(${GNURADIO_RUNTIME_INCLUDE_DIRS})
include_directories(${GNURADIO_BLOCKS_INCLUDE_DIRS})
include_directories(${GNURADIO_ANALOG_INCLUDE_DIRS})
include_directories(${GNURADIO_DIGITAL_INCLUDE_DIRS})
include_directories(${GNURADIO_FFT_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/missing/include)

list(APPEND __GNURADIO_LIBRARIES
    ${GNURADIO_PMT_LIBRARIES}
    ${GNURADIO_RUNTIME_LIBRARIES}
    ${GNURADIO_BLOCKS_LIBRARIES}
    ${GNURADIO_ANALOG_LIBRARIES}
    ${GNURADIO_DIGITAL_LIBRARIES}
    ${GNURADIO_FFT_LIBRARIES})

message(STATUS "GNURADIO_LIBRARIES:")
foreach (lib ${__GNURADIO_LIBRARIES})
    if (lib)
        list(APPEND GNURADIO_LIBRARIES ${lib})
        message(STATUS "  ${lib}")
    endif (lib)
endforeach (lib)

########################################################################
# inspect gr install
########################################################################
if (NOT GR_ROOT)
    get_filename_component(GR_ROOT "${GNURADIO_RUNTIME_INCLUDE_DIRS}/.." ABSOLUTE)
endif ()
set(GR_INCLUDE_ROOT ${GR_ROOT}/include/gnuradio)
set(GRC_BLOCKS_ROOT ${GR_ROOT}/share/gnuradio/grc/blocks)

message(STATUS "GR_ROOT: ${GR_ROOT}")
message(STATUS "GR_INCLUDE_ROOT: ${GR_INCLUDE_ROOT}")
message(STATUS "GRC_BLOCKS_ROOT: ${GRC_BLOCKS_ROOT}")

if (NOT EXISTS ${GR_INCLUDE_ROOT})
    message(FATAL "GNU Radio include directory ${GR_INCLUDE_ROOT} does not exist")
endif()

if (NOT EXISTS ${GRC_BLOCKS_ROOT})
    message(FATAL "GRC blocks directory ${GRC_BLOCKS_ROOT} does not exist")
endif()

########################################################################
# Setup pothos dependency
########################################################################
message(STATUS "")
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
# Setup boost
########################################################################
message(STATUS "")
message(STATUS "")
message(STATUS "######################################################")
message(STATUS "## Configuring Boost C++ Libraries...")
message(STATUS "######################################################")

# Although not required on my system, some users have linking issues without
SET(BOOST_REQUIRED_COMPONENTS
    thread
    system
)

if(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")
    list(APPEND BOOST_LIBRARYDIR "/usr/lib64") #fedora 64-bit fix
endif(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")

set(Boost_ADDITIONAL_VERSIONS
    "1.35.0" "1.35" "1.36.0" "1.36" "1.37.0" "1.37" "1.38.0" "1.38" "1.39.0" "1.39"
    "1.40.0" "1.40" "1.41.0" "1.41" "1.42.0" "1.42" "1.43.0" "1.43" "1.44.0" "1.44"
    "1.45.0" "1.45" "1.46.0" "1.46" "1.47.0" "1.47" "1.48.0" "1.48" "1.49.0" "1.49"
    "1.50.0" "1.50" "1.51.0" "1.51" "1.52.0" "1.52" "1.53.0" "1.53" "1.54.0" "1.54"
    "1.55.0" "1.55" "1.56.0" "1.56" "1.57.0" "1.57" "1.58.0" "1.58" "1.59.0" "1.59"
    "1.60.0" "1.60" "1.61.0" "1.61" "1.62.0" "1.62" "1.63.0" "1.63" "1.64.0" "1.64"
    "1.65.0" "1.65" "1.66.0" "1.66" "1.67.0" "1.67" "1.68.0" "1.68" "1.69.0" "1.69"
)

find_package(Boost COMPONENTS ${BOOST_REQUIRED_COMPONENTS})

if(NOT Boost_FOUND)
    message(FATAL "Requires Boost")
endif()

ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")

########################################################################
# json.hpp header
########################################################################
find_path(JSON_HPP_INCLUDE_DIR NAMES json.hpp PATH_SUFFIXES nlohmann)

if (NOT JSON_HPP_INCLUDE_DIR)
    message(WARNING "gr-pothos requires json.hpp, skipping...")
endif (NOT JSON_HPP_INCLUDE_DIR)

########################################################################
# Discover installed components
########################################################################
message(STATUS "")
message(STATUS "")
message(STATUS "######################################################")
message(STATUS "## Locate installed GNU Radio components...")
message(STATUS "######################################################")

list(APPEND COMPONENT_BLACKLIST
    uhd fcd comedi #ignore devices, pothos-sdr
    wxgui qtgui #ignore graphics, pothos-widgets
    messages thread swig #runtime headers
)

file(GLOB inc_dirs "${GR_INCLUDE_ROOT}/*")
foreach (inc_dir ${inc_dirs})
    get_filename_component(comp_name ${inc_dir} NAME)
    list(FIND COMPONENT_BLACKLIST ${comp_name} blacklisted)
    file(GLOB grc_blocks "${GRC_BLOCKS_ROOT}/${comp_name}_*.xml")
    list(LENGTH grc_blocks num_grc_blocks)
    if (IS_DIRECTORY ${inc_dir} AND ${blacklisted} EQUAL -1)
        find_library(${comp_name}_LIBRARY gnuradio-${comp_name})
    endif()

    if (NOT IS_DIRECTORY ${inc_dir})
        #pass
    elseif (NOT (${blacklisted} EQUAL -1))
        message(STATUS "Found ${comp_name} - skipped: blacklisted")
    elseif(NOT ${comp_name}_LIBRARY)
        message(STATUS "Found ${comp_name} - skipped: ${${comp_name}_LIBRARY}")
    elseif(${num_grc_blocks} EQUAL 0)
        message(STATUS "Found ${comp_name} - skipped: no GRC blocks")
    else()
        message(STATUS "Found ${comp_name} - OK")
        list(APPEND COMP_NAMES ${comp_name})
    endif()

endforeach (inc_dir)

########################################################################
# Build block adapter
#
# Test with PothosUtil --self-tests=/gnuradio
########################################################################
add_subdirectory(GrPothosBlock)

########################################################################
# Build modules
########################################################################
foreach (comp_name ${COMP_NAMES})

    set(doc_sources "")
    set(wrapper_output
        ${CMAKE_CURRENT_BINARY_DIR}/${comp_name}_wrapper.cc)

    file(GLOB_RECURSE headers "${GR_INCLUDE_ROOT}/${comp_name}/*.h")
    file(GLOB_RECURSE grc_blocks "${GRC_BLOCKS_ROOT}/${comp_name}_*.xml")

    #custom target for manual build of wrapper for debugging
    add_custom_target(${comp_name}_wrapper DEPENDS ${wrapper_output})

    add_custom_command(
        OUTPUT ${wrapper_output}
        COMMAND ${PYTHON_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/GrPothosUtil.py
            --target=${comp_name}
            --prefix=${GR_ROOT}
            --out=${wrapper_output}
            --log=${CMAKE_CURRENT_BINARY_DIR}/${comp_name}.log
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/registration.tmpl.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/GrPothosUtil.py
            ${CMAKE_CURRENT_SOURCE_DIR}/CppHeaderParser.py
            ${CMAKE_CURRENT_SOURCE_DIR}/xmltodict.py
            ${CMAKE_CURRENT_SOURCE_DIR}/blacklist.json
            ${headers}
            ${grc_blocks}
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
            channels/phase_noise_gen.cc)
        list(APPEND wrapper_output
            channels/amp_bal.cc
            channels/conj_fs_iqcorr.cc
            channels/distortion_2_gen.cc
            channels/distortion_3_gen.cc
            channels/impairments.cc
            channels/iqbal_gen.cc
            channels/phase_bal.cc
            channels/phase_noise_gen.cc)
    endif()

    if ("${comp_name}" STREQUAL "fec")
        list(APPEND wrapper_output
            fec/generic_decoder.cc
            fec/generic_encoder.cc
            fec/mtrx.cc
        )
    endif()

    if ("${comp_name}" STREQUAL "filter")
        list(APPEND doc_sources filter/optfirdes.cc)
        list(APPEND wrapper_output
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
        LIBRARIES
            ${${comp_name}_LIBRARY}
            ${GNURADIO_LIBRARIES}
            ${Boost_LIBRARIES}
        DESTINATION gnuradio
    )

endforeach (comp_name)

POTHOS_MODULE_UTIL(
    TARGET runtimeSupport
    SOURCES runtime_wrapper.cc
    LIBRARIES
        ${GNURADIO_LIBRARIES}
        ${Boost_LIBRARIES}
    DESTINATION gnuradio
)

message(STATUS "")
message(STATUS "")
