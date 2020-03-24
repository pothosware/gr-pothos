/*
 * Copyright 2020 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <gnuradio/types.h>

#include <string>
#include <vector>
#include <iostream>

static void checkBlockPort(
    const Pothos::Proxy& block,
    bool isInput,
    const std::string& typeName,
    size_t vlen)
{
    const std::string portType = isInput ? "input" : "output";

    POTHOS_TEST_EQUAL(
        typeName,
        block.call(portType, 0).call("dtype").call<std::string>("name"));
    POTHOS_TEST_EQUAL(
        vlen,
        block.call(portType, 0).call("dtype").call<size_t>("dimension"));
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_infer_dtype_from_suffix)
{
    struct TestParams
    {
        std::string blockName;
        Pothos::Object blockParam0;
        Pothos::Object blockParam1;
        bool vlenBeforeParam1;

        std::string inputType;
        std::string outputType;
    };

    const std::vector<TestParams> vlenTestParams =
    {
        // Same input and output type
        {"/gr/blocks/and", Pothos::Object("and_bb"), Pothos::Object(), false, "uint8", "uint8"},
        {"/gr/blocks/add", Pothos::Object("add_ss"), Pothos::Object(), false, "int16", "int16"},
        {"/gr/blocks/add", Pothos::Object("add_ii"), Pothos::Object(), false, "int32", "int32"},
        {"/gr/blocks/add", Pothos::Object("add_ff"), Pothos::Object(), false, "float32", "float32"},
        {"/gr/blocks/add", Pothos::Object("add_cc"), Pothos::Object(), false, "complex_float32", "complex_float32"},

        // Sinks
        {"/gr/blocks/vector_sink", Pothos::Object("vector_sink_b"), Pothos::Object(1024), true, "uint8", ""},
        {"/gr/blocks/vector_sink", Pothos::Object("vector_sink_s"), Pothos::Object(1024), true, "int16", ""},
        {"/gr/blocks/vector_sink", Pothos::Object("vector_sink_i"), Pothos::Object(1024), true, "int32", ""},
        {"/gr/blocks/vector_sink", Pothos::Object("vector_sink_f"), Pothos::Object(1024), true, "float32", ""},
        {"/gr/blocks/vector_sink", Pothos::Object("vector_sink_c"), Pothos::Object(1024), true, "complex_float32", ""},

        // TODO: sources, different input/output types
    };
    for(const auto& testParams: vlenTestParams)
    {
        for(size_t vlen = 1; vlen <= 4; ++vlen)
        {
            std::cout << " * Testing " << testParams.blockName << ", vlen=" << vlen << " (";
            if(!testParams.inputType.empty())
            {
                std::cout << testParams.inputType;
                if(!testParams.outputType.empty()) std::cout << " ";
            }
            if(!testParams.outputType.empty()) std::cout << "-> " << testParams.outputType;
            std::cout << ")" << std::endl;

            Pothos::Proxy block;
            if(testParams.blockParam1)
            {
                block = testParams.vlenBeforeParam1 ? Pothos::BlockRegistry::make(testParams.blockName, testParams.blockParam0, vlen, testParams.blockParam1)
                                                    : Pothos::BlockRegistry::make(testParams.blockName, testParams.blockParam0, testParams.blockParam1, vlen);
            }
            else
            {
                block = Pothos::BlockRegistry::make(testParams.blockName, testParams.blockParam0, vlen);
            }

            if(!testParams.inputType.empty()) checkBlockPort(block, true /*isInput*/, testParams.inputType, vlen);
            if(!testParams.outputType.empty()) checkBlockPort(block, false /*isInput*/, testParams.outputType, vlen);
        }
    }

    // _vxx blocks don't have vlen, but we need to make sure the "v" doesn't break it.
    const std::vector<TestParams> noVLenTestParams =
    {
        {"/gr/blocks/add_const", Pothos::Object("add_const_bb"), Pothos::Object(0), false, "uint8", "uint8"},
        {"/gr/blocks/add_const", Pothos::Object("add_const_ss"), Pothos::Object(0), false, "int16", "int16"},
        {"/gr/blocks/add_const", Pothos::Object("add_const_ii"), Pothos::Object(0), false, "int32", "int32"},
        {"/gr/blocks/add_const", Pothos::Object("add_const_ff"), Pothos::Object(0), false, "float32", "float32"},
        {"/gr/blocks/add_const", Pothos::Object("add_const_cc"), Pothos::Object(0), false, "complex_float32", "complex_float32"},
        {"/gr/blocks/add_const", Pothos::Object("add_const_vbb"), Pothos::Object(std::vector<unsigned char>{0}), false, "uint8", "uint8"},
        {"/gr/blocks/add_const", Pothos::Object("add_const_vss"), Pothos::Object(std::vector<short>{0}), false, "int16", "int16"},
        {"/gr/blocks/add_const", Pothos::Object("add_const_vii"), Pothos::Object(std::vector<int>{0}), false, "int32", "int32"},
        {"/gr/blocks/add_const", Pothos::Object("add_const_vff"), Pothos::Object(std::vector<float>{0.0f}), false, "float32", "float32"},
        {"/gr/blocks/add_const", Pothos::Object("add_const_vcc"), Pothos::Object(std::vector<gr_complex>{{0.0f,0.0f}}), false, "complex_float32", "complex_float32"},
    };
    for(const auto& testParams: noVLenTestParams)
    {
        std::cout << " * Testing " << testParams.blockName << " (";
        if(!testParams.inputType.empty()) std::cout << testParams.inputType << " ";
        if(!testParams.outputType.empty()) std::cout << "-> " << testParams.outputType;
        std::cout << ")" << std::endl;

        auto block = Pothos::BlockRegistry::make(testParams.blockName, testParams.blockParam0, testParams.blockParam1);

        if(!testParams.inputType.empty()) checkBlockPort(block, true /*isInput*/, testParams.inputType, 1 /*vlen*/);
        if(!testParams.outputType.empty()) checkBlockPort(block, false /*isInput*/, testParams.outputType, 1 /*vlen*/);
    }
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_infer_conversion_block_dtype)
{
    struct TestParams
    {
        std::string blockName;
        bool hasScale;

        std::string inputType;
        std::string outputType;
    };
    const std::vector<TestParams> allTestParams =
    {
        {"/gr/blocks/char_to_float", true, "int8", "float32"},
        {"/gr/blocks/char_to_short", false, "int8", "int16"},
        {"/gr/blocks/complex_to_float", false, "complex_float32", "float32"},
        {"/gr/blocks/float_to_char", true, "float32", "int8"},
        {"/gr/blocks/float_to_complex", false, "float32", "complex_float32"},
        {"/gr/blocks/float_to_int", true, "float32", "int32"},
        {"/gr/blocks/float_to_short", true, "float32", "int16"},
        {"/gr/blocks/int_to_float", true, "int32", "float32"},
        {"/gr/blocks/short_to_char", false, "int16", "int8"},
        {"/gr/blocks/short_to_float", true, "int16", "float32"},
    };

    for(const auto& testParams: allTestParams)
    {
        for(size_t vlen = 1; vlen <= 4; ++vlen)
        {
            std::cout << " * Testing " << testParams.blockName << ", vlen=" << vlen
                      << " (" << testParams.inputType << " -> " << testParams.outputType << ")" << std::endl;
            
            auto block = testParams.hasScale ? Pothos::BlockRegistry::make(testParams.blockName, vlen, 1.0f)
                                             : Pothos::BlockRegistry::make(testParams.blockName, vlen);

            checkBlockPort(block, true /*isInput*/, testParams.inputType, vlen);
            checkBlockPort(block, false /*isInput*/, testParams.outputType, vlen);
        }
    }

    // Note: uchar_to_float automatically has a dtype of 1 with no scale.
    auto ucharToFloat = Pothos::BlockRegistry::make("/gr/blocks/uchar_to_float");
    std::cout << " * Testing /gr/blocks/uchar_to_float" << std::endl;
    checkBlockPort(ucharToFloat, true /*isInput*/, "uint8", 1 /*vlen*/);
    checkBlockPort(ucharToFloat, false /*isInput*/, "float32", 1 /*vlen*/);

    // TODO: float_to_uchar, not generating for some reason
    //
    // Note: float_to_uchar automatically has a dtype of 1 with no scale.
    //auto floatToUChar = Pothos::BlockRegistry::make("/gr/blocks/float_to_uchar");
    //std::cout << " * Testing /gr/blocks/float_to_uchar" << std::endl;
    //checkBlockPort(floatToUChar, true /*isInput*/, "uint8", 1 /*vlen*/);
    //checkBlockPort(floatToUChar, false /*isInput*/, "float32", 1 /*vlen*/);
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_override_dtype)
{
    const std::vector<std::string> grDTypes = {"uint8", "int16", "int32", "float32", "complex_float32"};

    for(const auto& dtype: grDTypes)
    {
        auto nullSource = Pothos::BlockRegistry::make("/gr/blocks/null_source", dtype);
        checkBlockPort(nullSource, false /*isInput*/, dtype, 1 /*vlen*/);

        auto nullSink = Pothos::BlockRegistry::make("/gr/blocks/null_sink", dtype);
        checkBlockPort(nullSink, true /*isInput*/, dtype, 1 /*vlen*/);
    }
}
