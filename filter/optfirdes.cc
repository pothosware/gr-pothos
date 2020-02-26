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

// TODO: sensible defaults

#include "optfir.h"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Plugin.hpp>

#include <algorithm>
#include <string>
#include <vector>

#define REGISTER_GETTER_SETTER(field_name) \
    this->registerCall(this, POTHOS_FCN_TUPLE(GrFilterOptimalFIRDesigner, field_name)); \
    this->registerCall(this, POTHOS_FCN_TUPLE(GrFilterOptimalFIRDesigner, set_ ## field_name)); \
    this->registerProbe(#field_name, #field_name "_triggered", "probe_" #field_name);

#define IMPL_GETTER_SETTER(field_name, type) \
    type field_name() const \
    { \
        return d_ ## field_name; \
    } \
 \
    void set_## field_name(type field_name) \
    { \
        d_ ## field_name = field_name; \
 \
        this->recalculate(); \
    }

/***********************************************************************
 * |PothosDoc Optimal FIR Designer
 * 
 * This block implements GNU Radio's Python-only routines for designing
 * optimal FIR filters. These methodologies are based on section 6.6 of
 * "Digital Signal Processing: A Practical Approach", Emmanuael C. Ifeachor
 * and Barrie W. Jervis, Adison-Wesley, 1993.  ISBN 0-201-54413-X.
 *
 * This block emits a "taps_changed" signal upon activation and when one
 * of the parameters is modified. The "taps_changed" signal contains an
 * array of FIR taps, and can be connected to a FIR filter's set taps method.
 *
 * |category /GNURadio/Filters
 * |keywords fir filter taps highpass lowpass bandpass bandreject remez firdes
 *
 * |param band[Band Type] The band type of the FIR filter.
 * |widget ComboBox(editable=false)
 * |default "LOW_PASS"
 * |option [Low Pass] "LOW_PASS"
 * |option [High Pass] "HIGH_PASS"
 * |option [Band Pass] "BAND_PASS"
 * |option [Complex Band Pass] "COMPLEX_BAND_PASS"
 * |option [Band Reject] "BAND_REJECT"
 *
 * |param gain[Gain] The filter gain.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 1.0
 * |units dB
 * |preview enable
 *
 * |param sample_rate[Sample Rate] The sample rate, in samples per second.
 * The transition frequencies must be within the Nyqist frequency of the sampling rate.
 * |widget DoubleSpinBox(minimum=0,step=1.0)
 * |default 250e3
 * |units Sps
 * |preview enable
 *
 * |param low_freq[Lower Freq] The lower cutoff frequency.
 * For low pass filters, this is the end of the pass band. For high pass filters, this is the end of the stop band.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 11000
 * |units Hz
 * |preview when(enum=band, "LOW_PASS", "HIGH_PASS")
 *
 * |param high_freq[Upper Freq] The upper cutoff frequency.
 * For low pass filters, this is the start of the stop band. For high pass filters, this is the start of the pass band.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 12500
 * |units Hz
 * |preview when(enum=band, "LOW_PASS", "HIGH_PASS")
 *
 * |param low_passband_freq[Lower Passband Freq] The lower passband frequency.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 1e3
 * |units Hz
 * |preview when(enum=band, "BAND_PASS", "COMPLEX_BAND_PASS", "BAND_REJECT")
 *
 * |param high_passband_freq[Upper Passband Freq] The upper passband frequency.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 4e3
 * |units Hz
 * |preview when(enum=band, "BAND_PASS", "COMPLEX_BAND_PASS", "BAND_REJECT")
 *
 * |param low_stopband_freq[Lower Stopband Freq] The lower stopband frequency.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 2e3
 * |units Hz
 * |preview when(enum=band, "BAND_PASS", "COMPLEX_BAND_PASS", "BAND_REJECT")
 *
 * |param high_stopband_freq[Upper Stopband Freq] The upper stopband frequency.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 3e3
 * |units Hz
 * |preview when(enum=band, "BAND_PASS", "COMPLEX_BAND_PASS", "BAND_REJECT")
 *
 * |param passband_ripple[Passband Ripple] Desired passband ripple.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 0.1
 * |units dB
 * |preview enable
 *
 * |param stopband_atten[Stopband Attenuation] Desired stopband attenuation.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 60.0
 * |units dB
 * |preview enable
 *
 * |factory /gr/filter/optimal_fir_designer()
 * |setter set_band_type(band)
 * |setter set_gain(gain)
 * |setter set_sample_rate(sample_rate)
 * |setter set_low_freq(low_freq)
 * |setter set_high_freq(high_freq)
 * |setter set_low_passband_freq(low_passband_freq)
 * |setter set_high_passband_freq(high_passband_freq)
 * |setter set_passband_ripple(passband_ripple)
 * |setter set_low_stopband_freq(low_stopband_freq)
 * |setter set_high_stopband_freq(high_stopband_freq)
 * |setter set_stopband_atten(stopband_atten)
 **********************************************************************/
class GrFilterOptimalFIRDesigner: public Pothos::Block
{
public:
    static Pothos::Block* make()
    {
        return new GrFilterOptimalFIRDesigner();
    }
    
    GrFilterOptimalFIRDesigner():
        Pothos::Block(),
        d_band_type("LOW_PASS"),
        d_gain(1.0),
        d_sample_rate(250e3),
        d_low_freq(11000),
        d_high_freq(12500),
        d_low_passband_freq(1e3),
        d_high_passband_freq(4e3),
        d_passband_ripple(0.1),
        d_low_stopband_freq(2e3),
        d_high_stopband_freq(3e3),
        d_stopband_atten(60.0),
        d_taps()
    {
        REGISTER_GETTER_SETTER(band_type)
        REGISTER_GETTER_SETTER(gain)
        REGISTER_GETTER_SETTER(sample_rate)

        REGISTER_GETTER_SETTER(low_freq)
        REGISTER_GETTER_SETTER(high_freq)

        REGISTER_GETTER_SETTER(low_passband_freq)
        REGISTER_GETTER_SETTER(high_passband_freq)
        REGISTER_GETTER_SETTER(passband_ripple)

        REGISTER_GETTER_SETTER(low_stopband_freq)
        REGISTER_GETTER_SETTER(high_stopband_freq)
        REGISTER_GETTER_SETTER(stopband_atten)

        this->registerProbe("taps", "taps_triggered", "probe_taps");
        this->registerSignal("taps_changed");

        this->recalculate();
    }

    Pothos::Object taps() const
    {
        if(!d_taps)
        {
            throw Pothos::AssertionViolationException("d_taps is uninitialized");
        }

        return d_taps;
    }

    std::string band_type() const
    {
        return d_band_type;
    }

    void set_band_type(const std::string& band_type)
    {
        static const std::vector<std::string> valid_types =
        {
            "LOW_PASS",
            "HIGH_PASS",
            "BAND_PASS",
            "COMPLEX_BAND_PASS",
            "BAND_REJECT"
        };
        if(valid_types.end() == std::find(
                                    valid_types.begin(),
                                    valid_types.end(),
                                    band_type))
        {
            throw Pothos::InvalidArgumentException("Invalid filter type", band_type);
        }

        d_band_type = band_type;

        this->recalculate();
    }

    IMPL_GETTER_SETTER(gain, double)
    IMPL_GETTER_SETTER(sample_rate, double)

    IMPL_GETTER_SETTER(low_freq, double)
    IMPL_GETTER_SETTER(high_freq, double)

    IMPL_GETTER_SETTER(low_passband_freq, double)
    IMPL_GETTER_SETTER(high_passband_freq, double)
    IMPL_GETTER_SETTER(passband_ripple, double)

    IMPL_GETTER_SETTER(low_stopband_freq, double)
    IMPL_GETTER_SETTER(high_stopband_freq, double)
    IMPL_GETTER_SETTER(stopband_atten, double)

private:
    std::string d_band_type;

    double d_gain;
    double d_sample_rate;

    double d_low_freq;
    double d_high_freq;

    double d_low_passband_freq;
    double d_high_passband_freq;
    double d_passband_ripple;

    double d_low_stopband_freq;
    double d_high_stopband_freq;
    double d_stopband_atten;

    Pothos::Object d_taps; // Can be real or complex

    void recalculate()
    {
        if("LOW_PASS" == d_band_type)
        {
            d_taps = Pothos::Object(optfir_low_pass(
                                        d_gain,
                                        d_sample_rate,
                                        d_low_freq,
                                        d_high_freq,
                                        d_passband_ripple,
                                        d_stopband_atten));
        }
        else if("HIGH_PASS" == d_band_type)
        {
            d_taps = Pothos::Object(optfir_high_pass(
                                        d_gain,
                                        d_sample_rate,
                                        d_low_freq,
                                        d_high_freq,
                                        d_passband_ripple,
                                        d_stopband_atten));
        }
        else if("BAND_PASS" == d_band_type)
        {
            d_taps = Pothos::Object(optfir_band_pass(
                                        d_gain,
                                        d_sample_rate,
                                        d_low_stopband_freq,
                                        d_low_passband_freq,
                                        d_high_passband_freq,
                                        d_high_stopband_freq,
                                        d_passband_ripple,
                                        d_stopband_atten));
        }
        else if("COMPLEX_BAND_PASS" == d_band_type)
        {
            d_taps = Pothos::Object(optfir_complex_band_pass(
                                        d_gain,
                                        d_sample_rate,
                                        d_low_stopband_freq,
                                        d_low_passband_freq,
                                        d_high_passband_freq,
                                        d_high_stopband_freq,
                                        d_passband_ripple,
                                        d_stopband_atten));
        }
        else if("BAND_REJECT" == d_band_type)
        {
            d_taps = Pothos::Object(optfir_band_reject(
                                        d_gain,
                                        d_sample_rate,
                                        d_low_passband_freq,
                                        d_low_stopband_freq,
                                        d_high_stopband_freq,
                                        d_high_passband_freq,
                                        d_passband_ripple,
                                        d_stopband_atten));
        }
        else
        {
            throw Pothos::AssertionViolationException(
                      "Stored band type didn't trigger a recalculate",
                      d_band_type);
        }
        
        this->emitSignal("taps_changed", d_taps);
    }
};

static Pothos::BlockRegistry registerGrFilterOptimalFIRDesigner(
    "/gr/filter/optimal_fir_designer",
    Pothos::Callable(&GrFilterOptimalFIRDesigner::make));
