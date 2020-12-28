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

#include "optfir.h"

#include <gnuradio/filter/pm_remez.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

//
// This is ported from GNU Radio's optimal FIR filter routines, which are
// implemented in Python.
//
// See: gr-filter/python/optfir.py
//

// Convert a stopband attenuation in dB to an absolute value.
static inline double stopband_atten_to_dev(double atten_db)
{
    return std::pow(10.0, (atten_db / -20.0));
}

// Convert passband ripple spec expressed in dB to an absolute value.
static inline double passband_ripple_to_dev(double ripple_db)
{
    return (std::pow(10.0, (ripple_db / 20.0)) - 1.0) / (std::pow(10.0, (ripple_db / 20.0)) + 1);
}

/*
 * FIR lowpass filter length estimator.  freq1 and freq2 are
 * normalized to the sampling frequency.  delta_p is the passband
 * deviation (ripple), delta_s is the stopband deviation (ripple).
 * 
 * Note, this works for high pass filters too (freq1 > freq2), but
 * doesn't work well if the transition is near f == 0 or f == fs/2
 * 
 * From Herrmann et al (1973), Practical design rules for optimum
 * finite impulse response filters.  Bell System Technical J., 52, 769-99
 */
static double lporder(double freq1, double freq2, double delta_p, double delta_s)
{
    const auto df = std::abs(freq2 - freq1);
    const auto ddp = std::log10(delta_p);
    const auto dds = std::log10(delta_s);

    constexpr auto a1 = 5.309e-3;
    constexpr auto a2 = 7.114e-2;
    constexpr auto a3 = -4.761e-1;
    constexpr auto a4 = -2.66e-3;
    constexpr auto a5 = -5.941e-1;
    constexpr auto a6 = -4.278e-1;

    constexpr auto b1 = 11.01217;
    constexpr auto b2 = 0.5124401;

    const auto t1 = a1 * ddp * ddp;
    const auto t2 = a2 * ddp;
    const auto t3 = a4 * ddp * ddp;
    const auto t4 = a5 * ddp;

    const auto dinf = ((t1 + t2 + a3) * dds) + (t3 + t4 + a6);
    const auto ff = b1 + b2 * (ddp - dds);
    const auto n = dinf / df - ff * df + 1;

    return n;
}

struct pm_remez_params_t
{
    int order;
    std::vector<double> bands;
    std::vector<double> ampls;
    std::vector<double> error_weight;
};

/*
 * FIR order estimator (lowpass, highpass, bandpass, mulitiband).
 * 
 * (n, fo, ao, w) = remezord (f, a, dev)
 * (n, fo, ao, w) = remezord (f, a, dev, fs)
 * 
 * (n, fo, ao, w) = remezord (f, a, dev) finds the approximate order,
 * normalized frequency band edges, frequency band amplitudes, and
 * weights that meet input specifications f, a, and dev, to use with
 * the remez command.
 * 
 * * f is a sequence of frequency band edges (between 0 and Fs/2, where
 *   Fs is the sampling frequency), and a is a sequence specifying the
 *   desired amplitude on the bands defined by f. The length of f is
 *   twice the length of a, minus 2. The desired function is
 *   piecewise constant.
 * 
 * * dev is a sequence the same size as a that specifies the maximum
 *   allowable deviation or ripples between the frequency response
 *   and the desired amplitude of the output filter, for each band.
 * 
 * Use remez with the resulting order n, frequency sequence fo,
 * amplitude response sequence ao, and weights w to design the filter b
 * which approximately meets the specifications given by remezord
 * input parameters f, a, and dev:
 * 
 * b = remez (n, fo, ao, w)
 * 
 * (n, fo, ao, w) = remezord (f, a, dev, Fs) specifies a sampling frequency Fs.
 * 
 * Fs defaults to 2 Hz, implying a Nyquist frequency of 1 Hz. You can
 * therefore specify band edges scaled to a particular applications
 * sampling frequency.
 * 
 * In some cases remezord underestimates the order n. If the filter
 * does not meet the specifications, try a higher order such as n+1
 * or n+2.
 */
