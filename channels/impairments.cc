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
#include <vector>

/*
 * Ported from: gr-channels/python/impairments.py
 */

static constexpr size_t vlen = 1;

class impairments: public Pothos::Topology
{
public:
    static Pothos::Topology* make(
        float noise_mag,
        float iqbal_mag,
        float iqbal_phase,
        float i_offset,
        float q_offset,
        float freq_offset,
        const gr_complex& beta,
        const gr_complex& gamma)
    {
        return new impairments(noise_mag, iqbal_mag, iqbal_phase, i_offset, q_offset, freq_offset, beta, gamma);
    }
  
    impairments(float noise_mag,
                float iqbal_mag,
                float iqbal_phase,
                float i_offset,
                float q_offset,
                float freq_offset,
                const gr_complex& beta,
                const gr_complex& gamma):
        Pothos::Topology(),
        d_phase_noise_gen0(Pothos::BlockRegistry::make("/gr/channels/phase_noise_gen", 0.0f, 0.01)),
        d_iqbal_gen0(Pothos::BlockRegistry::make("/gr/channels/iqbal_gen", "RECEIVER")),
        d_distortion_2_gen0(Pothos::BlockRegistry::make("/gr/channels/distortion_2_gen", gamma)),
        d_distortion_3_gen0(Pothos::BlockRegistry::make("/gr/channels/distortion_3_gen", beta)),
        d_multiply_cc0(Pothos::BlockRegistry::make("/gr/blocks/multiply", "multiply_cc", vlen)),
        d_multiply_cc1(Pothos::BlockRegistry::make("/gr/blocks/multiply", "multiply_cc", vlen)),
        d_conjugate_cc0(Pothos::BlockRegistry::make("/gr/blocks/conjugate_cc")),
        d_add_const_cc0(Pothos::BlockRegistry::make("/gr/blocks/add_const", "add_const_cc", gr_complex(i_offset, q_offset))),
        d_sig_source_c0(Pothos::BlockRegistry::make("/gr/analog/sig_source", "sig_source_c", 1.0f, "GR_COS_WAVE", freq_offset, 1, 0)),
        d_noise_mag_constant_source(Pothos::BlockRegistry::make("/blocks/constant_source", Pothos::DType(typeid(float)))),
        d_i_offset_constant_source(Pothos::BlockRegistry::make("/blocks/constant_source", Pothos::DType(typeid(float)))),
        d_q_offset_constant_source(Pothos::BlockRegistry::make("/blocks/constant_source", Pothos::DType(typeid(float)))),
        d_noise_mag_eval(Pothos::BlockRegistry::make("/blocks/evaluator", std::vector<std::string>{"mag"})),
        d_iq_offset_eval(Pothos::BlockRegistry::make("/blocks/evaluator", std::vector<std::string>{"iOffset","qOffset"}))
    {
        this->connect(d_phase_noise_gen0, 0, d_distortion_3_gen0, 0);
        this->connect(d_multiply_cc0, 0, this, 0);
        this->connect(d_add_const_cc0, 0, d_multiply_cc0, 1);
        this->connect(d_sig_source_c0, 0, d_multiply_cc0, 0);
        this->connect(d_multiply_cc1, 0, d_phase_noise_gen0, 0);
        this->connect(d_sig_source_c0, 0, d_conjugate_cc0, 0);
        this->connect(this, 0, d_multiply_cc1, 1);
        this->connect(d_conjugate_cc0, 0, d_multiply_cc1, 0);
        this->connect(d_iqbal_gen0, 0, d_add_const_cc0, 0);
        this->connect(d_distortion_3_gen0, 0, d_distortion_2_gen0, 0);
        this->connect(d_distortion_2_gen0, 0, d_iqbal_gen0, 0);
        
        // Add calls or connect to internal calls
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, noise_mag));
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, set_noise_mag));
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, iqbal_mag));
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, set_iqbal_mag));
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, iqbal_phase));
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, set_iqbal_phase));
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, i_offset));
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, set_i_offset));
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, q_offset));
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, set_q_offset));
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, freq_offset));
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, set_freq_offset));
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, beta));
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, set_beta));
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, gamma));
        this->registerCall(this, POTHOS_FCN_TUPLE(impairments, set_gamma));

        // Setter connections
        this->connect(this, "set_noise_mag", d_noise_mag_constant_source, "setConstant");
        this->connect(this, "set_iqbal_mag", d_iqbal_gen0, "set_magnitude");
        this->connect(this, "set_iqbal_phase", d_iqbal_gen0, "set_phase");
        this->connect(this, "set_i_offset", d_i_offset_constant_source, "setConstant");
        this->connect(this, "set_q_offset", d_q_offset_constant_source, "setConstant");
        this->connect(this, "set_freq_offset", d_sig_source_c0, "set_frequency");
        this->connect(this, "set_beta", d_distortion_3_gen0, "set_beta");
        this->connect(this, "set_gamma", d_distortion_2_gen0, "set_beta");

        // Normal probe connections
        this->connect(this, "probe_noise_mag", d_noise_mag_constant_source, "probeConstant");
        this->connect(this, "probe_iqbal_mag", d_iqbal_gen0, "probe_magnitude");
        this->connect(this, "probe_iqbal_phase", d_iqbal_gen0, "probe_phase");
        this->connect(this, "probe_i_offset", d_i_offset_constant_source, "probeConstant");
        this->connect(this, "probe_q_offset", d_q_offset_constant_source, "probeConstant");
        this->connect(this, "probe_freq_offset", d_sig_source_c0, "probe_frequency");
        this->connect(this, "probe_beta", d_distortion_3_gen0, "probe_beta");
        this->connect(this, "probe_gamma", d_distortion_2_gen0, "probe_beta");
        this->connect(d_noise_mag_constant_source, "constantTriggered", this, "noise_mag_triggered");
        this->connect(d_iqbal_gen0, "magnitude_triggered", this, "iqbal_mag_triggered");
        this->connect(d_iqbal_gen0, "phase_triggered", this, "iqbal_phase_triggered");
        this->connect(d_i_offset_constant_source, "constantTriggered", this, "i_offset_triggered");
        this->connect(d_q_offset_constant_source, "constantTriggered", this, "q_offset_triggered");
        this->connect(d_sig_source_c0, "frequency_triggered", this, "freq_offset_triggered");
        this->connect(d_distortion_3_gen0, "beta_triggered", this, "beta_triggered");
        this->connect(d_distortion_2_gen0, "beta_triggered", this, "gamma_triggered");

        // Set up evaluator blocks
        d_noise_mag_eval.call("setExpression", "pow(10.0,mag/20.0)");
        this->connect(d_noise_mag_constant_source, "constantChanged", d_noise_mag_eval, "setMag");
        this->connect(d_noise_mag_eval, "triggered", d_phase_noise_gen0, "set_noise_mag");
        
        d_iq_offset_eval.call("setExpression", "complex(iOffset,qOffset)");
        this->connect(d_i_offset_constant_source, "constantChanged", d_iq_offset_eval, "setIOffset");
        this->connect(d_q_offset_constant_source, "constantChanged", d_iq_offset_eval, "setQOffset");
        this->connect(d_iq_offset_eval, "triggered", d_add_const_cc0, "set_k");

        // Call non-constructor setters after probes are set
        d_noise_mag_constant_source.call("setConstant", noise_mag);
        d_i_offset_constant_source.call("setConstant", i_offset);
        d_q_offset_constant_source.call("setConstant", q_offset);
        d_iqbal_gen0.call("set_magnitude", iqbal_mag);
        d_iqbal_gen0.call("set_phase", iqbal_phase);
    }

    float noise_mag() const
    {
        return d_noise_mag_constant_source.call("getConstant");
    }

    float iqbal_mag() const
    {
        return d_iqbal_gen0.call("magnitude");
    }

    float iqbal_phase() const
    {
        return d_iqbal_gen0.call("phase");
    }

    float i_offset() const
    {
        return d_i_offset_constant_source.call("getConstant");
    }

    float q_offset() const
    {
        return d_q_offset_constant_source.call("getConstant");
    }

    float freq_offset() const
    {
        return d_sig_source_c0.call("frequency");
    }

    gr_complex beta() const
    {
        return d_distortion_3_gen0.call("beta");
    }

    gr_complex gamma() const
    {
        return d_distortion_2_gen0.call("beta");
    }

    // These need to be here to connect to internal blocks.
    void set_noise_mag(float) {}
    void set_iqbal_mag(float) {}
    void set_iqbal_phase(float) {}
    void set_i_offset(float) {}
    void set_q_offset(float) {}
    void set_freq_offset(float) {}
    void set_beta(const gr_complex& beta) {}
    void set_gamma(const gr_complex& gamma) {}

