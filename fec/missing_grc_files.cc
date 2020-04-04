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
#include <gnuradio/block.h>

#include <vector>

using namespace gr;

/***********************************************************************
 * include block definitions
 **********************************************************************/
#include "/usr/local/include/gnuradio/fec/conv_bit_corr_bb.h"

/***********************************************************************
 * make GrPothosBlock wrapper with a gr::block
 **********************************************************************/
template <typename BlockType>
std::shared_ptr<Pothos::Block> makeGrPothosBlock(boost::shared_ptr<BlockType> block, size_t vlen, const Pothos::DType& overrideDType)
{
    auto block_ptr = boost::dynamic_pointer_cast<gr::block>(block);
    auto env = Pothos::ProxyEnvironment::make("managed");
    auto registry = env->findProxy("Pothos/BlockRegistry");
    return registry.call<std::shared_ptr<Pothos::Block>>("/gnuradio/block", block_ptr, vlen, overrideDType);
}

/***********************************************************************
 * create block factories
 **********************************************************************/

// To disambiguate
using DeclareSampleDelayPtr = void(gr::block::*)(unsigned);

namespace gr {
namespace fec {

std::shared_ptr<Pothos::Block> factory__conv_bit_corr_bb(std::vector<unsigned long long> correlator, int corr_sym, int corr_len, int cut, int flush, float thresh)
{
    auto __orig_block = conv_bit_corr_bb::make(correlator, corr_sym, corr_len, cut, flush, thresh);
    auto __pothos_block = makeGrPothosBlock(__orig_block, 1, Pothos::DType());
    auto __orig_block_ref = std::ref(*static_cast<gr::fec::conv_bit_corr_bb *>(__orig_block.get()));
    __pothos_block->registerCallable("declare_sample_delay", Pothos::Callable((DeclareSampleDelayPtr)&gr::fec::conv_bit_corr_bb::declare_sample_delay).bind(__orig_block_ref, 0));
    __pothos_block->registerCallable("tag_propagation_policy", Pothos::Callable(&gr::fec::conv_bit_corr_bb::tag_propagation_policy).bind(__orig_block_ref, 0));
    __pothos_block->registerCallable("set_tag_propagation_policy", Pothos::Callable(&gr::fec::conv_bit_corr_bb::set_tag_propagation_policy).bind(__orig_block_ref, 0));
    return __pothos_block;
}

} //namespace $ns
} //namespace $ns

/***********************************************************************
 * register block factories
 **********************************************************************/
static Pothos::BlockRegistry register__conv_bit_corr_bb("/gr/fec/conv_bit_corr_bb", &gr::fec::factory__conv_bit_corr_bb);

/***********************************************************************
 * register block descriptions and conversions (TODO)
 **********************************************************************/
