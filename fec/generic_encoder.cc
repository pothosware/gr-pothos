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

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/Managed.hpp>
#include <Pothos/Testing.hpp>

#include <Pothos/Util/TypeInfo.hpp>

#include <gnuradio/fec/cc_encoder.h>
#include <gnuradio/fec/ccsds_encoder.h>
#include <gnuradio/fec/dummy_encoder.h>
#include <gnuradio/fec/ldpc_encoder.h>
#include <gnuradio/fec/ldpc_par_mtrx_encoder.h>
#include <gnuradio/fec/repetition_encoder.h>
#include <gnuradio/fec/tpc_encoder.h>

#include <Poco/Path.h>

#include <string>

//
// TODOs:
//  * fec_mtrx_sptr
//  * Polar decoders
//  * Override logger
//

//
// Helper methods to get access the generic_encoder methods and fields
//

#define WRAP_ENCODER_FUNC(func,returnType) \
    static returnType generic_encoder_ ## func (const gr::fec::generic_encoder::sptr& encoder) \
    { \
        return encoder->func(); \
    }

WRAP_ENCODER_FUNC(rate, double)
WRAP_ENCODER_FUNC(get_input_size, int)
WRAP_ENCODER_FUNC(get_output_size, int)
WRAP_ENCODER_FUNC(get_input_conversion, std::string)
WRAP_ENCODER_FUNC(get_output_conversion, std::string)

static std::string generic_encoder_name(const gr::fec::generic_encoder::sptr& encoder)
{
    return encoder->d_name;
}

static bool generic_encoder_set_frame_size(
    const gr::fec::generic_encoder::sptr& encoder,
    unsigned int frame_size)
{
    return encoder->set_frame_size(frame_size);
}

// Register the factory functions for each generic_encoder implementation
// as constructors for the managed class.
static auto managedGenericEncoderSptr = Pothos::ManagedClass()
    .registerClass<gr::fec::generic_encoder::sptr>()
    .registerConstructor(&gr::fec::code::cc_encoder::make)
    .registerConstructor(&gr::fec::code::ccsds_encoder::make)
    .registerConstructor(&gr::fec::code::dummy_encoder::make)
    .registerConstructor(&gr::fec::ldpc_encoder::make)
    .registerConstructor(&gr::fec::code::ldpc_par_mtrx_encoder::make)
    .registerConstructor(&gr::fec::code::repetition_encoder::make)
    .registerConstructor(&gr::fec::tpc_encoder::make)
    .registerMethod("name", &generic_encoder_name)
    .registerMethod("rate", &generic_encoder_rate)
    .registerMethod("get_input_size", &generic_encoder_get_input_size)
    .registerMethod("get_output_size", &generic_encoder_get_output_size)
    .registerMethod("get_input_conversion", &generic_encoder_get_input_conversion)
    .registerMethod("get_output_conversion", &generic_encoder_get_output_conversion)
    .registerMethod("set_frame_size", &generic_encoder_set_frame_size)
    .commit("gr/fec/generic_encoder_sptr");

// Converter for easy conversion from evalulated ProxyVector
static gr::fec::generic_encoder::sptr proxyVectorToGrFECGenericEncoderSptr(const Pothos::ProxyVector &args)
{
    auto env = Pothos::ProxyEnvironment::make("managed");
    auto cls = env->findProxy("gr/fec/generic_encoder_sptr");
    auto encoder = cls.getHandle()->call("()", args.data(), args.size());
    return encoder;
}


pothos_static_block(registerProxyVectorToGrFECGenericEncoderSptr)
{
    Pothos::PluginRegistry::add(
        "/object/convert/containers/proxy_vec_to_gr_fec_generic_encoder_sptr",
        Pothos::Callable(&proxyVectorToGrFECGenericEncoderSptr));
}

//
// Tests
//

#include <gnuradio/constants.h>

