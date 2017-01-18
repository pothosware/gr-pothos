/*
 * Copyright 2017 Free Software Foundation, Inc.
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

#include "pothos_support.h"
#include <Pothos/Testing.hpp>
#include <Pothos/Object/Containers.hpp>
#include <Pothos/Framework/Packet.hpp>
#include <iostream>

template <typename T>
T test_loopback_pmt_helper(const T &inVal, const bool doEquals = true)
{
    POTHOS_TEST_CHECKPOINT();
    Pothos::Object inObj(inVal);
    std::cout << "Testing with " << inObj.toString() << " of type " << inObj.getTypeString() << std::endl;
    POTHOS_TEST_CHECKPOINT();
    auto inPmt = obj_to_pmt(inObj);
    POTHOS_TEST_CHECKPOINT();
    auto outObj(pmt_to_obj(inPmt));
    POTHOS_TEST_CHECKPOINT();
    auto outVal = outObj.convert<T>();
    if (doEquals)
    {
        POTHOS_TEST_EQUAL(inVal, outVal);
    }
    return outVal;
}

POTHOS_TEST_BLOCK("/gnuradio/block/tests", test_pmt_helper)
{
    //boolean
    test_loopback_pmt_helper(true);
    test_loopback_pmt_helper(false);

    //numbers
    test_loopback_pmt_helper(int(0));
    test_loopback_pmt_helper(int(42));
    test_loopback_pmt_helper(((long long)(1)) << 48);

    //strings
    test_loopback_pmt_helper(std::string(""));
    test_loopback_pmt_helper(std::string("hello"));

    //the empty object vector type
    test_loopback_pmt_helper(Pothos::ObjectVector());

    //and now with some values and a manual equality check
    {
        Pothos::ObjectVector objVec;
        objVec.emplace_back("abcdefg");
        objVec.emplace_back(1234567);
        auto outVec = test_loopback_pmt_helper(objVec, false);
        for (size_t i = 0; i < outVec.size(); i++)
        {
            POTHOS_TEST_TRUE(objVec.at(i).equals(outVec.at(i)));
        }
    }

    //the empty object map type
    test_loopback_pmt_helper(Pothos::ObjectMap());

    //and now with some values and a manual equality check
    {
        Pothos::ObjectMap objMap;
        objMap[Pothos::Object("key1")] = Pothos::Object(123);
        objMap[Pothos::Object("key2")] = Pothos::Object(456);
        auto outMap = test_loopback_pmt_helper(objMap, false);
        POTHOS_TEST_EQUAL(objMap.size(), outMap.size());
        for (const auto &pair : objMap)
        {
            POTHOS_TEST_TRUE(pair.second.equals(outMap.at(pair.first)));
        }
    }
}

POTHOS_TEST_BLOCK("/gnuradio/block/tests", test_pmt_packet)
{
    Pothos::Packet inPkt;
    inPkt.metadata["foo"] = Pothos::Object("bar");
    inPkt.payload = Pothos::BufferChunk(typeid(unsigned char), 100);
    for (size_t i = 0; i < 100; i++)
    {
        inPkt.payload.as<unsigned char *>()[i] = i;
    }

    POTHOS_TEST_CHECKPOINT();
    auto p = obj_to_pmt(Pothos::Object(inPkt));

    POTHOS_TEST_CHECKPOINT();
    auto outPkt = pmt_to_obj(p).convert<Pothos::Packet>();

    POTHOS_TEST_EQUAL(inPkt.metadata.size(), outPkt.metadata.size());
    for (const auto &pair : inPkt.metadata)
    {
        POTHOS_TEST_TRUE(pair.second.equals(outPkt.metadata.at(pair.first)));
    }

    POTHOS_TEST_EQUAL(inPkt.payload.elements(), outPkt.payload.elements());
    POTHOS_TEST_EQUAL(inPkt.payload.dtype.size(), outPkt.payload.dtype.size());
    for (size_t i = 0; i < 100; i++)
    {
        POTHOS_TEST_EQUAL(outPkt.payload.as<unsigned char *>()[i], i);
    }
}
