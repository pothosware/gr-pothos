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

#include <gnuradio/fec/cc_decoder.h>
#include <gnuradio/fec/dummy_decoder.h>
#include <gnuradio/fec/ldpc_decoder.h>
#include <gnuradio/fec/ldpc_bit_flip_decoder.h>
#include <gnuradio/fec/repetition_decoder.h>
#include <gnuradio/fec/tpc_decoder.h>

#include <Poco/Path.h>

#include <string>

//
// TODOs:
//  * Polar decoders
//  * Override logger
//

//
// Helper methods to get access the generic_decoder methods and fields
//

#define WRAP_DECODER_FUNC(func,returnType) \
    static returnType generic_decoder_ ## func (const gr::fec::generic_decoder::sptr& decoder) \
    { \
        return decoder->func(); \
    }

WRAP_DECODER_FUNC(rate, double)
WRAP_DECODER_FUNC(get_input_size, int)
WRAP_DECODER_FUNC(get_output_size, int)
WRAP_DECODER_FUNC(get_history, int)
WRAP_DECODER_FUNC(get_shift, float)
WRAP_DECODER_FUNC(get_input_item_size, int)
WRAP_DECODER_FUNC(get_output_item_size, int)
WRAP_DECODER_FUNC(get_input_conversion, std::string)
WRAP_DECODER_FUNC(get_output_conversion, std::string)

static std::string generic_decoder_name(const gr::fec::generic_decoder::sptr& decoder)
{
    return decoder->d_name;
}

// Register the factory functions for each generic_decoder implementation
// as constructors for the managed class.
static auto managedGenericDecoderSptr = Pothos::ManagedClass()
    .registerClass<gr::fec::generic_decoder::sptr>()
    .registerConstructor(&gr::fec::code::cc_decoder::make)
    .registerConstructor(&gr::fec::code::dummy_decoder::make)
    .registerConstructor(&gr::fec::ldpc_decoder::make)
    .registerConstructor(&gr::fec::code::ldpc_bit_flip_decoder::make)
    .registerConstructor(&gr::fec::code::repetition_decoder::make)
    .registerConstructor(&gr::fec::tpc_decoder::make)
    .registerMethod("name", &generic_decoder_name)
    .registerMethod("rate", &generic_decoder_rate)
    .registerMethod("get_input_size", &generic_decoder_get_input_size)
    .registerMethod("get_output_size", &generic_decoder_get_output_size)
    .registerMethod("get_history", &generic_decoder_get_history)
    .registerMethod("get_shift", &generic_decoder_get_shift)
    .registerMethod("get_input_item_size", &generic_decoder_get_input_item_size)
    .registerMethod("get_output_item_size", &generic_decoder_get_output_item_size)
    .registerMethod("get_input_conversion", &generic_decoder_get_input_conversion)
    .registerMethod("get_output_conversion", &generic_decoder_get_output_conversion)
    .commit("gr/fec/generic_decoder_sptr");

// Converter for easy conversion from evalulated ProxyVector
static gr::fec::generic_decoder::sptr proxyVectorToGrFECGenericDecoderSptr(const Pothos::ProxyVector &args)
{
    auto env = Pothos::ProxyEnvironment::make("managed");
    auto cls = env->findProxy("gr/fec/generic_decoder_sptr");
    auto decoder = cls.getHandle()->call("()", args.data(), args.size());
    return decoder;
}


pothos_static_block(registerProxyVectorToGrFECGenericDecoderSptr)
{
    Pothos::PluginRegistry::add(
        "/object/convert/containers/proxy_vec_to_gr_fec_generic_decoder_sptr",
        Pothos::Callable(&proxyVectorToGrFECGenericDecoderSptr));
}

//
// Tests
//

#include <gnuradio/constants.h>

POTHOS_TEST_BLOCK("/gnuradio/tests", test_fec_cc_decoder)
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
        env->makeProxy(-1) /*end_state*/,
        env->makeProxy("CC_STREAMING") /*mode*/,
        env->makeProxy(false) /*padded*/
    };

    auto decoderSptr = Pothos::Object(params).convert<gr::fec::generic_decoder::sptr>();
    POTHOS_TEST_EQUAL("cc_decoder", decoderSptr->d_name);
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_fec_dummy_decoder)
{
    auto env = Pothos::ProxyEnvironment::make("managed");

    const Pothos::ProxyVector params = {env->makeProxy(60) /*frame_size*/};

    auto decoderSptr = Pothos::Object(params).convert<gr::fec::generic_decoder::sptr>();
    POTHOS_TEST_EQUAL("dummy_decoder", decoderSptr->d_name);
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_fec_ldpc_decoder)
{
    auto env = Pothos::ProxyEnvironment::make("managed");

    // Values from gr-fec/examples/fecapi_ldpc_decoders.grc
    const Pothos::ProxyVector params =
    {
        env->makeProxy(Poco::Path(gr::prefix() + "/share/gnuradio/fec/ldpc/n_0100_k_0042_gap_02.alist").toString()) /*alist_file*/,
        env->makeProxy(0.5) /*sigma*/,
        env->makeProxy(50) /*max_iterations*/
    };

    auto decoderSptr = Pothos::Object(params).convert<gr::fec::generic_decoder::sptr>();
    POTHOS_TEST_EQUAL("ldpc_decoder", decoderSptr->d_name);
}

/*
        static generic_decoder::sptr make(const fec_mtrx_sptr mtrx_obj,
                                          unsigned int max_iter=100);
 */
POTHOS_TEST_BLOCK("/gnuradio/tests", test_fec_ldpc_bit_flip_decoder)
{
    auto env = Pothos::ProxyEnvironment::make("managed");

    // Values from gr-fec/examples/fecapi_ldpc_decoders.grc
    const Pothos::ProxyVector params =
    {
        env->makeProxy(Pothos::ProxyVector{
            env->makeProxy(Poco::Path(gr::prefix() + "/share/gnuradio/fec/ldpc/n_0100_k_0042_gap_02.alist").toString()) /*alist_file*/,
            env->makeProxy(2) /*gap*/
        }) /*mtrx_obj*/,
        env->makeProxy(100) /*max_iter*/
    };

    auto decoderSptr = Pothos::Object(params).convert<gr::fec::generic_decoder::sptr>();
    POTHOS_TEST_EQUAL("ldpc_bit_flip_decoder", decoderSptr->d_name);
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_fec_repetition_decoder)
{
    auto env = Pothos::ProxyEnvironment::make("managed");

    // Values from gr-fec/examples/fecapi_async_decoders.grc
    const Pothos::ProxyVector params =
    {
        env->makeProxy(1500) /*frame_size*/,
        env->makeProxy(3) /*rep*/,
        env->makeProxy(0.5) /*ap_prob*/
    };

    auto decoderSptr = Pothos::Object(params).convert<gr::fec::generic_decoder::sptr>();
    POTHOS_TEST_EQUAL("repetition_decoder", decoderSptr->d_name);
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_fec_tpc_decoder)
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
        env->makeProxy(3) /*qval*/,
        env->makeProxy(6) /*max_iter*/,
        env->makeProxy(1) /*decoder_type (max_log_map)*/
    };

    auto decoderSptr = Pothos::Object(params).convert<gr::fec::generic_decoder::sptr>();
    POTHOS_TEST_EQUAL("tpc_decoder", decoderSptr->d_name);
}
