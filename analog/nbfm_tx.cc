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
// Ported from: gr-analog/python/nbfm_tx.py
//

class nbfm_tx: public Pothos::Topology
{
public:
    static Pothos::Topology* make(
        int audio_rate,
        int quad_rate,
        float tau,
        float max_dev,
        float high_freq)
    {
        return new nbfm_tx(audio_rate, quad_rate, tau, max_dev, high_freq);
    }

    nbfm_tx(
        int audio_rate,
        int quad_rate,
        float tau,
        float max_dev,
        float high_freq
    ):
        Pothos::Topology(),
        d_fm_preemph(Pothos::BlockRegistry::make(
                         "/gr/analog/fm_preemph",
                         quad_rate,
                         tau,
                         high_freq)),
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

        if(d_quad_rate != d_audio_rate)
        {
            const auto interp_factor = d_quad_rate / d_audio_rate;
            d_optfir_designer.call("set_gain", interp_factor);
            d_optfir_designer.call("set_sample_rate", d_quad_rate);
            d_optfir_designer.call("set_low_freq", 4500);
            d_optfir_designer.call("set_high_freq", 7000);
            d_optfir_designer.call("set_passband_ripple", 0.1);
            d_optfir_designer.call("set_stopband_atten", 40);

            d_interp_fir_filter_fff = Pothos::BlockRegistry::make(
                                          "/gr/filter/interp_fir_filter",
                                          "interp_fir_filter_fff",
                                          interp_factor,
                                          std::vector<float>{0.0});

            this->connect(
                this, 0,
                d_interp_fir_filter_fff, 0);
            this->connect(
                d_optfir_designer, "taps_changed",
                d_interp_fir_filter_fff, "set_taps");
            this->connect(
                d_interp_fir_filter_fff, 0,
                d_fm_preemph, 0);
        }
        else
        {
            this->connect(
                this, 0,
                d_fm_preemph, 0);
        }

        d_frequency_modulator_fc = Pothos::BlockRegistry::make(
                                        "/gr/analog/frequency_modulator_fc",
                                        0.0f);
        this->set_max_deviation(max_dev);

        // Register calls
        this->registerCall(this, POTHOS_FCN_TUPLE(nbfm_tx, audio_rate));
        this->registerCall(this, POTHOS_FCN_TUPLE(nbfm_tx, quad_rate));
        this->registerCall(this, POTHOS_FCN_TUPLE(nbfm_tx, tau));
        this->registerCall(this, POTHOS_FCN_TUPLE(nbfm_tx, set_tau));
        this->registerCall(this, POTHOS_FCN_TUPLE(nbfm_tx, high_freq));
        this->registerCall(this, POTHOS_FCN_TUPLE(nbfm_tx, set_high_freq));
        this->registerCall(this, POTHOS_FCN_TUPLE(nbfm_tx, max_deviation));
        this->registerCall(this, POTHOS_FCN_TUPLE(nbfm_tx, set_max_deviation));

        // Set up remaining internal flowgraph
        this->connect(
            d_fm_preemph, 0,
            d_frequency_modulator_fc, 0);
        this->connect(
            d_frequency_modulator_fc, 0,
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
        return d_fm_preemph.call<float>("tau");
    }

    void set_tau(float tau)
    {
        d_fm_preemph.call("set_tau", tau);
    }

    float high_freq() const
    {
        return d_fm_preemph.call<float>("high_freq");
    }

    void set_high_freq(float high_freq)
    {
        d_fm_preemph.call("set_high_freq", high_freq);
    }

    float max_deviation() const
    {
        return d_max_dev;
    }

    void set_max_deviation(float max_dev)
    {
        d_max_dev = max_dev;
        const auto k = (2.0f * M_PI * d_max_dev) / static_cast<float>(d_quad_rate);
        d_frequency_modulator_fc.call("set_sensitivity", k);
    }

private:
    Pothos::Proxy d_fm_preemph;
    Pothos::Proxy d_optfir_designer;
    Pothos::Proxy d_interp_fir_filter_fff;
    Pothos::Proxy d_frequency_modulator_fc;

    int d_audio_rate;
    int d_quad_rate;
    float d_max_dev;
};

/***********************************************************************
 * |PothosDoc Narrowband FM Transmitter
 *
 * Takes a single float input stream of audio samples in the range [-1,+1]
 * and produces a single FM modulated complex baseband output.
 *
 * |category /GNURadio/Modulators
 * |keywords frequency modulation fm transmitter
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
 * |param tau[Tau] Preemphasis time constant.
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
 * |param high_freq[High Freq] The frequency at which to flatten preemphasis.
 * |widget DoubleSpinBox(minimum=0)
 * |default 0.0
 * |units Hz
 * |preview enable
 *
 * |factory /gr/analog/nbfm_tx(audio_rate, quad_rate, tau, max_dev, high_freq)
 * |setter set_tau(tau)
 * |setter set_max_deviation(max_dev)
 **********************************************************************/
static Pothos::BlockRegistry registerNbfmRx(
    "/gr/analog/nbfm_tx",
    Pothos::Callable(&nbfm_tx::make));
