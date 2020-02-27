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

#include <Poco/Format.h>

#include <cmath>

//
// Ported from: gr-analog/python/nbfm_rx.py
//

class nbfm_rx: public Pothos::Topology
{
public:
    static Pothos::Topology* make(
        int audio_rate,
        int quad_rate,
        float tau,
        float max_dev)
    {
        return new nbfm_rx(audio_rate, quad_rate, tau, max_dev);
    }

    nbfm_rx(
        int audio_rate,
        int quad_rate,
        float tau,
        float max_dev
    ):
        Pothos::Topology(),
        d_fm_deemph(Pothos::BlockRegistry::make(
                        "/gr/analog/fm_deemph",
                        quad_rate,
                        tau)),
        d_optfir_designer(Pothos::BlockRegistry::make("/gr/filter/optimal_fir_designer")),
        d_audio_rate(audio_rate),
        d_quad_rate(quad_rate),
        d_max_dev(max_dev)
    {
        if((d_quad_rate % d_audio_rate) != 0)
        {
            throw Pothos::InvalidArgumentException(
                      "quad_rate must be an integer multiple of audio_rate",
                      Poco::format(
                          "quad_rate=%d, audio_rate=%d",
                          d_quad_rate,
                          d_audio_rate));
        }

        // FM demodulator
        d_quadrature_demod_cf = Pothos::BlockRegistry::make(
                                    "/gr/analog/quadrature_demod_cf",
                                    0.0f);
        this->set_max_deviation(max_dev);


        // The original block had this as a standard firdes with
        // a center frequency of 2700 Hz and transition band width
        // of 500 Hz.
        d_optfir_designer.call("set_low_freq", 2450);
        d_optfir_designer.call("set_high_freq", 2950);
        d_optfir_designer.call("set_sample_rate", quad_rate);

        const auto audio_decim = d_quad_rate / d_audio_rate;
        d_fir_filter_fff = Pothos::BlockRegistry::make(
                               "/gr/filter/fir_filter",
                               "fir_filter_fff",
                               audio_decim,
                               std::vector<float>{0.0});

        // Register calls
        this->registerCall(this, POTHOS_FCN_TUPLE(nbfm_rx, audio_rate));
        this->registerCall(this, POTHOS_FCN_TUPLE(nbfm_rx, quad_rate));
        this->registerCall(this, POTHOS_FCN_TUPLE(nbfm_rx, tau));
        this->registerCall(this, POTHOS_FCN_TUPLE(nbfm_rx, set_tau));
        this->registerCall(this, POTHOS_FCN_TUPLE(nbfm_rx, max_deviation));
        this->registerCall(this, POTHOS_FCN_TUPLE(nbfm_rx, set_max_deviation));

        // Set up internal flowgraph
        this->connect(
            this, 0,
            d_quadrature_demod_cf, 0);
        this->connect(
            d_quadrature_demod_cf, 0,
            d_fm_deemph, 0);
        this->connect(
            d_fm_deemph, 0,
            d_fir_filter_fff, 0);
        this->connect(
            d_optfir_designer, "taps_changed",
            d_fir_filter_fff, "set_taps");
        this->connect(
            d_fir_filter_fff, 0,
            this, 0);
    }

    int audio_rate() const
    {
        return d_audio_rate;
    }

    int quad_rate() const
    {
        return d_quad_rate;
    }

    float tau() const
    {
        return d_fm_deemph.call<float>("tau");
    }

    void set_tau(float tau)
    {
        d_fm_deemph.call("set_tau", tau);
    }

    float max_deviation() const
    {
        return d_max_dev;
    }

    void set_max_deviation(float max_dev)
    {
        d_max_dev = max_dev;
        const auto k = static_cast<float>(d_quad_rate) / (2.0f * M_PI * d_max_dev);
        d_quadrature_demod_cf.call("set_gain", k);
    }

private:
    Pothos::Proxy d_quadrature_demod_cf;
    Pothos::Proxy d_fm_deemph;
    Pothos::Proxy d_optfir_designer;
    Pothos::Proxy d_fir_filter_fff;

    int d_audio_rate;
    int d_quad_rate;
    float d_max_dev;
};

/***********************************************************************
 * |PothosDoc Narrowband FM Receiver
 *
 * Takes a single complex baseband input stream and produces a single
 * float output stream of audio sample in the range [-1, +1].
 *
 * |category /GNURadio/Modulators
 * |keywords frequency modulation fm receiver
 *
 * |param audio_rate[Sample Rate] Sample rate of incoming audio stream.
 * |widget SpinBox(minimum=16000)
 * |default 16000
 * |preview enable
 * |units Hz
 *
 * |param quad_rate[Quadrature Rate] Sample rate of output stream. Must be a multiple of the audio rate.
 * |widget SpinBox(minimum=16000)
 * |default 16000
 * |preview enable
 * |units Hz
 *
 * |param tau[Tau] Deemphasis time constant.
 * |widget DoubleSpinBox(minimum=0,step=1e-6,decimals=9)
 * |default 75e-6
 * |units sec
 * |preview enable
 *
 * |param max_dev[Maximum Deviation]
 * |widget DoubleSpinBox(minimum=0)
 * |default 5e3
 * |units Hz
 * |preview enable
 *
 * |factory /gr/analog/nbfm_rx(audio_rate, quad_rate, tau, max_dev)
 * |setter set_tau(tau)
 * |setter set_max_deviation(max_dev)
 **********************************************************************/
static Pothos::BlockRegistry registerNbfmRx(
    "/gr/analog/nbfm_rx",
    Pothos::Callable(&nbfm_rx::make));