static pm_remez_params_t remezord(
    const std::vector<double>& fcuts,
    const std::vector<double>& mags,
    const std::vector<double>& devs,
    double fsamp = 2.0)
{
    pm_remez_params_t ret;

    std::vector<double> fcuts2;
    std::transform(
        fcuts.begin(),
        fcuts.end(),
        std::back_inserter(fcuts2),
        [&fsamp](double f){return f / fsamp;});

    const auto nf = fcuts.size();
    const auto nm = mags.size();
    const auto nd = devs.size();
    const auto nbands = nm;

    if(nm != nd)
        throw Pothos::InvalidArgumentException("Length of mags and devs must be equal");

    if(nf != (2 * (nbands - 1)))
        throw Pothos::InvalidArgumentException("Length of f must be 2 * len(mags) - 2");

    auto devs2 = devs;
    for(size_t i = 0; i < nm; ++i)
    {
        if(0 != mags[i]) devs2[i] /= mags[i];
    }

    // Separate the passband and stopband edges.
    std::vector<double> f1, f2;
    for(size_t i = 0; i < nf; ++i)
    {
        if(i % 2) f2.emplace_back(fcuts2[i]);
        else      f1.emplace_back(fcuts2[i]);
    }

    int n = 0;
    double min_delta = 2;
    for(size_t i = 0; i < f1.size(); ++i)
    {
        if((f2[i] - f1[i]) < min_delta)
        {
            n = static_cast<int>(i);
            min_delta = f2[i] - f1[i];
        }
    }

    double l = 0;
    if(2 == nbands)
    {
        // Lowpass or highpass case (use formula)
        l = lporder(f1[n], f2[n], devs2[0], devs2[1]);
    }
    else
    {
        // Bandpass or multipass case
        // Try different lowpasses and take the worst one that
        // goes through the BP specs.
        for(size_t i = 1; i < (nbands-1); ++i)
        {
            const auto l1 = lporder(f1[i-1], f2[i-1], devs2[i], devs2[i-1]);
            const auto l2 = lporder(f1[i], f2[i], devs2[i], devs2[i+1]);
            l = std::max(l, std::max(l1, l2));
        }
    }

    n = static_cast<int>(std::ceil(l) - 1); // Need order, not length for Remez.

    // Cook up Remez-compatible result.
    std::vector<double> ff{0};
    std::transform(
        fcuts2.begin(),
        fcuts2.end(),
        std::back_inserter(ff),
        [](double f){return f * 2.0;});
    ff.emplace_back(1.0);

    std::vector<double> aa;
    std::for_each(
        mags.begin(),
        mags.end(),
        [&aa](double a){
            aa.emplace_back(a);
            aa.emplace_back(a);});

    const auto max_dev = *std::max_element(devs2.begin(), devs2.end());
    std::vector<double> wts;
    std::transform(
        devs2.begin(),
        devs2.end(),
        std::back_inserter(wts),
        [&max_dev](double dev){return max_dev / dev;});

    ret.order = n;
    ret.bands = std::move(ff);
    ret.ampls = std::move(aa);
    ret.error_weight = std::move(wts);

    return ret;
}

std::vector<double> optfir_low_pass(
    double gain,
    double Fs,
    double freq1,
    double freq2,
    double passband_ripple_db,
    double stopband_atten_db,
    size_t nextra_taps)
{
    const auto passband_dev = passband_ripple_to_dev(passband_ripple_db);
    const auto stopband_dev = stopband_atten_to_dev(stopband_atten_db);
    const auto desired_ampls = std::vector<double>{gain, 0.0};
    const auto remez_params = remezord(
                                  std::vector<double>{freq1, freq2},
                                  desired_ampls,
                                  std::vector<double>{passband_dev, stopband_dev},
                                  Fs);

    return gr::filter::pm_remez(
               remez_params.order + nextra_taps,
               remez_params.bands,
               remez_params.ampls,
               remez_params.error_weight,
               "bandpass");
}

