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

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/Testing.hpp>

#include <Poco/Format.h>
#include <Poco/Thread.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// TODO: add overlay() to make scale visible or not based on types
struct BlockInfo
{
    std::string registryPath;
    bool hasScale;
};

static const BlockInfo& getBlockInfo(
    const std::string& inputType,
    const std::string& outputType)
{
    #define BlockInfoMapEntry(dtype1,dtype2,grType1,grType2,hasScale) \
        {dtype1 dtype2, {"/gr/blocks/" grType1 "_to_" grType2, hasScale}}

    static const std::unordered_map<std::string, BlockInfo> BlockInfoMap =
    {   
        BlockInfoMapEntry("int8", "float32", "char", "float", true),
        BlockInfoMapEntry("int8", "int16", "char", "short", false),
        BlockInfoMapEntry("int16", "int8", "short", "char", false),
        BlockInfoMapEntry("int16", "float32", "short", "float", true),
        BlockInfoMapEntry("int32", "float32", "int", "float", true),
        BlockInfoMapEntry("uint8", "float32", "char", "float", true),
        BlockInfoMapEntry("float32", "int8", "float", "char", true),
        BlockInfoMapEntry("float32", "int16", "float", "short", true),
        BlockInfoMapEntry("float32", "int32", "float", "int", true),
        BlockInfoMapEntry("float32", "complex_float32", "float", "complex", false),
        BlockInfoMapEntry("complex_float32", "float32", "complex", "float", false),
    };
    
    const auto key = inputType+outputType;
    auto blockInfoIter = BlockInfoMap.find(key);
    if(blockInfoIter != BlockInfoMap.end())
    {
        return blockInfoIter->second;
    }
    else
    {
        throw Pothos::InvalidArgumentException(
                   "GrConvert::make",
                   Poco::format(
                       "No valid conversion between %s and %s",
                       inputType,
                       outputType));
    }
}

/***********************************************************************
 * |PothosDoc Type Converter
 *
 * This is a convenience block that wraps around GNU Radio's various
 * type converter blocks. It chooses which block to use internally based
 * on input and output types specified by the caller.
 *
 * |category /GNURadio/Type Converters
 * |factory /gr/blocks/convert(inputDType, outputDType, vlen)
 * |setter set_scale(scale)
 *
 * |param inputDType[Input Data Type]
 * |widget DTypeChooser(uint8=1,int8=1,int16=1,int32=1,float32=1,complex_float32=1)
 * |default "int32"
 * |preview disable
 *
 * |param outputDType[Output Data Type]
 * |widget DTypeChooser(uint8=1,int8=1,int16=1,int32=1,float32=1,complex_float32=1)
 * |default "float32"
 * |preview disable
 *
 * |param scale[Scale] How much to scale up or down input values to fit in the output type.
 * |widget DoubleSpinBox(minimum=0.0, step=0.000001, decimals=6)
 * |default 1.0
 * |preview disable
 *
 * |param vlen[Vec Length]
 * |widget SpinBox(minimum=1)
 * |default 1
 * |preview disable
 **********************************************************************/
class GrConvert: public Pothos::Topology
{
public:
    static Pothos::Topology* make(const std::string& inputType, const std::string& outputType, size_t vlen)
    {
        return new GrConvert(inputType, outputType, vlen);
    }
    
    GrConvert(const std::string& inputType, const std::string& outputType, size_t vlen):
        Pothos::Topology()
    {
        const auto& blockInfo = getBlockInfo(inputType, outputType);
        d_hasScale = blockInfo.hasScale;

        d_block = blockInfo.hasScale ? Pothos::BlockRegistry::make(blockInfo.registryPath, vlen, 1.0f /*scale*/)
                                     : Pothos::BlockRegistry::make(blockInfo.registryPath, vlen);
        this->connect(this, 0, d_block, 0);
        this->connect(d_block, 0, this, 0);

        if(d_hasScale)
        {
            this->connect(this, "probe_scale", d_block, "probe_scale");
            this->connect(d_block, "scale_triggered", this, "scale_triggered");
        }

        this->registerCall(this, POTHOS_FCN_TUPLE(GrConvert, scale));
        this->registerCall(this, POTHOS_FCN_TUPLE(GrConvert, set_scale));
    }

    float scale() const
    {
        if(d_hasScale) return d_block.call<float>("scale");
        else return 0.0f;
    }

    void set_scale(float scale)
    {
        if(d_hasScale) d_block.call("set_scale", scale);
    }

private:
    Pothos::Proxy d_block;
    bool d_hasScale;
};

static Pothos::BlockRegistry registerGrConvert(
    "/gr/blocks/convert",
    Pothos::Callable(&GrConvert::make));

//
// Quick functionality test
//

POTHOS_TEST_BLOCK("/gnuradio/tests", test_convert)
{
    // Make sure this topology can be instantiated for all valid type conversions.
    const std::vector<std::pair<std::string, std::string>> validTypePairs =
    {
        {"int8", "float32"},
        {"int8", "int16"},

        {"int16", "int8"},
        {"int16", "float32"},

        {"int32", "float32"},

        {"uint8", "float32"},

        {"float32", "int8"},
        {"float32", "int16"},
        {"float32", "int32"},
        {"float32", "complex_float32"},

        {"complex_float32", "float32"},
    };
    for(const auto& typePair: validTypePairs)
    {
        constexpr size_t vlen = 1;

        std::cout << "Testing " << typePair.first << " -> " << typePair.second << "..." << std::endl;

        auto constantSource = Pothos::BlockRegistry::make(
                                  "/blocks/constant_source",
                                  typePair.first);
        auto collectorSink = Pothos::BlockRegistry::make(
                                 "/blocks/collector_sink",
                                 typePair.second);
        
        auto block = Pothos::BlockRegistry::make(
                         "/gr/blocks/convert",
                         typePair.first,
                         typePair.second,
                         vlen);

        // Make sure the types in and out of the custom topology
        // are as expected. Since the topology itself doesn't have
        // typed ports, we need to make a topology with blocks that
        // pass in and accept the expected types. If the types mismatch,
        // the collector sink will end up with zero elements.
        {
            Pothos::Topology topology;

            topology.connect(constantSource, 0, block, 0);
            topology.connect(block, 0, collectorSink, 0);

            topology.commit();
            Poco::Thread::sleep(100);
        }
        POTHOS_TEST_TRUE(collectorSink.call("getBuffer").call<size_t>("elements") > 0);
    }
}
