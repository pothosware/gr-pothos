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

#include <gnuradio/fec/fec_mtrx.h>
#include <gnuradio/fec/ldpc_G_matrix.h>
#include <gnuradio/fec/ldpc_H_matrix.h>

#include <Poco/Path.h>

#include <string>

// Thin wrapper functions to convert fec_mtrx implementation factory
// functions to fec_mtrx_sptr.

static gr::fec::code::fec_mtrx_sptr make_G_matrix(const std::string& filename)
{
    return gr::fec::code::ldpc_G_matrix::make(filename)->get_base_sptr();
}

static gr::fec::code::fec_mtrx_sptr make_H_matrix(
    const std::string& filename,
    unsigned int gap)
{
    return gr::fec::code::ldpc_H_matrix::make(filename, gap)->get_base_sptr();
}

static auto managedMtrxSptr = Pothos::ManagedClass()
    .registerClass<gr::fec::code::fec_mtrx_sptr>()
    .registerConstructor(&make_G_matrix)
    .registerConstructor(&make_H_matrix)
    .commit("gr/fec/mtrx_sptr");

// Converter for easy conversion from evalulated ProxyVector
static gr::fec::code::fec_mtrx_sptr proxyVectorToGrFECMtrxSptr(const Pothos::ProxyVector &args)
{
    auto env = Pothos::ProxyEnvironment::make("managed");
    auto cls = env->findProxy("gr/fec/mtrx_sptr");
    auto decoder = cls.getHandle()->call("()", args.data(), args.size());
    return decoder;
}

pothos_static_block(registerProxyVectorToGrFECMtrxSptr)
{
    Pothos::PluginRegistry::add(
        "/object/convert/containers/proxy_vec_to_gr_fec_mtrx_sptr",
        Pothos::Callable(&proxyVectorToGrFECMtrxSptr));
}

//
// Tests
//

#include <gnuradio/constants.h>


POTHOS_TEST_BLOCK("/gnuradio/tests", test_fec_mtrx)
{
    auto env = Pothos::ProxyEnvironment::make("managed");

    // G matrix

    // For some reason, the G matrix files shipped with GNU Radio are causing the ldpc_G_matrix destructor
    // to crash when freeing GSL resources.
    /*
    const Pothos::ProxyVector gMatrixParams =
    {
        env->makeProxy(Poco::Path(gr::prefix() + "/share/gnuradio/fec/ldpc/n_0100_k_0058_gen_matrix.alist").toString()),
    };
    auto gMatrixSptr = Pothos::Object(gMatrixParams).convert<gr::fec::code::fec_mtrx_sptr>();
    */

    // H matrix

    const Pothos::ProxyVector hMatrixParams =
    {
        env->makeProxy(Poco::Path(gr::prefix() + "/share/gnuradio/fec/ldpc/n_0100_k_0027_gap_04.alist").toString()) /*filename*/,
        env->makeProxy(5) /*gap*/
    };
    auto hMatrixSptr = Pothos::Object(hMatrixParams).convert<gr::fec::code::fec_mtrx_sptr>();
}
