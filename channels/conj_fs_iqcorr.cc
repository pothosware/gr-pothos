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

#include <vector>

/*
 * Ported from: gr-channels/python/conj_fs_iqcorr.py
 */

static constexpr int decimation = 1;
static constexpr size_t vlen = 1;

class conj_fs_iqcorr: public Pothos::Topology
{
public:
    static Pothos::Topology* make(int delay, const std::vector<gr_complex>& taps)
    {
        return new conj_fs_iqcorr(delay, taps);
    }
    
    conj_fs_iqcorr(int delay, const std::vector<gr_complex>& taps):
        Pothos::Topology(),
        d_fir_filter_ccc(Pothos::BlockRegistry::make("/gr/filter/fir_filter", "fir_filter_ccc", decimation, taps)),
        d_delay(Pothos::BlockRegistry::make("/gr/blocks/delay", "complex_float32", delay)),
        d_conjugate_cc(Pothos::BlockRegistry::make("/gr/blocks/conjugate_cc")),
        d_add_cc(Pothos::BlockRegistry::make("/gr/blocks/add", "add_cc", vlen))
    {
        this->connect(d_add_cc, 0, this, 0);
        this->connect(this, 0, d_conjugate_cc, 0);
        this->connect(d_fir_filter_ccc, 0, d_add_cc, 1);
        this->connect(d_conjugate_cc, 0, d_fir_filter_ccc, 0);
        this->connect(this, 0, d_delay, 0);
        this->connect(d_delay, 0, d_add_cc, 0);

        this->registerCall(this, POTHOS_FCN_TUPLE(conj_fs_iqcorr, delay));
        this->connect(this, "set_delay", d_delay, "set_dly");
        this->connect(this, "probe_delay", d_delay, "probe_dly");
        this->connect(d_delay, "dly_triggered", this, "delay_triggered");

        this->registerCall(this, POTHOS_FCN_TUPLE(conj_fs_iqcorr, taps));
        this->connect(this, "set_taps", d_fir_filter_ccc, "set_taps");
        this->connect(this, "probe_taps", d_fir_filter_ccc, "probe_taps");
        this->connect(d_fir_filter_ccc, "taps_triggered", this, "taps_triggered");
    }

    int delay() const
    {
        return d_delay.call("dly");
    }

    std::vector<gr_complex> taps() const
    {
        return d_fir_filter_ccc.call("taps");
    }

    Pothos::Object opaqueCallMethod(const std::string &name, const Pothos::Object *inputArgs, const size_t numArgs) const
    {
        // Try the topology first, then the two other blocks with relevant calls.
        try
        {
            return Pothos::Topology::opaqueCallMethod(name, inputArgs, numArgs);
        }
        catch (const Pothos::BlockCallNotFound&){}
        try
        {
            return d_delay.call<Pothos::Block*>("getPointer")->opaqueCallMethod(name, inputArgs, numArgs);
        }
        catch (const Pothos::BlockCallNotFound&){}

        return d_fir_filter_ccc.call<Pothos::Block*>("getPointer")->opaqueCallMethod(name, inputArgs, numArgs);
    }

private:
    Pothos::Proxy d_fir_filter_ccc;
    Pothos::Proxy d_delay;
    Pothos::Proxy d_conjugate_cc;
    Pothos::Proxy d_add_cc;
};

/***********************************************************************
 * |PothosDoc IQ Balance Correction
 *
 * Frequency selective conjugate method IQ balance corrector.
 *
 * |category /GNURadio/Impairments
 * |keywords rf iq impairments
 *
 * |param delay[Delay] The number of samples to delay the stream.
 * |widget SpinBox(minimum=0)
 * |default 0
 * |preview enable
 *
 * |param taps[Taps] Complex filter taps.
 * |widget LineEdit()
 * |default []
 * |preview disable
 *
 * |factory /gr/channels/conj_fs_iqcorr(delay,taps)
 **********************************************************************/
static Pothos::BlockRegistry registerAmpBal(
    "/gr/channels/conj_fs_iqcorr",
    Pothos::Callable(&conj_fs_iqcorr::make));
