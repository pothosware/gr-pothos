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
 * Ported from: gr-channels/python/distortion_3_gen.py
 */

static constexpr size_t vlen = 1;

class distortion_3_gen: public Pothos::Topology
{
public:
    static Pothos::Topology* make(const gr_complex& beta)
    {
        return new distortion_3_gen(beta);
    }
    
    distortion_3_gen(const gr_complex& beta):
        Pothos::Topology(),
        d_null_source0(Pothos::BlockRegistry::make("/gr/blocks/null_source", Pothos::DType(typeid(float)))),
        d_multiply_cc0(Pothos::BlockRegistry::make("/gr/blocks/multiply", "multiply_cc", vlen)),
        d_multiply_const_cc0(Pothos::BlockRegistry::make("/gr/blocks/multiply_const", "multiply_const_cc", beta, vlen)),
        d_float_to_complex0(Pothos::BlockRegistry::make("/gr/blocks/float_to_complex", vlen)),
        d_complex_to_mag_squared0(Pothos::BlockRegistry::make("/gr/blocks/complex_to_mag_squared", vlen)),
        d_add_cc0(Pothos::BlockRegistry::make("/gr/blocks/add", "add_cc", vlen))
    {
        // Assemble flowgraph
        this->connect(d_float_to_complex0, 0, d_multiply_cc0, 1);
        this->connect(d_null_source0, 0, d_float_to_complex0, 1);
        this->connect(d_complex_to_mag_squared0, 0, d_float_to_complex0, 0);
        this->connect(d_multiply_const_cc0, 0, d_add_cc0, 1);
        this->connect(d_multiply_cc0, 0, d_multiply_const_cc0, 0);
        this->connect(this, 0, d_complex_to_mag_squared0, 0);
        this->connect(this, 0, d_multiply_cc0, 0);
        this->connect(this, 0, d_add_cc0, 0);
        this->connect(d_add_cc0, 0, this, 0);

        // Add calls or connect to internal calls
        this->registerCall(this, POTHOS_FCN_TUPLE(distortion_3_gen, beta));
        this->registerCall(this, POTHOS_FCN_TUPLE(distortion_3_gen, set_beta));
        this->connect(this, "set_beta", d_multiply_const_cc0, "set_k");
        this->connect(this, "probe_beta", d_multiply_const_cc0, "probe_k");
        this->connect(d_multiply_const_cc0, "k_triggered", this, "beta_triggered");
    }

    gr_complex beta() const
    {
        return d_multiply_const_cc0.call("k");
    }

    // Put this here so we can connect to the internal block.
    void set_beta(const gr_complex& /*beta*/) {}

private:
    Pothos::Proxy d_null_source0;
    Pothos::Proxy d_multiply_cc0;

    // This was multiply_const_vcc in the original flowgraph for no reason. We're
    // changing it here to make connecting the topology to the block easier.
    Pothos::Proxy d_multiply_const_cc0;

    Pothos::Proxy d_float_to_complex0;
    Pothos::Proxy d_complex_to_mag_squared0;
    Pothos::Proxy d_add_cc0;
};

/***********************************************************************
 * |PothosDoc Third-Order Distortion
 *
 * Introduces third-order distortion to the input signal.
 *
 * |category /GNURadio/Impairments
 * |keywords rf beta
 *
 * |param beta[Beta] Distortion multiplier
 * |widget LineEdit()
 * |default 1+0i
 * |preview enable
 *
 * |factory /gr/channels/distortion_3_gen(beta)
 * |setter set_beta(beta)
 **********************************************************************/
static Pothos::BlockRegistry registerDistortion3Gen(
    "/gr/channels/distortion_3_gen",
    Pothos::Callable(&distortion_3_gen::make));
