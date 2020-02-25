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

//
// Ported from: gr-analog/python/fm_emph.py
//

class fm_deemph: public Pothos::Topology
{
public:
    Pothos::Topology* make(
        float sample_rate,
        float tau)
    {
        return new fm_deemph(sample_rate, tau);
    }

    fm_deemph(
        float sample_rate,
        float tau
    ):
        Pothos::Topology(),
        d_sample_rate(sample_rate),
        d_tau(tau),
        d_iir_filter_ffd(Pothos::BlockRegistry::make(
                             "/gr/filter/iir_filter_ffd",
                             std::vector<float>{},
                             std::vector<float>{},
                             false))
    {
        this->connect(this, 0, d_iir_filter_ffd, 0);
        this->connect(d_iir_filter_ffd, 0, this, 0);
        
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_deemph, sample_rate));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_deemph, set_sample_rate));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_deemph, tau));
        this->registerCall(this, POTHOS_FCN_TUPLE(fm_deemph, set_tau));
        
        this->recalculate();
    }

    float sample_rate() const
    {
        return d_sample_rate;
    }

    void set_sample_rate(float sample_rate)
    {
        d_sample_rate = sample_rate;

        this->recalculate();
    }

    float tau() const
    {
        return d_tau;
    }

    void set_tau(float tau)
    {
        d_tau = tau;

        this->recalculate();
    }

    std::vector<float> fftaps() const
    {
        return d_fftaps;
    }

    std::vector<float> fbtaps() const
    {
        return d_fbtaps;
    }

private:
    float d_sample_rate;
    float d_tau;

    std::vector<float> d_fftaps;
    std::vector<float> d_fbtaps;

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

        d_fbtaps = std::vector<float>{1.0f, -p1};
        d_fftaps = std::vector<float>{b0 * 1.0f, b0 * -z1};

        d_iir_filter_ffd.call("set_taps", d_fftaps, d_fbtaps);
    }
};

class fm_preemph: public Pothos::Topology
{
public:
    Pothos::Topology* make(
        float sample_rate,
        float tau,
        float high_freq)
    {
        return new fm_preemph(sample_rate, tau, high_freq);
    }

    fm_preemph(
        float sample_rate,
        float tau,
        float high_freq
    ):
        Pothos::Topology(),
        d_sample_rate(sample_rate),
        d_tau(tau),
        d_high_freq(high_freq),
        d_iir_filter_ffd(Pothos::BlockRegistry::make(
                             "/gr/filter/iir_filter_ffd",
                             std::vector<float>{},
                             std::vector<float>{},
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

    float sample_rate() const
    {
        return d_sample_rate;
    }

    void set_sample_rate(float sample_rate)
    {
        d_sample_rate = sample_rate;

        this->recalculate();
    }

    float tau() const
    {
        return d_tau;
    }

    void set_tau(float tau)
    {
        d_tau = tau;

        this->recalculate();
    }

    float high_freq() const
    {
        return d_high_freq;
    }

    void set_high_freq(float high_freq)
    {
        d_high_freq = high_freq;

        this->recalculate();
    }

    std::vector<float> fftaps() const
    {
        return d_fftaps;
    }

    std::vector<float> fbtaps() const
    {
        return d_fbtaps;
    }

private:
    float d_sample_rate;
    float d_tau;
    float d_high_freq;

    std::vector<float> d_fftaps;
    std::vector<float> d_fbtaps;

    Pothos::Proxy d_iir_filter_ffd;

    void recalculate()
    {
        // Set fh to something sensible, if needed.
        // N.B. fh == fs/2.0 or fh == 0.0 results in a pole on the unit circle
        // at z = -1.0 or z = 1.0 respectively.  That makes the filter unstable
        // and useless.
        if((d_sample_rate <= 0.0f) || (d_high_freq >= (d_sample_rate / 2.0f)))
        {
            d_high_freq = 0.925f * (d_sample_rate / 2.0f);
        }

        // Digital corner frequencies
        const float w_cl = 1.0f / d_tau;
        const float w_ch = 2.0f * M_PI * d_high_freq;

        // Prewarped analog corner frequencies
        const float w_cla = 2.0f * d_sample_rate * std::tan(w_cl / (2.0f * d_sample_rate));
        const float w_cha = 2.0f * d_sample_rate * std::tan(w_ch / (2.0f * d_sample_rate));

        // Resulting digital pole, zero, and gain term from the bilinear
        // transformation of H(s) = (s + w_cla) / (s + w_cha) to
        // H(z) = b0 (1 - z1 z^-1)/(1 - p1 z^-1)
        const float kl = -w_cla / (2.0f * d_sample_rate);
        const float kh = -w_cha / (2.0f * d_sample_rate);
        const float z1 = (1.0f + kl) / (1.0f - kl);
        const float p1 = (1.0f + kh) / (1.0f - kh);
        const float b0 = (1.0f - kl) / (1.0f - kh);

        // Since H(s = infinity) = 1.0, then H(z = -1) = 1.0 and
        // this filter  has 0 dB gain at fs/2.0.
        // That isn't what users are going to expect, so adjust with a
        // gain, g, so that H(z = 1) = 1.0 for 0 dB gain at DC.
        constexpr float w_0dB = 2.0f * M_PI * 0.0f;
        const float g = std::abs(1.0f - p1 * std::polar(1.0f, -w_0dB))
                      / (b0 * std::abs(1.0f - z1 * std::polar(1.0f, -w_0dB)));

        d_fftaps = {1.0, -p1};
        d_fbtaps = {g * b0 * 1.0f, g * b0 * -z1};

        d_iir_filter_ffd.call("set_taps", d_fftaps, d_fbtaps);
    }
};

static Pothos::BlockRegistry registerFmDeemph(
    "/gr/analog/fm_deemph",
    Pothos::Callable(&fm_deemph::make));

static Pothos::BlockRegistry registerFmPreemph(
    "/gr/analog/fm_preemph",
    Pothos::Callable(&fm_preemph::make));
