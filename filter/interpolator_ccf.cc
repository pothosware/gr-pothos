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

class interpolator_ccf: public Pothos::Topology
{
public:
    static Pothos::Topology* make(
        double interp,
        const std::vector<float>& taps,
        double attenuation)
    {
        return new interpolator_ccf(interp, taps, attenuation);
    }

    interpolator_ccf(double interp,
                     const std::vector<float>& taps,
                     double attenuation):
        Pothos::Topology()
    {
        std::vector<float> tmp_taps;

        if(taps.empty())
        {
            bool made = false;
            while(!made)
            {
                constexpr double bw = 0.4;
                constexpr double tb = 0.2;
                constexpr double start_ripple = 0.99;
                double ripple = start_ripple;

                try
                {
                    tmp_taps = Pothos::Object(optfir_low_pass(interp, interp, bw, (bw+tb), ripple, attenuation, 0)).convert<std::vector<float>>();
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
                        static auto& logger = Poco::Logger::get("/gr/filter/interpolator_ccf");
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
                    "/gr/filter/pfb_interpolator_ccf",
                    interp,
                    tmp_taps);

        this->connect(this, 0, d_pfb, 0);
        this->connect(d_pfb, 0, this, 0);

        // Taps
        this->registerCall(this, POTHOS_FCN_TUPLE(interpolator_ccf, taps));
        this->registerCall(this, POTHOS_FCN_TUPLE(interpolator_ccf, set_taps));
        this->connect(this, "set_taps", d_pfb, "set_taps");
        this->connect(this, "probe_taps", d_pfb, "probe_taps");
        this->connect(d_pfb, "taps_triggered", this, "taps_triggered");

        // Other
        this->registerCall(this, POTHOS_FCN_TUPLE(interpolator_ccf, declare_sample_delay));
        this->connect(this, "declare_sample_delay", d_pfb, "declare_sample_delay");
    }

    std::vector<float> taps() const
    {
        return d_pfb.call("taps");
    }

    // These need to be here to connect the topology to the internal blocks.
    void set_taps(const std::vector<float>&) {}
    void declare_sample_delay(unsigned) {}

private:
    Pothos::Proxy d_pfb;
};

static Pothos::BlockRegistry registerinterpolatorCCF(
    "/gr/filter/interpolator_ccf",
    Pothos::Callable(&interpolator_ccf::make));
