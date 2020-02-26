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

#include <cmath>
#include <string>
#include <vector>

//
// Ported from: gr-filter/python/freq_xlating_fft_filter.py
//

#define REGISTER_GETTER(field_name) \
    this->registerCall(this, POTHOS_FCN_TUPLE(freq_xlating_fft_filter_ccc, field_name));

#define REGISTER_GETTER_SETTER(field_name) \
    REGISTER_GETTER(field_name) \
    this->registerCall(this, POTHOS_FCN_TUPLE(freq_xlating_fft_filter_ccc, set_ ## field_name));

#define IMPL_GETTER(field_name, type) \
    type field_name() const \
    { \
        return d_ ## field_name; \
    } \

#define IMPL_GETTER_SETTER(field_name, type) \
    IMPL_GETTER(field_name, type) \
 \
    void set_## field_name(const type& field_name) \
    { \
        d_ ## field_name = field_name; \
 \
        this->recalculate(); \
    }

class freq_xlating_fft_filter_ccc: public Pothos::Topology
{
public:
    static Pothos::Topology* make(
        int decimation,
        const std::vector<gr_complex>& taps,
        double center_freq,
        double samp_rate)
    {
        return new freq_xlating_fft_filter_ccc(
                       decimation,
                       taps,
                       center_freq,
                       samp_rate);
    }

    freq_xlating_fft_filter_ccc(
        int decimation,
        const std::vector<gr_complex>& taps,
        double center_freq,
        double samp_rate
    ):
        Pothos::Topology(),
        d_decimation(decimation),
        d_taps(taps),
        d_center_freq(center_freq),
        d_samp_rate(samp_rate),
        d_fft_filter_ccc(Pothos::BlockRegistry::make(
                             "/gr/filter/fft_filter_ccc",
                             d_decimation,
                             d_taps,
                             1 /*nthreads*/)),
        d_rotator_cc(Pothos::BlockRegistry::make(
                         "/gr/blocks/rotator_cc",
                         0.0 /*phase_inc*/))
    {
        this->connect(
            this, 0,
            d_fft_filter_ccc, 0);
        this->connect(
            d_fft_filter_ccc, 0,
            d_rotator_cc, 0);
        this->connect(
            d_rotator_cc, 0,
            this, 0);

        this->connect(
            this, "nthreads",
            d_fft_filter_ccc, "nthreads");
        this->connect(
            this, "set_nthreads",
            d_fft_filter_ccc, "set_nthreads");
        this->connect(
            this, "probe_nthreads",
            d_fft_filter_ccc, "probe_nthreads");
        this->connect(
            d_fft_filter_ccc, "nthreads_triggered",
            this, "nthreads_triggered");

        // TODO: declare_sample_delay, need to expose sync_block in PothosGrBlock

        REGISTER_GETTER(decimation)
        REGISTER_GETTER_SETTER(taps)
        REGISTER_GETTER_SETTER(center_freq)
        REGISTER_GETTER_SETTER(samp_rate)
    }

    IMPL_GETTER(decimation, int)
    IMPL_GETTER_SETTER(taps, std::vector<gr_complex>)
    IMPL_GETTER_SETTER(center_freq, double)
    IMPL_GETTER_SETTER(samp_rate, double)

private:
    int d_decimation;
    std::vector<gr_complex> d_taps;
    double d_center_freq;
    double d_samp_rate;

    Pothos::Proxy d_fft_filter_ccc;
    Pothos::Proxy d_rotator_cc;

    void recalculate()
    {
        const auto phase_inc = (2.0 * M_PI * d_center_freq) / d_samp_rate;

        auto rtaps = d_taps;
        for(size_t i = 0; i < rtaps.size(); ++i)
        {
            rtaps[i] *= std::exp(i * static_cast<float>(phase_inc) * gr_complex(0,1));
        }

        d_fft_filter_ccc.call("set_taps", rtaps);
        d_rotator_cc.call("set_phase_inc", -d_decimation * phase_inc);
    }
};

static Pothos::BlockRegistry registerGrFilterFreqXlatingFFTFilterCCC(
    "/gr/filter/freq_xlating_fft_filter_ccc",
    Pothos::Callable(&freq_xlating_fft_filter_ccc::make));
