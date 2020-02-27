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
#include <complex>
#include <iostream>

//
// Ported from: gr-analog/python/fm_emph.py
//

class fm_deemph: public Pothos::Topology
{
public:
    static Pothos::Topology* make(
        double sample_rate,
        double tau)
    {
        return new fm_deemph(sample_rate, tau);
    }

    fm_deemph(
        double sample_rate,
        double tau
    ):
        Pothos::Topology(),
        d_sample_rate(sample_rate),
        d_tau(tau),
        d_iir_filter_ffd(Pothos::BlockRegistry::make(
                             "/gr/filter/iir_filter",
                             "iir_filter_ffd",
                             std::vector<double>{0.0}, // Passing in an empty vector
                             std::vector<double>{0.0}, // here results in a crash.
                             false))
    {
        this->connect(this, 0, d_iir_filter_ffd, 0);
        this->connect(d_iir_filter_ffd, 0, this, 0);
        
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_deemph, sample_rate));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_deemph, set_sample_rate));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_deemph, tau));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_deemph, set_tau));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_deemph, fftaps));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_deemph, fbtaps));
        
        this->recalculate();
    }

    double sample_rate() const
    {
        return d_sample_rate;
    }

    void set_sample_rate(double sample_rate)
    {
        d_sample_rate = sample_rate;

        this->recalculate();
    }

    double tau() const
    {
        return d_tau;
    }

    void set_tau(double tau)
    {
        d_tau = tau;

        this->recalculate();
    }

    std::vector<double> fftaps() const
    {
        return d_fftaps;
    }

    std::vector<double> fbtaps() const
    {
        return d_fbtaps;
    }

private:
    double d_sample_rate;
    double d_tau;

    std::vector<double> d_fftaps;
    std::vector<double> d_fbtaps;

    Pothos::Proxy d_iir_filter_ffd;

    void recalculate()
    {
        // Digital corner frequency
        const auto w_c = 1.0f / d_tau;

        // Prewarped analog corner frequency
        const auto w_ca = 2.0f * d_sample_rate * std::tan(w_c / (2.0f * d_sample_rate));

        // Resulting digital pole, zero, and gain term from the bilinear
        // transformation of H(s) = w_ca / (s + w_ca) to
        // H(z) = b0 (1 - z1 z^-1)/(1 - p1 z^-1)
        const auto k = -w_ca / (2.0f * d_sample_rate);
        constexpr auto z1 = -1.0f;
        const auto p1 = (1.0f + k) / (1.0f - k);
        const auto b0 = -k / (1.0f - k);

        d_fbtaps = std::vector<double>{1.0f, -p1};
        d_fftaps = std::vector<double>{b0 * 1.0f, b0 * -z1};

        d_iir_filter_ffd.call("set_taps", d_fftaps, d_fbtaps);
    }
};

class fm_preemph: public Pothos::Topology
{
public:
    static Pothos::Topology* make(
        double sample_rate,
        double tau,
        double high_freq)
    {
        return new fm_preemph(sample_rate, tau, high_freq);
    }

    fm_preemph(
        double sample_rate,
        double tau,
        double high_freq
    ):
        Pothos::Topology(),
        d_sample_rate(sample_rate),
        d_tau(tau),
        d_high_freq(high_freq),
        d_iir_filter_ffd(Pothos::BlockRegistry::make(
                             "/gr/filter/iir_filter",
                             "iir_filter_ffd",
                             std::vector<double>{0.0}, // Passing in an empty vector
                             std::vector<double>{0.0}, // here results in a crash.
                             false))
    {
        this->connect(this, 0, d_iir_filter_ffd, 0);
        this->connect(d_iir_filter_ffd, 0, this, 0);
        
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_preemph, sample_rate));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_preemph, set_sample_rate));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_preemph, tau));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_preemph, set_tau));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_preemph, high_freq));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_preemph, set_high_freq));
        
        this->recalculate();
    }

    double sample_rate() const
    {
        return d_sample_rate;
    }

    void set_sample_rate(double sample_rate)
    {
        d_sample_rate = sample_rate;

        this->recalculate();
    }

    double tau() const
    {
        return d_tau;
    }

    void set_tau(double tau)
    {
        d_tau = tau;

        this->recalculate();
    }

    double high_freq() const
    {
        return d_high_freq;
    }

    void set_high_freq(double high_freq)
    {
        d_high_freq = high_freq;

        this->recalculate();
    }

    std::vector<double> fftaps() const
    {
        return d_fftaps;
    }

    std::vector<double> fbtaps() const
    {
        return d_fbtaps;
    }

