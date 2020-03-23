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

/*
 * Ported from: gr-channels/python/quantizer.py
 */

static constexpr int vlen = 1;
static constexpr float scale = 1.0f;

static float convertBits(size_t bits)
{
    return std::pow(2.0f, static_cast<float>(bits)-1);
}

class quantizer: public Pothos::Topology
{
public:
    static Pothos::Topology* make(size_t bits)
    {
        return new quantizer(bits);
    }

    quantizer(size_t bits):
        Pothos::Topology(),
        d_short_to_float0(Pothos::BlockRegistry::make("/gr/blocks/short_to_float", vlen, scale)),
        d_float_to_short0(Pothos::BlockRegistry::make("/gr/blocks/float_to_short", vlen, scale)),
        d_multiply_const_ff0(Pothos::BlockRegistry::make("/gr/blocks/multiply_const", "multiply_const_ff", convertBits(bits), vlen)),
        d_multiply_const_ff1(Pothos::BlockRegistry::make("/gr/blocks/multiply_const", "multiply_const_ff", 1.0f/convertBits(bits), vlen)),
        d_triggered_signal(Pothos::BlockRegistry::make("/blocks/triggered_signal")),
        d_bits_constant_source(Pothos::BlockRegistry::make("/blocks/constant_source", Pothos::DType(typeid(size_t)))),
        d_bits_eval0(Pothos::BlockRegistry::make("/blocks/evaluator", std::vector<std::string>{"bits"})),
        d_bits_eval1(Pothos::BlockRegistry::make("/blocks/evaluator", std::vector<std::string>{"bits"}))
    {
        // Set up the topology.
        this->connect(this, 0, d_multiply_const_ff0, 0);
        this->connect(d_multiply_const_ff0, 0, d_float_to_short0, 0);
        this->connect(d_float_to_short0, 0, d_short_to_float0, 0);
        this->connect(d_short_to_float0, 0, d_multiply_const_ff1, 0);
        this->connect(d_multiply_const_ff1, 0, this, 0);

        // Add calls.
        this->registerCall(this, POTHOS_FCN_TUPLE(quantizer, bits));
        this->registerCall(this, POTHOS_FCN_TUPLE(quantizer, set_bits));
        this->connect(this, "set_bits", d_bits_constant_source, "setConstant");

        // Set up evaluator blocks.
        d_bits_eval0.call("setExpression", "pow(2,bits-1)");
        this->connect(d_bits_constant_source, "constantChanged", d_bits_eval0, "setBits");
        this->connect(d_bits_eval0, "triggered", d_multiply_const_ff0, "set_k");

        d_bits_eval1.call("setExpression", "1/pow(2,bits-1)");
        this->connect(d_bits_constant_source, "constantChanged", d_bits_eval1, "setBits");
        this->connect(d_bits_eval1, "triggered", d_multiply_const_ff1, "set_k");

        d_bits_constant_source.call("setConstant", bits);
    }

    size_t bits() const
    {
        return d_bits_constant_source.call("getConstant");
    }

    // This is needed to connect to internal blocks.
    void set_bits(size_t) {}

private:
    Pothos::Proxy d_short_to_float0;
    Pothos::Proxy d_float_to_short0;

    // These were vff for no reason.
    Pothos::Proxy d_multiply_const_ff0;
    Pothos::Proxy d_multiply_const_ff1;

    Pothos::Proxy d_triggered_signal;
    Pothos::Proxy d_bits_constant_source;
    Pothos::Proxy d_bits_eval0;
    Pothos::Proxy d_bits_eval1;
};

/***********************************************************************
 * |PothosDoc Quantizer
 *
 * |category /GNURadio/Digital
 * |keywords rf bits
 *
 * |param bits[Bits] The number of bits to compress the signal into
 * |widget SpinBox(minimum=2,maximum=16)
 * |default 16
 * |preview enable
 *
 * |factory /gr/channels/quantizer(bits)
 * |initializer set_bits(bits)
 **********************************************************************/
static Pothos::BlockRegistry registerPhaseBal(
    "/gr/channels/quantizer",
    Pothos::Callable(&quantizer::make));
