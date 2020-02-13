/*
 * Copyright 2017,2020 Free Software Foundation, Inc.
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
    auto expected = feeder.call("feedTestPlan", testPlan.dump());
    topology.commit();
    POTHOS_TEST_TRUE(topology.waitInactive());
    collector.call("verifyTestPlan", expected);
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
    auto expected = feeder.call("feedTestPlan", testPlan.dump());
    topology.commit();
    POTHOS_TEST_TRUE(topology.waitInactive());
    collector.call("verifyTestPlan", expected);
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_getter_probes)
{
    constexpr float lo = 0.1;
    constexpr float hi = 0.2;
    constexpr float initialState = 0;

    auto triggeredSignal = Pothos::BlockRegistry::make("/blocks/triggered_signal");
    triggeredSignal.call("setActivateTrigger", true);

    auto loSlotToMessage = Pothos::BlockRegistry::make("/blocks/slot_to_message", "lo");
    auto hiSlotToMessage = Pothos::BlockRegistry::make("/blocks/slot_to_message", "hi");
    auto lastStateSlotToMessage = Pothos::BlockRegistry::make("/blocks/slot_to_message", "lastState");

    auto loCollectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", "float32");
    auto hiCollectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", "float32");
    auto lastStateCollectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", "float32");
    
    auto thresholdFF = Pothos::BlockRegistry::make("/gr/blocks/threshold_ff", lo, hi, initialState);

    // Execute the topology.
    {
        Pothos::Topology topology;

        topology.connect(triggeredSignal, "triggered", thresholdFF, "probe_lo");
        topology.connect(thresholdFF, "lo_triggered", loSlotToMessage, "lo");
        topology.connect(loSlotToMessage, 0, loCollectorSink, 0);
    
        topology.connect(triggeredSignal, "triggered", thresholdFF, "probe_hi");
        topology.connect(thresholdFF, "hi_triggered", hiSlotToMessage, "hi");
        topology.connect(hiSlotToMessage, 0, hiCollectorSink, 0);

        topology.connect(triggeredSignal, "triggered", thresholdFF, "probe_last_state");
        topology.connect(thresholdFF, "last_state_triggered", lastStateSlotToMessage, "lastState");
        topology.connect(lastStateSlotToMessage, 0, lastStateCollectorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    auto loCollectorMessages = loCollectorSink.call<Pothos::ObjectVector>("getMessages");
    POTHOS_TEST_EQUAL(1, loCollectorMessages.size());
    POTHOS_TEST_EQUAL(lo, loCollectorMessages[0].convert<float>())

    auto hiCollectorMessages = hiCollectorSink.call<Pothos::ObjectVector>("getMessages");
    POTHOS_TEST_EQUAL(1, hiCollectorMessages.size());
    POTHOS_TEST_EQUAL(hi, hiCollectorMessages[0].convert<float>())

    auto lastStateCollectorMessages = lastStateCollectorSink.call<Pothos::ObjectVector>("getMessages");
    POTHOS_TEST_EQUAL(1, lastStateCollectorMessages.size());
    POTHOS_TEST_EQUAL(initialState, lastStateCollectorMessages[0].convert<float>())
}
