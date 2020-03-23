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

/*
 * Ported from: gr-channels/python/amp_bal.py
 */

static constexpr size_t vlen = 1;

class amp_bal: public Pothos::Topology
{
public:
    static Pothos::Topology* make(double alpha)
    {
        return new amp_bal(alpha);
    }
    
    amp_bal(double alpha):
        Pothos::Topology(),
        d_alpha(alpha),
        d_rms_ff0(Pothos::BlockRegistry::make("/gr/blocks/rms", "rms_ff", d_alpha)),
        d_rms_ff1(Pothos::BlockRegistry::make("/gr/blocks/rms", "rms_ff", d_alpha)),
        d_multiply_ff(Pothos::BlockRegistry::make("/gr/blocks/multiply", "multiply_ff", vlen)),
        d_float_to_complex(Pothos::BlockRegistry::make("/gr/blocks/float_to_complex", vlen)),
        d_divide_ff(Pothos::BlockRegistry::make("/gr/blocks/divide", "divide_ff", vlen)),
        d_complex_to_float(Pothos::BlockRegistry::make("/gr/blocks/complex_to_float", vlen))
    {
        this->connect(d_float_to_complex, 0, this, 0);
        this->connect(this, 0, d_complex_to_float, 0);
        this->connect(d_complex_to_float, 0, d_rms_ff0, 0);
        this->connect(d_complex_to_float, 1, d_rms_ff1, 0);
        this->connect(d_rms_ff0, 0, d_divide_ff, 0);
        this->connect(d_rms_ff1, 0, d_divide_ff, 1);
        this->connect(d_complex_to_float, 0, d_float_to_complex, 0);
        this->connect(d_complex_to_float, 1, d_multiply_ff, 1);
        this->connect(d_divide_ff, 0, d_multiply_ff, 0);
        this->connect(d_multiply_ff, 0, d_float_to_complex, 1);

        this->registerCall(this, POTHOS_FCN_TUPLE(amp_bal, alpha));
        this->registerCall(this, POTHOS_FCN_TUPLE(amp_bal, set_alpha));
    }

    double alpha() const
    {
        return d_alpha;
    }

    void set_alpha(double alpha)
    {
        d_alpha = alpha;
        d_rms_ff0.call("set_alpha", alpha);
        d_rms_ff1.call("set_alpha", alpha);
    }

private:
    double d_alpha;

    Pothos::Proxy d_rms_ff0;
    Pothos::Proxy d_rms_ff1;
    Pothos::Proxy d_multiply_ff;
    Pothos::Proxy d_float_to_complex;
    Pothos::Proxy d_divide_ff;
    Pothos::Proxy d_complex_to_float;
};

/***********************************************************************
 * |PothosDoc Amplitude Balance Correction
 *
 * Restores IQ amplitude balance.
 *
 * |category /GNURadio/Impairments
 * |keywords rf iq rms alpha
 *
 * |param alpha[Alpha] Gain for running average filter
 * |widget DoubleSpinBox(minimum=0,step=1e-6,decimals=6)
 * |default 1e-4
 * |units Hz
 * |preview enable
 *
 * |factory /gr/channels/amp_bal(alpha)
 * |setter set_alpha(alpha)
 **********************************************************************/
static Pothos::BlockRegistry registerAmpBal(
    "/gr/channels/amp_bal",
    Pothos::Callable(&amp_bal::make));
