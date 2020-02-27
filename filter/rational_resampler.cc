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
#include <Pothos/Plugin.hpp>

#include <gnuradio/gr_complex.h>
#include <gnuradio/filter/firdes.h>

#include <Poco/Logger.h>
#include <Poco/NumberFormatter.h>

#include <cmath>
#include <string>
#include <vector>

//
// Ported from: gr-filter/python/rational_resampler.py
//

static std::vector<float> design_filter(
    unsigned interpolation,
    unsigned decimation,
    float fractional_bw)
{
    if((fractional_bw < 0.0f) || (fractional_bw > 0.5f))
    {
        throw Pothos::InvalidArgumentException(
                  "fractional_bw must be in the range [0.0,0.5]",
                  std::to_string(fractional_bw));
    }

    float transition_width = 0.0f;
    float mid_transition_band = 0.0f;

    constexpr auto beta = 7.0f;
    constexpr auto halfband = 0.5f;
    const auto rate = interpolation / decimation;
    if(rate >= 1.0f)
    {
        transition_width = halfband - fractional_bw;
        mid_transition_band = halfband - transition_width/2.0f;
    }
    else
    {
        transition_width = rate * (halfband - fractional_bw);
        mid_transition_band = rate*halfband - transition_width/2.0f;
    }

    return gr::filter::firdes::low_pass(
               interpolation,
               interpolation,
               mid_transition_band,
               transition_width,
               gr::filter::firdes::WIN_KAISER,
               beta);
}

static int gcd(int a, int b)
{
    if(b == 0) return a;
    return gcd(b, a % b);
}

class rational_resampler: public Pothos::Topology
{
public:
    static Pothos::Topology* make(
        const std::string& type,
        unsigned interpolation,
        unsigned decimation,
        const std::vector<float>& taps,
        float fractional_bw)
    {
        return new rational_resampler(
                       type,
                       interpolation,
                       decimation,
                       taps,
                       fractional_bw);
    }
    
    rational_resampler(
        const std::string& type,
        unsigned interpolation,
        unsigned decimation,
        const std::vector<float>& taps,
        float fractional_bw)
    {
        const auto d = gcd(interpolation, decimation);
        if(!taps.empty() && (d > 1))
        {
            static auto& logger = Poco::Logger::get(this->getName());
            poco_warning_f3(
                logger,
                "Rational resampler has user-provided taps but interpolation (%s) and decimation "
                "(%s) have a GCD of %s, which increases the complexity of the filter bank. Consider "
                "reducing these values by the GCD.",
                Poco::NumberFormatter::format(interpolation),
                Poco::NumberFormatter::format(decimation),
                Poco::NumberFormatter::format(d));
        }

        auto _taps = taps;

        if(taps.empty())
        {
            interpolation /= d;
            decimation /= d;
            _taps = design_filter(interpolation, decimation, fractional_bw);
        }

        d_rational_resampler_base = Pothos::BlockRegistry::make(
                                        "/gr/filter/rational_resampler_base",
                                        type,
                                        interpolation,
                                        decimation,
                                        _taps);

        // This block is a thin wrapper around rational_resampler_base, so just
        // connect everything to that.
        this->connect(this, 0, d_rational_resampler_base, 0);
        this->connect(d_rational_resampler_base, 0, this, 0);

        this->connect(
            this, "interpolation",
            d_rational_resampler_base, "interpolation");
        this->connect(
            this, "probe_interpolation",
            d_rational_resampler_base, "probe_interpolation");
        this->connect(
            d_rational_resampler_base, "interpolation_triggered",
            this, "interpolation_triggered");

        this->connect(
            this, "decimation",
            d_rational_resampler_base, "decimation");
        this->connect(
            this, "probe_decimation",
            d_rational_resampler_base, "probe_decimation");
        this->connect(
            d_rational_resampler_base, "decimation_triggered",
            this, "decimation_triggered");

        this->connect(
            this, "taps",
            d_rational_resampler_base, "taps");
        this->connect(
            this, "set_taps",
            d_rational_resampler_base, "set_taps");
        this->connect(
            this, "probe_taps",
            d_rational_resampler_base, "probe_taps");
        this->connect(
            d_rational_resampler_base, "taps_triggered",
            this, "taps_triggered");
    }

    Pothos::Object opaqueCallMethod(
        const std::string& name,
        const Pothos::Object *inputArgs,
        const size_t numArgs) const override
    {
        // Calls that go to the topology
        try
        {
            return Pothos::Topology::opaqueCallMethod(name, inputArgs, numArgs);
        }
        catch (const Pothos::BlockCallNotFound &){}

        // Forward everything else to the internal block.
        return d_rational_resampler_base.call<Pothos::Block*>("getPointer")->opaqueCallMethod(name, inputArgs, numArgs);
    }

private:
    Pothos::Proxy d_rational_resampler_base;
};

static Pothos::BlockRegistry registerRationalResampler(
    "/gr/filter/rational_resampler",
    Pothos::Callable(&rational_resampler::make));
