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

#include <gnuradio/types.h>

/*
 * Ported from: gr-channels/python/phase_noise_gen.py
 */

static constexpr size_t vlen = 1;

class phase_noise_gen: public Pothos::Topology
{
public:
    static Pothos::Topology* make(float noise_mag, float alpha)
    {
        return new phase_noise_gen(noise_mag, alpha);
    }
    
    phase_noise_gen(float noise_mag, float alpha):
        Pothos::Topology(),
        d_single_pole_iir_filter_ff0(Pothos::BlockRegistry::make("/gr/filter/single_pole_iir_filter", "single_pole_iir_filter_ff", alpha, vlen)),
        d_transcendental0(Pothos::BlockRegistry::make("/gr/blocks/transcendental", "sin", "float")),
        d_transcendental1(Pothos::BlockRegistry::make("/gr/blocks/transcendental", "cos", "float")),
        d_multiply_cc0(Pothos::BlockRegistry::make("/gr/blocks/multiply", "multiply_cc", vlen)),
        d_float_to_complex0(Pothos::BlockRegistry::make("/gr/blocks/float_to_complex", vlen)),
        d_noise_source_f0(Pothos::BlockRegistry::make("/gr/analog/noise_source", "noise_source_f", "GR_GAUSSIAN", noise_mag, 42)),
        d_alpha_constant_source(Pothos::BlockRegistry::make("/blocks/constant_source", Pothos::DType(typeid(float))))
    {
        d_alpha_constant_source.call("setConstant", alpha);
        
        // Assemble flowgraph
        this->connect(d_float_to_complex0, 0, d_multiply_cc0, 1);
        this->connect(d_noise_source_f0, 0, d_single_pole_iir_filter_ff0, 0);
        this->connect(this, 0, d_multiply_cc0, 0);
        this->connect(d_multiply_cc0, 0, this, 0);
        this->connect(d_single_pole_iir_filter_ff0, 0, d_transcendental0, 0);
        this->connect(d_single_pole_iir_filter_ff0, 0, d_transcendental1, 0);
        this->connect(d_transcendental0, 0, d_float_to_complex0, 0);
        this->connect(d_transcendental1, 0, d_float_to_complex0, 1);

        // Add calls or connect to internal calls
        this->registerCall(this, POTHOS_FCN_TUPLE(phase_noise_gen, noise_mag));
        this->registerCall(this, POTHOS_FCN_TUPLE(phase_noise_gen, set_noise_mag));
        this->connect(this, "set_noise_mag", d_noise_source_f0, "set_amplitude");
        this->registerCall(this, POTHOS_FCN_TUPLE(phase_noise_gen, alpha));
        this->registerCall(this, POTHOS_FCN_TUPLE(phase_noise_gen, set_alpha));
        this->connect(this, "set_alpha", d_single_pole_iir_filter_ff0, "set_taps");
        this->connect(this, "set_alpha", d_alpha_constant_source, "setConstant");

        // Expose internal probes
        this->connect(this, "probe_alpha", d_alpha_constant_source, "probeConstant");
        this->connect(d_alpha_constant_source, "constantTriggered", this, "alpha_triggered");
        this->connect(this, "probe_noise_mag", d_noise_source_f0, "probe_amplitude");
        this->connect(d_noise_source_f0, "amplitude_triggered", this, "noise_mag_triggered");
    }

    float noise_mag() const
    {
        return d_noise_source_f0.call("amplitude");
    }

    // Put this here so we can connect to the internal block.
    void set_noise_mag(float) {}

    float alpha() const
    {
        return d_alpha_constant_source.call("constant");
    }

    // Put this here so we can connect to the internal blocks.
    void set_alpha(float) {}

private:
    Pothos::Proxy d_single_pole_iir_filter_ff0;
    Pothos::Proxy d_transcendental0;
    Pothos::Proxy d_transcendental1;
    Pothos::Proxy d_multiply_cc0;
    Pothos::Proxy d_float_to_complex0;
    Pothos::Proxy d_noise_source_f0;

    // We need to use this block to store alpha since we can't query
    // it from the block that uses it. This lets us emulate a probe
    // in a topology.
    Pothos::Proxy d_alpha_constant_source;
};

/***********************************************************************
 * |PothosDoc Phase Noise Generator
 *
 * Introduces phase noise to the input signal.
 *
 * |category /GNURadio/Impairments
 * |keywords rf alpha
 *
 * |param noise_mag[Noise Magnitude] Noise source magnitude
 * |widget DoubleSpinBox(minimum=-50,maximum=50)
 * |default 0.0
 * |preview enable
 *
 * |param alpha[Alpha]
 * |widget DoubleSpinBox(minimum=-50,maximum=50)
 * |default 0.1
 * |preview enable
 *
 * |factory /gr/channels/phase_noise_gen(noise_mag,alpha)
 * |setter set_noise_mag(noise_mag)
 * |setter set_alpha(alpha)
 **********************************************************************/
static Pothos::BlockRegistry registerPhaseNoiseGen(
    "/gr/channels/phase_noise_gen",
    Pothos::Callable(&phase_noise_gen::make));