POTHOS_TEST_BLOCK("/gnuradio/tests", test_fec_cc_encoder)
{
    auto env = Pothos::ProxyEnvironment::make("managed");

    // Values from gr-fec/examples/fecapi_cc_decoders.grc
    const Pothos::ProxyVector params =
    {
        env->makeProxy(60) /*frame_size*/,
        env->makeProxy(7) /*k*/,
        env->makeProxy(2) /*rate*/,
        env->makeProxy(std::vector<int>{109, 79}) /*polys*/,
        env->makeProxy(0) /*start_state*/,
        env->makeProxy("CC_STREAMING") /*mode*/,
        env->makeProxy(false) /*padded*/
    };

    auto encoderSptr = Pothos::Object(params).convert<gr::fec::generic_encoder::sptr>();
    POTHOS_TEST_EQUAL("cc_encoder", encoderSptr->d_name);
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_fec_ccsds_encoder)
{
    auto env = Pothos::ProxyEnvironment::make("managed");

    // Values from gr-fec/examples/fecapi_encoders.grc
    const Pothos::ProxyVector params =
    {
        env->makeProxy(60) /*frame_size*/,
        env->makeProxy(0) /*start_state*/,
        env->makeProxy("CC_STREAMING") /*mode*/
    };

    auto encoderSptr = Pothos::Object(params).convert<gr::fec::generic_encoder::sptr>();
    POTHOS_TEST_EQUAL("ccsds_encoder", encoderSptr->d_name);
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_fec_dummy_encoder)
{
    auto env = Pothos::ProxyEnvironment::make("managed");

    const Pothos::ProxyVector params =
    {
        env->makeProxy(60) /*frame_size*/,
        env->makeProxy(false) /*pack*/,
        env->makeProxy(false) /*packed_bits*/
    };

    auto encoderSptr = Pothos::Object(params).convert<gr::fec::generic_encoder::sptr>();
    POTHOS_TEST_EQUAL("dummy_encoder", encoderSptr->d_name);
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_fec_ldpc_encoder)
{
    auto env = Pothos::ProxyEnvironment::make("managed");

    // Values from gr-fec/examples/fecapi_ldpc_encoders.grc
    const Pothos::ProxyVector params =
    {
        env->makeProxy(Poco::Path(gr::prefix() + "/share/gnuradio/fec/ldpc/n_0100_k_0042_gap_02.alist").toString()) /*alist_file*/,
    };

    auto encoderSptr = Pothos::Object(params).convert<gr::fec::generic_encoder::sptr>();

    // GNU Radio mistakenly doesn't set the encoder name for the LDPC encoder, so confirm another way.
    POTHOS_TEST_EQUAL(
        "gr::fec::ldpc_encoder_impl",
        Pothos::Util::typeInfoToString(typeid(*encoderSptr)));
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_fec_ldpc_par_mtrx_encoder)
{
    auto env = Pothos::ProxyEnvironment::make("managed");

    // Values from gr-fec/examples/fecapi_ldpc_encoders.grc
    const Pothos::ProxyVector params =
    {
        env->makeProxy(Poco::Path(gr::prefix() + "/share/gnuradio/fec/ldpc/n_0100_k_0042_gap_02.alist").toString()) /*alist_file*/,
        env->makeProxy(2) /*gap*/
    };

    auto encoderSptr = Pothos::Object(params).convert<gr::fec::generic_encoder::sptr>();

    // GNU Radio mistakenly doesn't set the encoder name for the LDPC encoder, so confirm another way.
    POTHOS_TEST_EQUAL(
        "gr::fec::code::ldpc_par_mtrx_encoder_impl",
        Pothos::Util::typeInfoToString(typeid(*encoderSptr)));
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_fec_repetition_encoder)
{
    auto env = Pothos::ProxyEnvironment::make("managed");

    // Values from gr-fec/examples/fecapi_async_encoders.grc
    const Pothos::ProxyVector params =
    {
        env->makeProxy(1000) /*frame_size*/,
        env->makeProxy(3) /*rep*/
    };

    auto encoderSptr = Pothos::Object(params).convert<gr::fec::generic_encoder::sptr>();
    POTHOS_TEST_EQUAL("repetition_encoder", encoderSptr->d_name);
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_fec_tpc_encoder)
{
    auto env = Pothos::ProxyEnvironment::make("managed");

    // Values from gr-fec/examples/tpc_ber_curve_gen.grc
    const Pothos::ProxyVector params =
    {
        env->makeProxy(std::vector<int>{3}) /*row_poly*/,
        env->makeProxy(std::vector<int>{43}) /*col_poly*/,
        env->makeProxy(26) /*krow*/,
        env->makeProxy(6) /*kcol*/,
        env->makeProxy(9) /*bval*/,
        env->makeProxy(3) /*qval*/
    };

    auto encoderSptr = Pothos::Object(params).convert<gr::fec::generic_encoder::sptr>();

    // GNU Radio mistakenly doesn't set the encoder name for the LDPC encoder, so confirm another way.
    POTHOS_TEST_EQUAL(
        "gr::fec::tpc_encoder",
        Pothos::Util::typeInfoToString(typeid(*encoderSptr)));
}
