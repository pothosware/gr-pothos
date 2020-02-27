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

#include <cmath>

//
// Ported from: gr-analog/python/fm_demod.py
//

class fm_demod_cf: public Pothos::Topology
{
public:
    static Pothos::Topology* make(
        int channel_rate,
        int audio_decim,
        float deviation,
        float audio_pass,
        float audio_stop,
        float gain,
        float tau)
    {
        return new fm_demod_cf(
                       channel_rate,
                       audio_decim,
                       deviation,
                       audio_pass,
                       audio_stop,
                       gain,
                       tau);
    }
    
    fm_demod_cf(
        int channel_rate,
        int audio_decim,
        float deviation,
        float audio_pass,
        float audio_stop,
        float gain,
        float tau
    ):
        Pothos::Topology(),
        d_channel_rate(channel_rate),
        d_audio_decim(audio_decim),
        d_deviation(deviation),
        d_audio_pass(audio_pass),
        d_audio_stop(audio_stop),
        d_gain(gain),
        d_quadrature_demod_cf(Pothos::BlockRegistry::make(
                                  "/gr/analog/quadrature_demod_cf",
                                  0.0f /*gain*/)),
        d_optfir_designer(Pothos::BlockRegistry::make("/gr/filter/optimal_fir_designer")),
        d_fir_filter_fff(Pothos::BlockRegistry::make(
                             "/gr/filter/fir_filter",
                             "fir_filter_fff",
                             d_audio_decim,
                             std::vector<float>())),
        d_fm_deemph(Pothos::BlockRegistry::make(
                        "/gr/analog/fm_deemph",
                        d_channel_rate,
                        tau))
    {
        this->connect(this, 0, d_quadrature_demod_cf, 0);

        if(tau > 0.0f)
        {
            this->connect(
                d_quadrature_demod_cf, 0,
                d_fm_deemph, 0);
            this->connect(
                d_fm_deemph, 0,
                d_fir_filter_fff, 0);
        }
        else
        {
            this->connect(
                d_quadrature_demod_cf, 0,
                d_fir_filter_fff, 0);
        }

        this->connect(
            d_optfir_designer, "taps_changed",
            d_fir_filter_fff, "set_taps");
        this->connect(d_fir_filter_fff, 0, this, 0);

        const auto k = d_channel_rate / (2.0f * M_PI * d_deviation);
        d_quadrature_demod_cf.call("set_gain", k);

        d_optfir_designer.call("set_band_type", "LOW_PASS");
        d_optfir_designer.call("set_gain", d_gain);
        d_optfir_designer.call("set_sample_rate", d_channel_rate);
        d_optfir_designer.call("set_low_freq", d_audio_pass);
        d_optfir_designer.call("set_high_freq", d_audio_stop);
        d_optfir_designer.call("set_passband_ripple", 0.1);
        d_optfir_designer.call("set_stopband_atten", 60.0);

        this->registerCall(this, POTHOS_FCN_TUPLE(fm_demod_cf, channel_rate));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_demod_cf, audio_decim));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_demod_cf, deviation));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_demod_cf, audio_pass));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_demod_cf, audio_stop));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_demod_cf, gain));
    }

    int channel_rate() const
    {
        return d_channel_rate;
    }

    int audio_decim() const
    {
        return d_audio_decim;
    }

    float deviation() const
    {
        return d_deviation;
    }

    float audio_pass() const
    {
        return d_audio_pass;
    }

    float audio_stop() const
    {
        return d_audio_stop;
    }

    float gain() const
    {
        return d_gain;
    }

    float tau() const
    {
        return d_fm_deemph.call<float>("tau");
    }

private:
    int d_channel_rate;
    int d_audio_decim;
    float d_deviation;
    float d_audio_pass;
    float d_audio_stop;
    float d_gain;

    Pothos::Proxy d_quadrature_demod_cf;
    Pothos::Proxy d_optfir_designer;
    Pothos::Proxy d_fir_filter_fff;
    Pothos::Proxy d_fm_deemph;
};

class demod_20k0f3e_cf: public fm_demod_cf
{
public:
    static Pothos::Topology* make(
        int channel_rate,
        int audio_decim)
    {
        return new demod_20k0f3e_cf(channel_rate, audio_decim);
    }

    demod_20k0f3e_cf(
        int channel_rate,
        int audio_decim
    ):
        fm_demod_cf(
            channel_rate,
            audio_decim,
            5000.0f /*deviation*/,
            3500.0f /*audio_pass*/,
            4000.0f /*audio_stop*/,
            1.0f /*gain*/,
            75e-6 /*tau*/)
    {
    }
};

class demod_200k0f3e_cf: public fm_demod_cf
{
public:
    static Pothos::Topology* make(
        int channel_rate,
        int audio_decim)
    {
        return new demod_200k0f3e_cf(channel_rate, audio_decim);
    }