private:

    Pothos::Proxy d_phase_noise_gen0;
    Pothos::Proxy d_iqbal_gen0;
    Pothos::Proxy d_distortion_2_gen0;
    Pothos::Proxy d_distortion_3_gen0;
    Pothos::Proxy d_multiply_cc0;
    Pothos::Proxy d_multiply_cc1;
    Pothos::Proxy d_conjugate_cc0;
    Pothos::Proxy d_add_const_cc0; // Was vcc for no reason
    Pothos::Proxy d_sig_source_c0;

    // Constant sources to abuse as variables
    Pothos::Proxy d_noise_mag_constant_source;
    Pothos::Proxy d_i_offset_constant_source;
    Pothos::Proxy d_q_offset_constant_source;

    // Eval blocks to process given constants without storing them
    Pothos::Proxy d_noise_mag_eval;
    Pothos::Proxy d_iq_offset_eval;
};

/***********************************************************************
 * |PothosDoc Radio Impairments Model
 *
 * Emulate various impairments on the given input signal. This block
 * applies the following:
 * <ul>
 * <li>IQ imbalance</li>
 * <li>Phase noise</li>
 * <li>Second-order distortion</li>
 * <li>Third-order distortion</li>
 * </ul>
 *
 * |category /GNURadio/Impairments
 * |category /GNURadio/Channel Models
 * |keywords rf iq imbalance phase noise distortion
 *
 * |param noise_mag[Phase Noise Magnitude]
 * |widget DoubleSpinBox(minimum=-100,maximum=0,step=1)
 * |default 0
 * |preview enable
 *
 * |param iqbal_mag[IQ Magnitude Imbalance]
 * |widget DoubleSpinBox(minimum=0,maximum=10,step=0.1,decimals=1)
 * |default 0
 * |preview enable
 *
 * |param iqbal_phase[IQ Phase Imbalance]
 * |widget DoubleSpinBox(minimum=0,maximum=45,step=0.1,decimals=1)
 * |default 0
 * |preview enable
 *
 * |param i_offset[Inphase Offset]
 * |widget DoubleSpinBox(minimum=-1,maximum=1,step=0.001,decimals=3)
 * |default 0
 * |preview enable
 *
 * |param q_offset[Quadrature Offset]
 * |widget DoubleSpinBox(minimum=-1,maximum=1,step=0.001,decimals=3)
 * |default 0
 * |preview enable
 *
 * |param freq_offset[Freq Offset]
 * |widget DoubleSpinBox(minimum=-0.5,maximum=0.5,step=0.001,decimals=3)
 * |default 0.0
 * |preview enable
 *
 * |param gamma[Second Order Distortion] Second-order distortion multiplier
 * |widget DoubleSpinBox(minimum=-0.1,maximum=0,step=0.001,decimals=3)
 * |default 0.0
 * |preview enable
 *
 * |param beta[Third Order Distortion] Third-order distortion multiplier
 * |widget DoubleSpinBox(minimum=-0.1,maximum=0,step=0.001,decimals=3)
 * |default 0.0
 * |preview enable
 *
 * |factory /gr/channels/impairments(noise_mag,iqbal_mag,iqbal_phase,i_offset,q_offset,freq_offset,beta,gamma)
 **********************************************************************/
static Pothos::BlockRegistry registerImpairments(
    "/gr/channels/impairments",
    Pothos::Callable(&impairments::make));
