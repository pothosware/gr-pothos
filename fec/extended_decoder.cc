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

#include "bitflip.h"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Plugin.hpp>

#include <gnuradio/fec/generic_decoder.h>

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

/*
 * Ported from: gr-fec/python/extended_decoder.py
 */


//solution to log_(1-2*t)(1-2*.0335) = 1/taps where t is thresh (syndrome density)
//for i in numpy.arange(.1, .499, .01):
//    print str(log((1-(2 * .035)), (1-(2 * i)))) + ':' + str(i);

static const std::unordered_map<double, double> garbletable =
{
    {0.310786835319,  0.1},
    {0.279118162802,  0.11},
    {0.252699589071,  0.12},
    {0.230318516016,  0.13},
    {0.211108735347,  0.14},
    {0.194434959095,  0.15},
    {0.179820650401,  0.16},
    {0.166901324951,  0.17},
    {0.15539341766,   0.18},
    {0.145072979886,  0.19},
    {0.135760766313,  0.2},
    {0.127311581396,  0.21},
    {0.119606529806,  0.22},
    {0.112547286766,  0.23},
    {0.106051798775,  0.24},
    {0.10005101381,   0.25},
    {0.0944863633098, 0.26},
    {0.0893078003966, 0.27},
    {0.084472254501,  0.28},
    {0.0799424008658, 0.29},
    {0.0756856701944, 0.3},
    {0.0716734425668, 0.31},
    {0.0678803831565, 0.32},
    {0.0642838867856, 0.33},
    {0.0608636049994, 0.34},
    {0.0576010337489, 0.35},
    {0.0544791422522, 0.36},
    {0.0514820241933, 0.37},
    {0.0485945507251, 0.38},
    {0.0458019998183, 0.39},
    {0.0430896262596, 0.4},
    {0.0404421166935, 0.41},
    {0.0378428350972, 0.42},
    {0.0352726843274, 0.43},
    {0.0327082350617, 0.44},
    {0.0301183562535, 0.45},
    {0.0274574540266, 0.46},
    {0.0246498236897, 0.47},
    {0.0215448131298, 0.48},
    {0.0177274208353, 0.49},
};
static std::vector<double> __garbletable_keys()
{
    std::vector<double> keys;
    std::transform(
        garbletable.begin(),
        garbletable.end(),
        std::back_inserter(keys),
        [](const std::unordered_map<double, double>::value_type& pair)
        {
            return pair.first;
        });
    std::sort(keys.begin(), keys.end());

    return keys;
}

static const std::vector<double>& garbletable_keys()
{
    // Only do this once.
    static const auto keys = __garbletable_keys();

    return keys;
}
/*
    def __init__(self, decoder_obj_list, threading, ann=None, puncpat='11',
                 integration_period=10000, flush=None, rotator=None):
 */
class extended_decoder: public Pothos::Topology
{
public:
    static Pothos::Topology* make(
        const std::vector<gr::fec::generic_decoder::sptr>& decoder_list,
        const std::string& threading,
        const std::string& ann,
        const std::string& puncpat,
        int integration_period,
        int flush)
    {
        return new extended_decoder(decoder_list, threading, ann, puncpat, integration_period, flush);
    }