    demod_200k0f3e_cf(
        int channel_rate,
        int audio_decim
    ):
        fm_demod_cf(
            channel_rate,
            audio_decim,
            75000.0f /*deviation*/,
            15000.0f /*audio_pass*/,
            16000.0f /*audio_stop*/,
            2.0f /*gain*/,
            75e-6 /*tau*/)
    {
    }
};

/***********************************************************************
 * |PothosDoc FM Demod
 *
 * Generalized FM demodulation block with deemphasis and audio
 * filtering.
 * 
 * This block demodulates a band-limited, complex down-converted FM
 * channel into the the original baseband signal, optionally applying
 * deemphasis. Low pass filtering is done on the resultant signal. It
 * produces an output float stream in the range of [-1.0, +1.0].
 *
 * |category /GNURadio/Modulators
 * |keywords frequency modulation fm
 *
 * |param channel_rate[Channel Rate] Incoming sample rate of the FM baseband.
 * |widget SpinBox(minimum=0)
 * |default 250000
 * |preview enable
 *
 * |param audio_decim[Audio Decimation] Input to output decimation rate.
 * |widget SpinBox(mimimum=1)
 * |default 1
 * |preview enable
 *
 * |param deviation[FM Deviation] Maximum FM deviation.
 * |widget DoubleSpinBox(minimum=0,step=1.0)
 * |default 5000.0
 * |preview enable
 *
 * |param audio_pass[Audio Passband Frequency] Audio low-pass filter passband frequency.
 * |widget DoubleSpinBox(minimum=1,step=1.0)
 * |default 5000.0
 * |preview enable
 *
 * |param audio_stop[Audio Stopband Frequency] Audio low-pass filter stop frequency.
 * |widget DoubleSpinBox(minimum=1,step=1.0)
 * |default 5500.0
 * |preview enable
 *
 * |param gain[Audio Gain] Gain applied to the audio output.
 * |widget DoubleSpinBox(minimum=0,step=0.01)
 * |default 1.0
 * |preview enable
 *
 * |param tau[Tau] Deemphasis time constant.
 * |widget DoubleSpinBox(minimum=0,step=1e-6,decimals=9)
 * |default 75e-6
 * |preview enable
 *
 * |factory /gr/analog/fm_demod_cf(channel_rate,audio_decim,deviation,audio_pass,audio_stop,gain,tau)
 **********************************************************************/
static Pothos::BlockRegistry registerFmDemod(
    "/gr/analog/fm_demod_cf",
    Pothos::Callable(&fm_demod_cf::make));

/***********************************************************************
 * |PothosDoc FM Demod (20 kHz)
 *
 * NBFM demodulation block, 20 KHz channels
 * 
 * This block demodulates a complex, downconverted, narrowband FM
 * channel conforming to 20K0F3E emission standards, outputting
 * floats in the range [-1.0, +1.0].
 *
 * |category /GNURadio/Modulators
 * |keywords frequency modulation fm narrow band
 *
 * |param channel_rate[Channel Rate] Incoming sample rate of the FM baseband.
 * |widget SpinBox(minimum=0)
 * |default 250000
 * |preview enable
 *
 * |param audio_decim[Audio Decimation] Input to output decimation rate.
 * |widget SpinBox(mimimum=1)
 * |default 1
 * |preview enable
 *
 * |factory /gr/analog/demod_20k0f3e_cf(channel_rate,audio_decim)
 **********************************************************************/
static Pothos::BlockRegistry registerDemod20k0f3eCF(
    "/gr/analog/demod_20k0f3e_cf",
    Pothos::Callable(&demod_20k0f3e_cf::make));

/***********************************************************************
 * |PothosDoc FM Demod (200 kHz)
 *
 * WFM demodulation block, mono.
 * 
 * This block demodulates a complex, downconverted, wideband FM
 * channel conforming to 200KF3E emission standards, outputting
 * floats in the range [-1.0, +1.0].
 *
 * |category /GNURadio/Modulators
 * |keywords frequency modulation fm wide band
 *
 * |param channel_rate[Channel Rate] Incoming sample rate of the FM baseband.
 * |widget SpinBox(minimum=0)
 * |default 250000
 * |preview enable
 *
 * |param audio_decim[Audio Decimation] Input to output decimation rate.
 * |widget SpinBox(mimimum=1)
 * |default 1
 * |preview enable
 *
 * |factory /gr/analog/demod_200k0f3e_cf(channel_rate,audio_decim)
 **********************************************************************/
static Pothos::BlockRegistry registerDemod200k0f3eCF(
    "/gr/analog/demod_200k0f3e_cf",
    Pothos::Callable(&demod_200k0f3e_cf::make));