private:
    double d_sample_rate;
    double d_tau;
    double d_high_freq;

    std::vector<double> d_fftaps;
    std::vector<double> d_fbtaps;

    Pothos::Proxy d_iir_filter_ffd;

    void recalculate()
    {
        // Set fh to something sensible, if needed.
        // N.B. fh == fs/2.0 or fh == 0.0 results in a pole on the unit circle
        // at z = -1.0 or z = 1.0 respectively.  That makes the filter unstable
        // and useless.
        if((d_sample_rate <= 0.0) || (d_high_freq >= (d_sample_rate / 2.0)))
        {
            d_high_freq = 0.925 * (d_sample_rate / 2.0);
        }

        // Digital corner frequencies
        const auto w_cl = 1.0 / d_tau;
        const auto w_ch = 2.0 * M_PI * d_high_freq;

        // Prewarped analog corner frequencies
        const auto w_cla = 2.0 * d_sample_rate * std::tan(w_cl / (2.0 * d_sample_rate));
        const auto w_cha = 2.0 * d_sample_rate * std::tan(w_ch / (2.0 * d_sample_rate));

        // Resulting digital pole, zero, and gain term from the bilinear
        // transformation of H(s) = (s + w_cla) / (s + w_cha) to
        // H(z) = b0 (1 - z1 z^-1)/(1 - p1 z^-1)
        const auto kl = -w_cla / (2.0 * d_sample_rate);
        const auto kh = -w_cha / (2.0 * d_sample_rate);
        const auto z1 = (1.0 + kl) / (1.0 - kl);
        const auto p1 = (1.0 + kh) / (1.0 - kh);
        const auto b0 = (1.0 - kl) / (1.0 - kh);

        // Since H(s = infinity) = 1.0, then H(z = -1) = 1.0 and
        // this filter  has 0 dB gain at fs/2.0.
        // That isn't what users are going to expect, so adjust with a
        // gain, g, so that H(z = 1) = 1.0 for 0 dB gain at DC.
        constexpr auto w_0dB = 2.0 * M_PI * 0.0;
        const auto g = std::abs(1.0 - p1 * std::polar(1.0, -w_0dB))
                     / (b0 * std::abs(1.0 - z1 * std::polar(1.0, -w_0dB)));

        d_fftaps = {1.0, -p1};
        d_fbtaps = {g * b0 * 1.0, g * b0 * -z1};

        d_iir_filter_ffd.call("set_taps", d_fftaps, d_fbtaps);
    }
};

/***********************************************************************
 * |PothosDoc FM Deemphasis Filter
 *
 * This digital deemphasis filter design uses the
 * "bilinear transformation" method of designing digital filters:
 *
 * 1. Convert digital specifications into the analog domain, by prewarping
 *    digital frequency specifications into analog frequencies.
 *
 *    w_a = (2/T)tan(wT/2)
 *
 * 2. Use an analog filter design technique to design the filter.
 *
 * 3. Use the bilinear transformation to convert the analog filter design to a
 *    digital filter design.
 * Jackson, Leland B., _Digital_Filters_and_Signal_Processing_Second_Edition_,
 *   Kluwer Academic Publishers, 1989, pp 201-212
 *
 * Orfanidis, Sophocles J., _Introduction_to_Signal_Processing_, Prentice Hall,
 *   1996, pp 573-583
 *
 * |category /GNURadio/Modulators
 * |keywords frequency modulation fm emphasis
 *
 * |param sample_rate[Sample Rate] Incoming sample rate.
 * |widget SpinBox(minimum=0)
 * |default 250000
 * |preview enable
 *
 * |param tau[Tau] Deemphasis time constant.
 * |widget DoubleSpinBox(minimum=0,step=1e-6,decimals=9)
 * |default 75e-6
 * |preview enable
 *
 * |factory /gr/analog/fm_deemph(sample_rate, tau)
 **********************************************************************/
static Pothos::BlockRegistry registerFmDeemph(
    "/gr/analog/fm_deemph",
    Pothos::Callable(&fm_deemph::make));

/***********************************************************************
 * |PothosDoc FM Preemphasis Filter
 *
 * This digital deemphasis filter design uses the
 * "bilinear transformation" method of designing digital filters:
 *
 * 1. Convert digital specifications into the analog domain, by prewarping
 *    digital frequency specifications into analog frequencies.
 *
 *    w_a = (2/T)tan(wT/2)
 *
 * 2. Use an analog filter design technique to design the filter.
 *
 * 3. Use the bilinear transformation to convert the analog filter design to a
 *    digital filter design.
 * Jackson, Leland B., _Digital_Filters_and_Signal_Processing_Second_Edition_,
 *   Kluwer Academic Publishers, 1989, pp 201-212
 *
 * Orfanidis, Sophocles J., _Introduction_to_Signal_Processing_, Prentice Hall,
 *   1996, pp 573-583
 *
 * |category /GNURadio/Modulators
 * |keywords frequency modulation fm emphasis
 *
 * |param sample_rate[Sample Rate] Incoming sample rate.
 * |widget SpinBox(minimum=0)
 * |default 250000
 * |preview enable
 *
 * |param tau[Tau] Preemphasis time constant.
 * |widget DoubleSpinBox(minimum=0,step=1e-6,decimals=9)
 * |default 75e-6
 * |preview enable
 *
 * |param high_freq[High Freq] The frequency at which the filter flattens out.
 * |widget DoubleSpinBox(minimum=0)
 * |default 0.0
 * |preview enable
 *
 * |factory /gr/analog/fm_preemph(sample_rate, tau, high_freq)
 **********************************************************************/
static Pothos::BlockRegistry registerFmPreemph(
    "/gr/analog/fm_preemph",
    Pothos::Callable(&fm_preemph::make));