    extended_decoder(const std::vector<gr::fec::generic_decoder::sptr>& decoder_list,
                     const std::string& threading,
                     const std::string& ann,
                     const std::string& puncpat,
                     int integration_period,
                     int flush):
        Pothos::Topology()
    {
        if(decoder_list.empty())
        {
            throw Pothos::InvalidArgumentException("You must specify at least one decoder.");
        }
        if(("NONE" == threading) && (1 != decoder_list.size()))
        {
            throw Pothos::InvalidArgumentException("If no threading is specified, you must specify only a single decoder.");
        }
        if((decoder_list.size() > 1) && (decoder_list[0]->get_history() != 0))
        {
            throw Pothos::InvalidArgumentException("Cannot use multi-threaded parallelism on a decoder with history.");
        }

        const std::string decoder0_input_conv = decoder_list[0]->get_input_conversion();
        const std::string decoder0_output_conv = decoder_list[0]->get_output_conversion();
        const auto decoder0_input_item_size = decoder_list[0]->get_input_item_size();
        const auto decoder0_output_item_size = decoder_list[0]->get_output_item_size();
        const auto decoder0_shift = decoder_list[0]->get_shift();

        // Anything going through the annihilator needs shifted uchar vals.
        if(("uchar" == decoder0_input_conv) || ("packed_bits" == decoder0_input_conv))
        {
            d_blocks.emplace_back(Pothos::BlockRegistry::make(
                                      "/gr/blocks/multiply_const",
                                      "multiply_const_ff",
                                      48.0f));
        }
        if(0.0 != decoder0_shift)
        {
            d_blocks.emplace_back(Pothos::BlockRegistry::make(
                                      "/gr/blocks/add_const",
                                      "add_const_ff",
                                      decoder0_shift));
        }
        if("packed_bits" == decoder0_input_conv)
        {
            d_blocks.emplace_back(Pothos::BlockRegistry::make(
                                      "/gr/blocks/add_const",
                                      "add_const_ff",
                                      128.0f));
        }
        if(("uchar" == decoder0_input_conv) || ("packed_bits" == decoder0_input_conv))
        {
            d_blocks.emplace_back(Pothos::BlockRegistry::make("/gr/blocks/float_to_uchar"));
        }

        if(!ann.empty())
        {
            const auto cat = read_big_bitlist(ann);
            const auto ann_num_ones = std::count(std::begin(cat), std::end(cat), '1');

            // Find the last index whose value is greater than the number of ones in
            // the annihilator.
            double synd_garble = 0.49;
            const auto& idx_list = garbletable_keys();
            auto idx_list_iter = std::find_if(
                                     idx_list.rbegin(),
                                     idx_list.rend(),
                                     [&ann_num_ones](double i)
                                     {
                                         return ((1.0 / ann_num_ones) >= i);
                                     });
            if(idx_list.rend() == idx_list_iter)
            {
                synd_garble = garbletable.at(*idx_list_iter);
            }

            const auto puncpat_num_zeros = std::count(std::begin(puncpat), std::end(puncpat), '0');
            d_blocks.emplace_back(Pothos::BlockRegistry::make(
                                      "/gr/fec/conv_bit_corr_bb",
                                      cat,
                                      puncpat.size() - puncpat_num_zeros,
                                      ann.size(),
                                      integration_period,
                                      flush,
                                      synd_garble));
        }

        if("11" != puncpat)
        {
            d_blocks.emplace_back(Pothos::BlockRegistry::make(
                                      "/gr/fec/depuncture_bb",
                                      puncpat.size(),
                                      read_bitlist(puncpat),
                                      0 /*symbol*/));
        }

        if("packed_bits" == decoder0_input_conv)
        {
            d_blocks.emplace_back(Pothos::BlockRegistry::make("/gr/blocks/uchar_to_float"));
            d_blocks.emplace_back(Pothos::BlockRegistry::make(
                                      "/gr/blocks/add_const",
                                      "add_const_ff",
                                      -128.0f));
            d_blocks.emplace_back(Pothos::BlockRegistry::make("/gr/digital/binary_slicer_fb"));
            d_blocks.emplace_back(Pothos::BlockRegistry::make(
                                      "/gr/blocks/unpacked_to_packed",
                                      "unpacked_to_packed_bb",
                                      1 /*bits_per_chunk*/,
                                      "GR_MSB_FIRST"));
        }

        // TODO: logic for converting from decoder type to item sizes, since Pothos uses DType for those

        /*
        if threading == 'capillary':
            self.blocks.append(capillary_threaded_decoder(decoder_obj_list,
                                                          fec.get_decoder_input_item_size(decoder_obj_list[0]),
                                                          fec.get_decoder_output_item_size(decoder_obj_list[0])))

        elif threading == 'ordinary':
            self.blocks.append(threaded_decoder(decoder_obj_list,
                                                fec.get_decoder_input_item_size(decoder_obj_list[0]),
                                                fec.get_decoder_output_item_size(decoder_obj_list[0])))

        else:
            self.blocks.append(fec.decoder(decoder_obj_list[0],
                                           fec.get_decoder_input_item_size(decoder_obj_list[0]),
                                           fec.get_decoder_output_item_size(decoder_obj_list[0])))

        if fec.get_decoder_output_conversion(decoder_obj_list[0]) == "unpack":
            self.blocks.append(blocks.packed_to_unpacked_bb(1, gr.GR_MSB_FIRST));

        self.connect((self, 0), (self.blocks[0], 0));
        self.connect((self.blocks[-1], 0), (self, 0));

        for i in range(len(self.blocks) - 1):
            self.connect((self.blocks[i], 0), (self.blocks[i+1], 0));
         */

        this->connect(
            this, 0,
            d_blocks[0], 0);
        this->connect(
            d_blocks.back(), 0,
            this, 0);

        for(size_t i = 0; i < (d_blocks.size()-1); ++i)
        {
            this->connect(
                d_blocks[i], 0,
                d_blocks[i+1], 0);
        }
    }

private:
    std::vector<Pothos::Proxy> d_blocks;
};

static Pothos::BlockRegistry registerExtendedDecoder(
    "/gr/fec/extended_decoder",
    Pothos::Callable(&extended_decoder::make));
