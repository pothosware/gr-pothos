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

#pragma once

#include <gnuradio/gr_complex.h>

#include <vector>

//
// This is ported from GNU Radio's optimal FIR filter routines, which are
// implemented in Python.
//
// See: gr-filter/python/optfir.py
//

std::vector<double> optfir_low_pass(
    double gain,
    double Fs,
    double freq1,
    double freq2,
    double passband_ripple_db,
    double stopband_atten_db,
    size_t nextra_taps = 2);

std::vector<double> optfir_band_pass(
    double gain,
    double Fs,
    double freq_sb1,
    double freq_pb1,
    double freq_pb2,
    double freq_sb2,
    double passband_ripple_db,
    double stopband_atten_db,
    size_t nextra_taps = 2);

std::vector<gr_complex> optfir_complex_band_pass(
    double gain,
    double Fs,
    double freq_sb1,
    double freq_pb1,
    double freq_pb2,
    double freq_sb2,
    double passband_ripple_db,
    double stopband_atten_db,
    size_t nextra_taps = 2);

std::vector<double> optfir_band_reject(
    double gain,
    double Fs,
    double freq_pb1,
    double freq_sb1,
    double freq_sb2,
    double freq_pb2,
    double passband_ripple_db,
    double stopband_atten_db,
    size_t nextra_taps = 2);

std::vector<double> optfir_high_pass(
    double gain,
    double Fs,
    double freq1,
    double freq2,
    double passband_ripple_db,
    double stopband_atten_db,
    size_t nextra_taps = 2);
