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

#include "optfir.h"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Plugin.hpp>

#include <Poco/Format.h>
#include <Poco/Logger.h>

#include <gnuradio/gr_complex.h>

#include <cmath>
#include <string>
#include <vector>

//
// Ported from: gr-filter/python/pfb.py
//

class channelizer_ccf: public Pothos::Topology
{
public:
    static Pothos::Topology* make(
        size_t nchans,
        const std::vector<float>& taps,
        double oversample_rate,
        double attenuation)
    {
        return new channelizer_ccf(nchans, taps, oversample_rate, attenuation);
    }
        
    channelizer_ccf(size_t nchans,
                    const std::vector<float>& taps,
                    double oversample_rate,
                    double attenuation):
        Pothos::Topology(),
        d_s2ss(Pothos::BlockRegistry::make("/gr/blocks/stream_to_streams", Pothos::DType(typeid(gr_complex)), nchans))
    {
        std::vector<float> tmp_taps;

        if(taps.empty())
        {
            bool made = false;
            while(!made)
            {
                constexpr double bw = 0.4;
                constexpr double tb = 0.2;
                constexpr double start_ripple = 0.1;
                double ripple = start_ripple;

                try
                {
                    tmp_taps = Pothos::Object(optfir_low_pass(1, nchans, bw, (bw+tb), ripple, attenuation, 0)).convert<std::vector<float>>();
                    made = true;
                }
                catch(const std::exception&)
                {
                    ripple += 0.01;
                    if(ripple >= 1.0)
                    {
                        throw Pothos::RuntimeException("optfir could not generate an appropriate filter.");
                    }
                    else
                    {
                        static auto& logger = Poco::Logger::get("/gr/filter/channelizer_ccf");
                        poco_warning_f2(
                            logger,
                            "Attempted ripple %f (ideal %f). If this is a problem, adjust the attenuation or provide your own filter taps.",
                            ripple,
                            start_ripple);
                    }
                }
            }
        }
        else
        {
            tmp_taps = taps;
        }

        if(tmp_taps.empty())
        {
            throw Pothos::AssertionViolationException("Empty taps");
        }

        d_pfb = Pothos::BlockRegistry::make(
                    "/gr/filter/pfb_channelizer_ccf",
                    nchans,
                    tmp_taps,
                    oversample_rate);

        this->connect(this, 0, d_s2ss, 0);
        for(size_t chan = 0; chan < nchans; ++chan)
        {
            this->connect(d_s2ss, chan, d_pfb, chan);
            this->connect(d_pfb, chan, this, chan);
        }

        // Channel map
        this->registerCall(this, POTHOS_FCN_TUPLE(channelizer_ccf, channel_map));
        this->registerCall(this, POTHOS_FCN_TUPLE(channelizer_ccf, set_channel_map));
        this->connect(this, "set_channel_map", d_pfb, "set_channel_map");
        this->connect(this, "probe_channel_map", d_pfb, "probe_channel_map");
        this->connect(d_pfb, "channel_map_triggered", this, "channel_map_triggered");

        // Taps
        this->registerCall(this, POTHOS_FCN_TUPLE(channelizer_ccf, taps));
        this->registerCall(this, POTHOS_FCN_TUPLE(channelizer_ccf, set_taps));
        this->connect(this, "set_taps", d_pfb, "set_taps");
        this->connect(this, "probe_taps", d_pfb, "probe_taps");
        this->connect(d_pfb, "taps_triggered", this, "taps_triggered");

        // Other
        this->registerCall(this, POTHOS_FCN_TUPLE(channelizer_ccf, declare_sample_delay));
        this->connect(this, "declare_sample_delay", d_pfb, "declare_sample_delay");
    }

    std::vector<int> channel_map() const
    {
        return d_pfb.call("channel_map");
    }

    std::vector<float> taps() const
    {
        return d_pfb.call("taps");
    }

    // These need to be here to connect the topology to the internal blocks.
    void set_channel_map(const std::vector<int>&) {}
    void set_taps(const std::vector<float>&) {}
    void declare_sample_delay(unsigned) {}

private:
    Pothos::Proxy d_s2ss;
    Pothos::Proxy d_pfb;
};

static Pothos::BlockRegistry registerChannelizerCCF(
    "/gr/filter/channelizer_ccf",
    Pothos::Callable(&channelizer_ccf::make));
