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

#include <gnuradio/filter/firdes.h>

#include <algorithm>
#include <string>
#include <vector>

#define REGISTER_GETTER_SETTER(field_name) \
    this->registerCall(this, POTHOS_FCN_TUPLE(GrFilterFIRDesigner, field_name)); \
    this->registerCall(this, POTHOS_FCN_TUPLE(GrFilterFIRDesigner, set_ ## field_name)); \
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
 * |PothosDoc FIR Designer
 *
 * This block wraps GNU Radio's FIR filter design functionality. It emits
 * a "taps_changed" signal upon activation and when one of the parameters
 * is modified. The "taps_changed" signal contains an array of FIR taps
 * and can be connected to a FIR filter's set taps method.
 *
 * |category /GNURadio/Filters
 * |keywords firdes filter taps highpass lowpass bandpass bandreject hilbert root raised cosine rrc gaussian
 *
 * |param filter_type[Filter Type] The band type of the FIR filter.
 * |widget ComboBox(editable=false)
 * |default "LOW_PASS_2"
 * |option [Low Pass] "LOW_PASS_2"
 * |option [Low Pass (max atten.)] "LOW_PASS"
 * |option [High Pass] "HIGH_PASS_2"
 * |option [High Pass (max atten.)] "HIGH_PASS"
 * |option [Band Pass] "BAND_PASS_2"
 * |option [Band Pass (max atten.)] "BAND_PASS"
 * |option [Complex Band Pass] "COMPLEX_BAND_PASS_2"
 * |option [Complex Band Pass (max atten.)] "COMPLEX_BAND_PASS"
 * |option [Band Reject] "BAND_REJECT_2"
 * |option [Band Reject (max atten.)] "BAND_REJECT"
 * |option [Hilbert] "HILBERT"
 * |option [Root Raised Cosine] "ROOT_RAISED_COSINE"
 * |option [Gaussian] "GAUSSIAN"
 * |preview enable
 *
 * |param gain[Gain] The filter gain.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 1.0
 * |units dB
 * |preview enable
 *
 * |param sampling_freq[Sampling Freq] The sample rate.
 * The transition frequencies must be within the Nyquist frequency of the sampling rate.
 * |widget DoubleSpinBox(minimum=0,step=1.0)
 * |default 250e3
 * |units Hz
 * |preview when(enum=filter_type, "LOW_PASS", "LOW_PASS_2", "HIGH_PASS", "HIGH_PASS_2", "BAND_PASS", "BAND_PASS_2", "COMPLEX_BAND_PASS", "COMPLEX_BAND_PASS_2", "BAND_REJECT", "BAND_REJECT_2")
 *
 * |param cutoff_freq[Cutoff Freq] The cutoff frequency.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 11000
 * |units Hz
 * |preview when(enum=filter_type, "LOW_PASS", "LOW_PASS_2", "HIGH_PASS", "HIGH_PASS_2")
 *
 * |param low_cutoff_freq[Lower Cutoff Freq] The lower cutoff frequency.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 11000
 * |units Hz
 * |preview when(enum=filter_type, "BAND_PASS", "BAND_PASS_2", "COMPLEX_BAND_PASS", "COMPLEX_BAND_PASS_2", "BAND_REJECT", "BAND_REJECT_2")
 *
 * |param high_cutoff_freq[Upper Cutoff Freq] The upper cutoff frequency.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 12500
 * |units Hz
 * |preview when(enum=filter_type, "BAND_PASS", "BAND_PASS_2", "COMPLEX_BAND_PASS", "COMPLEX_BAND_PASS_2", "BAND_REJECT", "BAND_REJECT_2")
 *
 * |param transition_width[Transition Width] Width of transition band.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 1e3
 * |units Hz
 * |preview when(enum=filter_type, "LOW_PASS", "LOW_PASS_2", "HIGH_PASS", "HIGH_PASS_2", "BAND_PASS", "BAND_PASS_2", "COMPLEX_BAND_PASS", "COMPLEX_BAND_PASS_2", "BAND_REJECT", "BAND_REJECT_2")
 *
 * |param win_type [Window Type] The window type to apply to the taps.
 * |widget ComboBox(editable=false)
 * |option [Hamming] "WIN_HAMMING"
 * |option [Hann] "WIN_HANN"
 * |option [Blackman] "WIN_BLACKMAN"
 * |option [Rectangular] "WIN_RECTANGULAR"
 * |option [Kaiser] "WIN_KAISER"
 * |option [Blackman-Harris] "WIN_BLACKMAN_HARRIS"
 * |option [Bartlett] "WIN_BARTLETT"
 * |option [Flat-top] "WIN_FLATTOP"
 * |default "WIN_HAMMING"
 * |preview when(enum=filter_type, "LOW_PASS", "LOW_PASS_2", "HIGH_PASS", "HIGH_PASS_2", "BAND_PASS", "BAND_PASS_2", "COMPLEX_BAND_PASS", "COMPLEX_BAND_PASS_2", "BAND_REJECT", "BAND_REJECT_2", "HILBERT")
 *
 * |param kaiser_beta[Kaiser Beta] The beta parameter (Kaiser windowing only).
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 6.76
 * |preview when(enum=win_type, "WIN_KAISER")
 *
 * |param attenuation_dB[Attenuation] The out-of-band attenuation.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |units dB
 * |default 60
 * |preview when(enum=filter_type, "BAND_PASS_2", "COMPLEX_BAND_PASS_2", "BAND_REJECT_2")
 *
 * |param symbol_rate[Symbol Rate] Root raised cosine and gaussian only. For Gaussian, must be a factor of the sampling freq.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 10.76
 * |preview when(enum=filter_type, "ROOT_RAISED_COSINE", "GAUSSIAN")
 *
 * |param ntaps[Num Taps] Manually specified tap count.
 * |widget SpinBox(minimum=1)
 * |default 19
 * |preview when(enum=filter_type, "HILBERT", "ROOT_RAISED_COSINE", "GAUSSIAN")
 *
 * |param alpha[Alpha] The excess bandwidth factor (root raised cosine only).
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 1e-3
 * |preview when(enum=filter_type, "ROOT_RAISED_COSINE")
 *
 * |param bt[Bandwidth/Bitrate Ratio] Gaussian only.
 * |widget DoubleSpinBox(minimum=0.0,step=0.01)
 * |default 1.0
 * |preview when(enum=filter_type, "GAUSSIAN")
 *
 * |factory /gr/filter/fir_designer()
 * |setter set_filter_type(filter_type)
 * |setter set_gain(gain)
 * |setter set_sampling_freq(sampling_freq)
 * |setter set_low_cutoff_freq(low_cutoff_freq)
 * |setter set_high_cutoff_freq(high_cutoff_freq)
 * |setter set_transition_width(transition_width)
 * |setter set_win_type(win_type)
 * |setter set_kaiser_beta(kaiser_beta)
 * |setter set_attenuation_dB(attenuation_dB)
 * |setter set_symbol_rate(symbol_rate)
 * |setter set_ntaps(ntaps)
 * |setter set_alpha(alpha)
 * |setter set_bt(bt)
 **********************************************************************/
class GrFilterFIRDesigner: public Pothos::Block
{
public:
    static Pothos::Block* make()
    {
        return new GrFilterFIRDesigner();
    }

    GrFilterFIRDesigner():
        Pothos::Block(),
        d_filter_type("LOW_PASS"),
        d_gain(1.0),
        d_sampling_freq(250e3),
        d_cutoff_freq(12500),
        d_low_cutoff_freq(11000),
        d_high_cutoff_freq(12500),
        d_transition_width(1e3),
        d_win_type("WIN_HAMMING"),
        d_kaiser_beta(6.76),
        d_attenuation_dB(60),
        d_symbol_rate(10.76),
        d_ntaps(19),
        d_alpha(0.001),
        d_bt(1.0),
        d_taps()
    {
        REGISTER_GETTER_SETTER(filter_type)
        REGISTER_GETTER_SETTER(gain)
        REGISTER_GETTER_SETTER(sampling_freq)
        REGISTER_GETTER_SETTER(cutoff_freq)
        REGISTER_GETTER_SETTER(low_cutoff_freq)
        REGISTER_GETTER_SETTER(high_cutoff_freq)
        REGISTER_GETTER_SETTER(transition_width)
        REGISTER_GETTER_SETTER(win_type);
        REGISTER_GETTER_SETTER(kaiser_beta)
        REGISTER_GETTER_SETTER(attenuation_dB)
        REGISTER_GETTER_SETTER(symbol_rate)
        REGISTER_GETTER_SETTER(ntaps)
        REGISTER_GETTER_SETTER(alpha)
        REGISTER_GETTER_SETTER(bt)

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

    IMPL_GETTER_SETTER(gain, double)
    IMPL_GETTER_SETTER(sampling_freq, double)
    IMPL_GETTER_SETTER(cutoff_freq, double)
    IMPL_GETTER_SETTER(low_cutoff_freq, double)
    IMPL_GETTER_SETTER(high_cutoff_freq, double)
    IMPL_GETTER_SETTER(transition_width, double)
    IMPL_GETTER_SETTER(kaiser_beta, double)
    IMPL_GETTER_SETTER(attenuation_dB, double)
    IMPL_GETTER_SETTER(symbol_rate, double)
    IMPL_GETTER_SETTER(ntaps, int)
    IMPL_GETTER_SETTER(alpha, double)
    IMPL_GETTER_SETTER(bt, double)

    std::string filter_type() const
    {
        return d_filter_type;
    }

    void set_filter_type(const std::string& filter_type)
    {
        static const std::vector<std::string> valid_types =
        {
            "LOW_PASS",
            "LOW_PASS_2",
            "HIGH_PASS",
            "HIGH_PASS_2",
            "BAND_PASS",
            "BAND_PASS_2",
            "COMPLEX_BAND_PASS",
            "COMPLEX_BAND_PASS_2",
            "BAND_REJECT",
            "BAND_REJECT_2",
            "HILBERT",
            "ROOT_RAISED_COSINE",
            "GAUSSIAN"
        };
        if(valid_types.end() == std::find(valid_types.begin(), valid_types.end(), filter_type))
        {
            throw Pothos::InvalidArgumentException("Invalid filter type", filter_type);
        }

        this->recalculate();
    }

    std::string win_type() const
    {
        return d_win_type;
    }

    void set_win_type(const std::string& win_type)
    {
        // This makes sure we can convert to the internal enum.
        try{Pothos::Object(win_type).convert<gr::filter::firdes::win_type>();}
        catch(const Pothos::ObjectConvertError&){
            throw Pothos::InvalidArgumentException("Invalid window type", win_type);
        }

        d_win_type = win_type;

        this->recalculate();
    }

private:
    std::string d_filter_type;

    double d_gain;
    double d_sampling_freq;
    double d_cutoff_freq;
    double d_low_cutoff_freq;
    double d_high_cutoff_freq;
    double d_transition_width;
    std::string d_win_type;
    double d_kaiser_beta;
    double d_attenuation_dB;
    double d_symbol_rate;
    int d_ntaps;
    double d_alpha;
    double d_bt;

    Pothos::Object d_taps; // Can be real or complex

    void recalculate()
    {
        if("LOW_PASS" == d_filter_type)
        {
            d_taps = Pothos::Object(gr::filter::firdes::low_pass(
                                        d_gain,
                                        d_sampling_freq,
                                        d_cutoff_freq,
                                        d_transition_width,
                                        Pothos::Object(d_win_type).convert<gr::filter::firdes::win_type>(),
                                        d_kaiser_beta));
        }
        else if("LOW_PASS_2" == d_filter_type)
        {
            d_taps = Pothos::Object(gr::filter::firdes::low_pass_2(
                                        d_gain,
                                        d_sampling_freq,
                                        d_cutoff_freq,
                                        d_transition_width,
                                        d_attenuation_dB,
                                        Pothos::Object(d_win_type).convert<gr::filter::firdes::win_type>(),
                                        d_kaiser_beta));
        }
        else if("HIGH_PASS" == d_filter_type)
        {
            d_taps = Pothos::Object(gr::filter::firdes::high_pass(
                                        d_gain,
                                        d_sampling_freq,
                                        d_cutoff_freq,
                                        d_transition_width,
                                        Pothos::Object(d_win_type).convert<gr::filter::firdes::win_type>(),
                                        d_kaiser_beta));
        }
        else if("HIGH_PASS_2" == d_filter_type)
        {
            d_taps = Pothos::Object(gr::filter::firdes::high_pass_2(
                                        d_gain,
                                        d_sampling_freq,
                                        d_cutoff_freq,
                                        d_transition_width,
                                        d_attenuation_dB,
                                        Pothos::Object(d_win_type).convert<gr::filter::firdes::win_type>(),
                                        d_kaiser_beta));
        }
        else if("BAND_PASS" == d_filter_type)
        {
            d_taps = Pothos::Object(gr::filter::firdes::band_pass(
                                        d_gain,
                                        d_sampling_freq,
                                        d_low_cutoff_freq,
                                        d_high_cutoff_freq,
                                        d_transition_width,
                                        Pothos::Object(d_win_type).convert<gr::filter::firdes::win_type>(),
                                        d_kaiser_beta));
        }
        else if("BAND_PASS_2" == d_filter_type)
        {
            d_taps = Pothos::Object(gr::filter::firdes::band_pass_2(
                                        d_gain,
                                        d_sampling_freq,
                                        d_low_cutoff_freq,
                                        d_high_cutoff_freq,
                                        d_transition_width,
                                        d_attenuation_dB,
                                        Pothos::Object(d_win_type).convert<gr::filter::firdes::win_type>(),
                                        d_kaiser_beta));
        }
        else if("COMPLEX_BAND_PASS" == d_filter_type)
        {
            d_taps = Pothos::Object(gr::filter::firdes::complex_band_pass(
                                        d_gain,
                                        d_sampling_freq,
                                        d_low_cutoff_freq,
                                        d_high_cutoff_freq,
                                        d_transition_width,
                                        Pothos::Object(d_win_type).convert<gr::filter::firdes::win_type>(),
                                        d_kaiser_beta));
        }
        else if("COMPLEX_BAND_PASS_2" == d_filter_type)
        {
            d_taps = Pothos::Object(gr::filter::firdes::complex_band_pass_2(
                                        d_gain,
                                        d_sampling_freq,
                                        d_low_cutoff_freq,
                                        d_high_cutoff_freq,
                                        d_transition_width,
                                        d_attenuation_dB,
                                        Pothos::Object(d_win_type).convert<gr::filter::firdes::win_type>(),
                                        d_kaiser_beta));
        }
        else if("BAND_REJECT" == d_filter_type)
        {
            d_taps = Pothos::Object(gr::filter::firdes::band_reject(
                                        d_gain,
                                        d_sampling_freq,
                                        d_low_cutoff_freq,
                                        d_high_cutoff_freq,
                                        d_transition_width,
                                        Pothos::Object(d_win_type).convert<gr::filter::firdes::win_type>(),
                                        d_kaiser_beta));
        }
        else if("BAND_REJECT_2" == d_filter_type)
        {
            d_taps = Pothos::Object(gr::filter::firdes::band_reject_2(
                                        d_gain,
                                        d_sampling_freq,
                                        d_low_cutoff_freq,
                                        d_high_cutoff_freq,
                                        d_transition_width,
                                        d_attenuation_dB,
                                        Pothos::Object(d_win_type).convert<gr::filter::firdes::win_type>(),
                                        d_kaiser_beta));
        }
        else if("HILBERT" == d_filter_type)
        {
            d_taps = Pothos::Object(gr::filter::firdes::hilbert(
                                        d_ntaps,
                                        Pothos::Object(d_win_type).convert<gr::filter::firdes::win_type>(),
                                        d_kaiser_beta));
        }
        else if("ROOT_RAISED_COSINE" == d_filter_type)
        {
            d_taps = Pothos::Object(gr::filter::firdes::root_raised_cosine(
                                        d_gain,
                                        d_sampling_freq,
                                        d_symbol_rate,
                                        d_alpha,
                                        d_ntaps));
        }
        else if("GAUSSIAN" == d_filter_type)
        {
            d_taps = Pothos::Object(gr::filter::firdes::gaussian(
                         d_gain,
                         d_symbol_rate,
                         d_bt,
                         d_ntaps));
        }
        else
        {
            throw Pothos::AssertionViolationException(
                      "Stored filter type didn't trigger a recalculate",
                      d_filter_type);
        }

        this->emitSignal("taps_changed", d_taps);
    }
};

static Pothos::BlockRegistry registerGrFilterFIRDesigner(
    "/gr/filter/fir_designer",
    Pothos::Callable(&GrFilterFIRDesigner::make));