std::vector<double> optfir_band_pass(
    double gain,
    double Fs,
    double freq_sb1,
    double freq_pb1,
    double freq_pb2,
    double freq_sb2,
    double passband_ripple_db,
    double stopband_atten_db,
    size_t nextra_taps)
{
    const auto passband_dev = passband_ripple_to_dev(passband_ripple_db);
    const auto stopband_dev = stopband_atten_to_dev(stopband_atten_db);
    const auto desired_ampls = std::vector<double>{0.0, gain, 0.0};
    const auto desired_freqs = std::vector<double>{freq_sb1, freq_pb1, freq_pb2, freq_sb2};
    const auto desired_ripple = std::vector<double>{stopband_dev, passband_dev, stopband_dev};
    const auto remez_params = remezord(
                                  desired_freqs,
                                  desired_ampls,
                                  desired_ripple,
                                  Fs);

    return gr::filter::pm_remez(
               remez_params.order + nextra_taps,
               remez_params.bands,
               remez_params.ampls,
               remez_params.error_weight,
               "bandpass");
}

std::vector<gr_complex> optfir_complex_band_pass(
    double gain,
    double Fs,
    double freq_sb1,
    double freq_pb1,
    double freq_pb2,
    double freq_sb2,
    double passband_ripple_db,
    double stopband_atten_db,
    size_t nextra_taps)
{
    const auto center_freq = (freq_pb2 + freq_pb1) / 2.0;
    const auto lp_pb = (freq_pb2 - center_freq) / 1.0;
    const auto lp_sb = freq_sb2 - center_freq;
    const auto lptaps = optfir_low_pass(
                            gain,
                            Fs,
                            lp_pb,
                            lp_sb,
                            passband_ripple_db,
                            stopband_atten_db,
                            nextra_taps);

    std::vector<gr_complex> spinner;
    for(size_t i = 0; i < lptaps.size(); ++i)
    {
        spinner.emplace_back(std::exp(gr_complex(0.0, 2.0 * M_PI * center_freq / Fs * i)));
    }

    std::vector<gr_complex> taps;
    for(size_t i = 0; i < lptaps.size(); ++i)
    {
        taps.emplace_back(spinner[i] * static_cast<float>(lptaps[i]));
    }

    return taps;
}

std::vector<double> optfir_band_reject(
    double gain,
    double Fs,
    double freq_pb1,
    double freq_sb1,
    double freq_sb2,
    double freq_pb2,
    double passband_ripple_db,
    double stopband_atten_db,
    size_t nextra_taps)
{
    const auto passband_dev = passband_ripple_to_dev(passband_ripple_db);
    const auto stopband_dev = stopband_atten_to_dev(stopband_atten_db);
    const auto desired_ampls = std::vector<double>{gain, 0.0, gain};
    const auto desired_freqs = std::vector<double>{freq_pb1, freq_sb1, freq_sb2, freq_pb2};
    const auto desired_ripple = std::vector<double>{passband_dev, stopband_dev, passband_dev};
    const auto remez_params = remezord(
                                  desired_freqs,
                                  desired_ampls,
                                  desired_ripple,
                                  Fs);

    // Make sure we use an odd number of taps.
    if((remez_params.order + nextra_taps) % 2) ++nextra_taps;

    return gr::filter::pm_remez(
               remez_params.order + nextra_taps,
               remez_params.bands,
               remez_params.ampls,
               remez_params.error_weight,
               "bandpass");
}

std::vector<double> optfir_high_pass(
    double gain,
    double Fs,
    double freq1,
    double freq2,
    double passband_ripple_db,
    double stopband_atten_db,
    size_t nextra_taps)
{
    const auto passband_dev = passband_ripple_to_dev(passband_ripple_db);
    const auto stopband_dev = stopband_atten_to_dev(stopband_atten_db);
    const auto desired_ampls = std::vector<double>{0.0, 1.0};
    const auto remez_params = remezord(
                                  std::vector<double>{freq1, freq2},
                                  desired_ampls,
                                  std::vector<double>{stopband_dev, passband_dev},
                                  Fs);

    // Make sure we use an odd number of taps.
    if((remez_params.order + nextra_taps) % 2) ++nextra_taps;

    return gr::filter::pm_remez(
               remez_params.order + nextra_taps,
               remez_params.bands,
               remez_params.ampls,
               remez_params.error_weight,
               "bandpass");
}
