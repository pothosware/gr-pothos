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

//
// Ported from: gr-analog/python/am_demod.py
//

// TODO: getters, setters

class am_demod_cf: public Pothos::Topology
{
public:
    Pothos::Topology* make(
        int channel_rate,
        int audio_decim,
        float audio_pass,
        float audio_stop)
    {
        return new am_demod_cf(channel_rate, audio_decim, audio_pass, audio_stop);
    }
    
    am_demod_cf(
        int channel_rate,
        int audio_decim,
        float audio_pass,
        float audio_stop
    ):
        Pothos::Topology(),
        d_channel_rate(channel_rate),
        d_audio_decim(audio_decim),
        d_audio_pass(audio_pass),
        d_audio_stop(audio_stop)
    {
        d_complex_to_mag = Pothos::BlockRegistry::make("/gr/blocks/complex_to_mag", 1 /*vlen*/);
        d_add_const_ff = Pothos::BlockRegistry::make("/gr/blocks/add_const", "add_const_ff", -1.0f);
        d_optfir_designer = Pothos::BlockRegistry::make("/gr/filter/optimal_fir_designer");
        d_fir_filter_fff = Pothos::BlockRegistry::make("/gr/filter/fir_filter_fff", d_audio_decim, std::vector<float>());
        
        this->connect(this, 0, d_complex_to_mag, 0);
        this->connect(d_complex_to_mag, 0, d_add_const_ff, 0);
        this->connect(d_add_const_ff, 0, d_fir_filter_fff, 0);
        this->connect(
            d_optfir_designer, "taps_changed",
            d_fir_filter_fff, "set_taps");
        this->connect(d_fir_filter_fff, 0, this, 0);

        d_optfir_designer.call("set_band", "LOW_PASS");
        d_optfir_designer.call("set_sample_rate", d_channel_rate);
        d_optfir_designer.call("set_low_freq", d_audio_pass);
        d_optfir_designer.call("set_high_freq", d_audio_stop);
        d_optfir_designer.call("set_passband_ripple", 0.1);
        d_optfir_designer.call("set_stopband_atten", 60.0);
    }

private:
    int d_channel_rate;
    int d_audio_decim;
    float d_audio_pass;
    float d_audio_stop;

    Pothos::Proxy d_complex_to_mag;
    Pothos::Proxy d_add_const_ff;
    Pothos::Proxy d_optfir_designer;
    Pothos::Proxy d_fir_filter_fff;
};

class demod_10k0a3e_cf: public am_demod_cf
{
public:
    Pothos::Topology* make(
        int channel_rate,
        int audio_decim)
    {
        return new demod_10k0a3e_cf(channel_rate, audio_decim);
    }

    demod_10k0a3e_cf(
        int channel_rate,
        int audio_decim
    ):
        am_demod_cf(channel_rate, audio_decim, 5000, 5500)
    {
    }
};

/***********************************************************************
 * |PothosDoc AM Demod
 *
 * Generalized AM demodulation block with audio filtering.
 * 
 * This block demodulates a band-limited, complex down-converted AM
 * channel into the the original baseband signal, applying low pass
 * filtering to the audio output. It produces a float stream in the
 * range [-1.0, +1.0].
 *
 * |category /GNURadio/Modulators
 * |keywords frequency modulation am
 *
 * |param channel_rate[Channel Rate] Incoming sample rate of the AM baseband.
 * |widget SpinBox(minimum=0)
 * |default 250000
 * |preview enable
 *
 * |param audio_decim[Audio Decimation] Input to output decimation rate.
 * |widget SpinBox(mimimum=1)
 * |default 1
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
 * |factory /gr/analog/am_demod_cf(channel_rate,audio_decim,audio_pass,audio_stop)
 **********************************************************************/
static Pothos::BlockRegistry registerAmDemod(
    "/gr/analog/am_demod_cf",
    Pothos::Callable(&am_demod_cf::make));

/***********************************************************************
 * |PothosDoc AM Demod (10 kHz)
 *
 * Generalized AM demodulation block, 10 kHz channel.
 * This block demodulates an AM channel conformant to 10K0A3E emission
 * standards, such as broadcast band AM transmissions.
 *
 * |category /GNURadio/Modulators
 * |keywords frequency modulation am
 *
 * |param channel_rate[Channel Rate] Incoming sample rate of the AM baseband.
 * |widget SpinBox(minimum=0)
 * |default 250000
 * |preview enable
 *
 * |param audio_decim[Audio Decimation] Input to output decimation rate.
 * |widget SpinBox(mimimum=1)
 * |default 1
 * |preview enable
 *
 * |factory /gr/analog/demod_10k0a3e_cf(channel_rate,audio_decim)
 **********************************************************************/
static Pothos::BlockRegistry registerDemod10k0a3eCF(
    "/gr/analog/demod_10k0a3e_cf",
    Pothos::Callable(&demod_10k0a3e_cf::make));
