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

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <json.hpp>

using json = nlohmann::json;

POTHOS_TEST_BLOCK("/gnuradio/tests", test_copy_stream)
{
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "float");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "float");
    auto copy = Pothos::BlockRegistry::make("/gr/blocks/copy", "float");

    //setup the topology
    Pothos::Topology topology;
    topology.connect(feeder, 0, copy, 0);
    topology.connect(copy, 0, collector, 0);

    //create a test plan for a stream with random labels
    json testPlan;
    testPlan["enableBuffers"] = true;
    testPlan["enableLabels"] = true;
    auto expected = feeder.callProxy("feedTestPlan", testPlan.dump());
    topology.commit();
    POTHOS_TEST_TRUE(topology.waitInactive());
    collector.callVoid("verifyTestPlan", expected);
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_copy_packets)
{
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "uint8");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "uint8");
    auto copy = Pothos::BlockRegistry::make("/gr/blocks/pdu_set", "key0", "value0");

    //setup the topology
    Pothos::Topology topology;
    topology.connect(feeder, 0, copy, "pdus");
    topology.connect(copy, "pdus", collector, 0);

    //create a test plan for packets
    json testPlan;
    testPlan["enablePackets"] = true;
    testPlan["enableLabels"] = true;
    auto expected = feeder.callProxy("feedTestPlan", testPlan.dump());
    topology.commit();
    POTHOS_TEST_TRUE(topology.waitInactive());
    collector.callVoid("verifyTestPlan", expected);
}
