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

#include <cmath>
#include <iostream>
#include <string>

/*
 * Ported from: gr-channels/python/iqbal_gen.py
 */

static constexpr size_t vlen = 1;

static float convertMag(float mag)
{
    return std::pow(10.0f, (mag / 20.0f));
}

static float convertPhaseCos(float phase)
{
    return std::cos(phase * M_PI / 180.0f);
}

static float convertPhaseSin(float phase)
{
    return std::sin(phase * M_PI / 180.0f);
}

class iqbal_gen: public Pothos::Topology
{
public:
    static Pothos::Topology* make(const std::string& mode)
    {
        return new iqbal_gen(mode);
    }
    
    iqbal_gen(const std::string& mode):
        Pothos::Topology(),
        d_mag(Pothos::BlockRegistry::make("/gr/blocks/multiply_const", "multiply_const_ff", convertMag(0.0f), vlen)),
        d_sin_phase(Pothos::BlockRegistry::make("/gr/blocks/multiply_const", "multiply_const_ff", convertPhaseSin(0.0f), vlen)),
        d_cos_phase(Pothos::BlockRegistry::make("/gr/blocks/multiply_const", "multiply_const_ff", convertPhaseCos(0.0f), vlen)),
        d_f2c(Pothos::BlockRegistry::make("/gr/blocks/float_to_complex", vlen)),
        d_c2f(Pothos::BlockRegistry::make("/gr/blocks/complex_to_float", vlen)),
        d_adder(Pothos::BlockRegistry::make("/gr/blocks/add", "add_ff", vlen)),
        d_mag_constant_source(Pothos::BlockRegistry::make("/blocks/constant_source", Pothos::DType(typeid(float)))),
        d_phase_constant_source(Pothos::BlockRegistry::make("/blocks/constant_source", Pothos::DType(typeid(float)))),
        d_mag_eval(Pothos::BlockRegistry::make("/blocks/evaluator", std::vector<std::string>{"x"})),
        d_sin_phase_eval(Pothos::BlockRegistry::make("/blocks/evaluator", std::vector<std::string>{"x"})),
        d_cos_phase_eval(Pothos::BlockRegistry::make("/blocks/evaluator", std::vector<std::string>{"x"}))
    {
        // Assemble flowgraph based on mode
        if(mode == "TRANSMITTER")
        {
            this->connect(this, 0, d_c2f, 0);
            this->connect(d_c2f, 0, d_mag, 0);
            this->connect(d_mag, 0, d_cos_phase, 0);
            this->connect(d_cos_phase, 0, d_f2c, 0);
            this->connect(d_mag, 0, d_sin_phase, 0);
            this->connect(d_sin_phase, 0, d_adder, 0);
            this->connect(d_c2f, 1, d_adder, 1);
            this->connect(d_adder, 0, d_f2c, 1);
            this->connect(d_f2c, 0, this, 0);
        }
        else if(mode == "RECEIVER")
        {
            this->connect(this, 0, d_c2f, 0);
            this->connect(d_c2f, 0, d_cos_phase, 0);
            this->connect(d_cos_phase, 0, d_adder, 0);
            this->connect(d_c2f, 0, d_sin_phase, 0);
            this->connect(d_sin_phase, 0, d_adder, 1);
            this->connect(d_adder, 0, d_mag, 0);
            this->connect(d_mag, 0, d_f2c, 0);
            this->connect(d_c2f, 0, d_f2c, 1);
            this->connect(d_f2c, 0, this, 0);
        }
        else
        {
            throw Pothos::InvalidArgumentException("iq_bal_gen", "Invalid mode "+mode);
        }

        // Add calls
        this->registerCall(this, POTHOS_FCN_TUPLE(iqbal_gen, magnitude));
        this->registerCall(this, POTHOS_FCN_TUPLE(iqbal_gen, set_magnitude));
        this->registerCall(this, POTHOS_FCN_TUPLE(iqbal_gen, phase));
        this->registerCall(this, POTHOS_FCN_TUPLE(iqbal_gen, set_phase));

        // Connections to emulate probes
        this->connect(this, "set_magnitude", d_mag_constant_source, "setConstant");
        this->connect(this, "probe_magnitude", d_mag_constant_source, "probeConstant");
        this->connect(d_mag_constant_source, "constantTriggered", this, "magnitude_triggered");
        this->connect(d_mag_constant_source, "constantChanged", this, "magnitude_changed");

        this->connect(this, "set_phase", d_phase_constant_source, "setConstant");
        this->connect(this, "probe_phase", d_phase_constant_source, "probeConstant");
        this->connect(d_phase_constant_source, "constantTriggered", this, "phase_triggered");
        this->connect(d_phase_constant_source, "constantChanged", this, "phase_changed");

        // Note: muparserx has a global pi constant
        set_up_probe_hack(d_mag_constant_source, d_mag_eval, d_mag, "pow(10.0,(x/20.0))");
        set_up_probe_hack(d_phase_constant_source, d_sin_phase_eval, d_sin_phase, "sin(x*pi/180.0)");
        set_up_probe_hack(d_phase_constant_source, d_cos_phase_eval, d_cos_phase, "cos(x*pi/180.0)");
    }

    float magnitude() const
    {
        return d_mag_constant_source.call("constant");
    }

    // We need this here for block connections.
    void set_magnitude(float) {}

    float phase() const
    {
        return d_phase_constant_source.call("constant");
    }

    // We need this here for block connections.
    void set_phase(float) {}

private:
    // These were vff for no reason.
    Pothos::Proxy d_mag;
    Pothos::Proxy d_sin_phase;
    Pothos::Proxy d_cos_phase;

    Pothos::Proxy d_f2c;
    Pothos::Proxy d_c2f;
    Pothos::Proxy d_adder;

    // Terrible hacky way to fake probes in a topology when the
    // value needs extra processing before being passed into an
    // internal block.
    Pothos::Proxy d_mag_constant_source;
    Pothos::Proxy d_phase_constant_source;
    Pothos::Proxy d_mag_eval;
    Pothos::Proxy d_sin_phase_eval;
    Pothos::Proxy d_cos_phase_eval;

    void set_up_probe_hack(
        const Pothos::Proxy& constant_source,
        const Pothos::Proxy& eval,
        const Pothos::Proxy& multiply_const_ff,
        const std::string& expression)
    {
        eval.call("setExpression", expression);

        this->connect(
            constant_source, "constantChanged",
            eval, "setX");
        this->connect(
            eval, "triggered",
            multiply_const_ff, "set_k");
    }
};

// TODO: add mag+phase setters

/***********************************************************************
 * |PothosDoc IQ Imbalance Generator
 *
 * Introduces IQ imbalance to the input signal.
 *
 * |category /GNURadio/Impairments
 * |keywords rf balance magnitude phase
 *
 * |param mode[Mode] Transmitter or receiver mode
 * |widget ComboBox(editable=false)
 * |option [Transmitter] "TRANSMITTER"
 * |option [Receiver] "RECEIVER"
 * |default "TRANSMITTER"
 * |preview enable
 *
 * |factory /gr/channels/iqbal_gen(mode)
 **********************************************************************/
static Pothos::BlockRegistry registerIqBal(
    "/gr/channels/iqbal_gen",
    Pothos::Callable(&iqbal_gen::make));
