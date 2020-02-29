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

#include <gnuradio/gr_complex.h>

/*
 * Ported from: gr-channels/python/phase_bal.py
 */

static constexpr size_t vlen = 1;

class phase_bal: public Pothos::Topology
{
public:
    static Pothos::Topology* make(double alpha)
    {
        return new phase_bal(alpha);
    }

    phase_bal(double alpha):
        Pothos::Topology(),
        d_add_const_cc0(Pothos::BlockRegistry::make("/gr/blocks/add_const", "add_const_cc", gr_complex(0.0,0.0))),
        d_single_pole_iir_filter_ff0(Pothos::BlockRegistry::make("/gr/filter/single_pole_iir_filter", "single_pole_iir_filter_ff", alpha, vlen)),
        d_sub_ff0(Pothos::BlockRegistry::make("/gr/blocks/sub", "sub_ff", vlen)),
        d_sub_ff1(Pothos::BlockRegistry::make("/gr/blocks/sub", "sub_ff", vlen)),
        d_multiply_ff0(Pothos::BlockRegistry::make("/gr/blocks/multiply", "multiply_ff", vlen)),
        d_multiply_ff1(Pothos::BlockRegistry::make("/gr/blocks/multiply", "multiply_ff", vlen)),
        d_multiply_ff2(Pothos::BlockRegistry::make("/gr/blocks/multiply", "multiply_ff", vlen)),
        d_multiply_const_vff0(Pothos::BlockRegistry::make("/gr/blocks/multiply_const", "multiply_const_vff", std::vector<float>{2.0f}, vlen)),
        d_float_to_complex0(Pothos::BlockRegistry::make("/gr/blocks/float_to_complex", vlen)),
        d_divide_ff0(Pothos::BlockRegistry::make("/gr/blocks/divide", "divide_ff", vlen)),
        d_complex_to_mag_squared0(Pothos::BlockRegistry::make("/gr/blocks/complex_to_mag_squared", vlen)),
        d_complex_to_float0(Pothos::BlockRegistry::make("/gr/blocks/complex_to_float", vlen))
    {
        // Note: this add_const_cc wasn't in the original block, but without it,
        // PothosFlow evaluates this topology's input type as int16[4].
        this->connect(this, 0, d_add_const_cc0, 0);
        this->connect(d_add_const_cc0, 0, d_complex_to_float0, 0);
        this->connect(d_add_const_cc0, 0, d_complex_to_mag_squared0, 0);
        this->connect(d_float_to_complex0, 0, this, 0);

        this->connect(d_complex_to_float0, 0, d_multiply_ff0, 0);
        this->connect(d_complex_to_float0, 1, d_multiply_ff0, 1);
        this->connect(d_multiply_ff0, 0, d_divide_ff0, 0);
        this->connect(d_sub_ff0, 0, d_float_to_complex0, 1);
        this->connect(d_multiply_ff1, 0, d_sub_ff0, 1);
        this->connect(d_single_pole_iir_filter_ff0, 0, d_multiply_ff1, 1);
        this->connect(d_complex_to_float0, 0, d_multiply_ff1, 0);
        this->connect(d_multiply_ff2, 0, d_sub_ff1, 1);
        this->connect(d_complex_to_float0, 1, d_sub_ff0, 0);
        this->connect(d_sub_ff1, 0, d_float_to_complex0, 0);
        this->connect(d_complex_to_mag_squared0, 0, d_divide_ff0, 1);
        this->connect(d_complex_to_float0, 0, d_sub_ff1, 0);
        this->connect(d_divide_ff0, 0, d_multiply_const_vff0, 0);
        this->connect(d_multiply_const_vff0, 0, d_single_pole_iir_filter_ff0, 0);
        this->connect(d_single_pole_iir_filter_ff0, 0, d_multiply_ff2, 0);
        this->connect(d_complex_to_float0, 1, d_multiply_ff2, 1);

        this->registerCall(this, POTHOS_FCN_TUPLE(phase_bal, alpha));
        this->registerCall(this, POTHOS_FCN_TUPLE(phase_bal, set_alpha));
    }

    double alpha() const
    {
        return d_alpha;
    }

    void set_alpha(double alpha)
    {
        d_alpha = alpha;
    }

    Pothos::Object opaqueCallMethod(const std::string &name, const Pothos::Object *inputArgs, const size_t numArgs) const
    {
        // Try the topology first, then the other block with relevant calls.
        try
        {
            return Pothos::Topology::opaqueCallMethod(name, inputArgs, numArgs);
        }
        catch (const Pothos::BlockCallNotFound&){}

        return d_single_pole_iir_filter_ff0.call<Pothos::Block*>("getPointer")->opaqueCallMethod(name, inputArgs, numArgs);
    }

private:
    double d_alpha;

    Pothos::Proxy d_add_const_cc0;

    Pothos::Proxy d_single_pole_iir_filter_ff0;
    Pothos::Proxy d_sub_ff0;
    Pothos::Proxy d_sub_ff1;
    Pothos::Proxy d_multiply_ff0;
    Pothos::Proxy d_multiply_ff1;
    Pothos::Proxy d_multiply_ff2;
    Pothos::Proxy d_multiply_const_vff0;
    Pothos::Proxy d_float_to_complex0;
    Pothos::Proxy d_divide_ff0;
    Pothos::Proxy d_complex_to_mag_squared0;
    Pothos::Proxy d_complex_to_float0;
};

/***********************************************************************
 * |PothosDoc Phase Balance Correction
 *
 * Restores IQ phase balance.
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
 * |factory /gr/channels/phase_bal(alpha)
 * |initializer set_alpha(alpha)
 **********************************************************************/
static Pothos::BlockRegistry registerAmpBal(
    "/gr/channels/phase_bal",
    Pothos::Callable(&phase_bal::make));
