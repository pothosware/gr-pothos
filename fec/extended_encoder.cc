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

#include <gnuradio/fec/generic_encoder.h>

#include <memory>
#include <string>
#include <vector>

/*
 * Ported from: gr-fec/python/extended_encoder.py
 */

class extended_encoder: public Pothos::Topology
{
public:
    static Pothos::Topology* make(
        const std::vector<gr::fec::generic_encoder::sptr>& encoder_list,
        const std::string& threading,
        const std::string& puncpat)
    {
        return new extended_encoder(encoder_list, threading, puncpat);
    }

    extended_encoder(const std::vector<gr::fec::generic_encoder::sptr>& encoder_list,
                     const std::string& threading,
                     const std::string& puncpat):
        Pothos::Topology()
    {
        if(encoder_list.empty())
        {
            throw Pothos::InvalidArgumentException("You must specify at least one encoder.");
        }
        if(("NONE" == threading) && (1 != encoder_list.size()))
        {
            throw Pothos::InvalidArgumentException("If no threading is specified, you must specify only a single encoder.");
        }

        if("pack" == std::string(encoder_list[0]->get_input_conversion()))
        {
            d_blocks.emplace_back(Pothos::BlockRegistry::make(
                                      "/gr/blocks/pack_k_bits_bb",
                                      8));
        }

        if("CAPILLARY" == threading)
        {
            d_blocks.emplace_back(Pothos::BlockRegistry::make(
                                      "/gr/fec/capillary_threaded_encoder",
                                      encoder_list,
                                      "int8",
                                      "int8"));
        }
        else if("ORDINARY" == threading)
        {
            d_blocks.emplace_back(Pothos::BlockRegistry::make(
                                      "/gr/fec/threaded_encoder",
                                      encoder_list,
                                      "int8",
                                      "int8"));
        }
        else if("NONE" == threading)
        {
            d_blocks.emplace_back(Pothos::BlockRegistry::make(
                                      "/gr/fec/encoder",
                                      encoder_list[0],
                                      "int8",
                                      "int8"));
        }
        else
        {
            throw Pothos::InvalidArgumentException("Invalid threading type", threading);
        }

        if("packed_bits" == std::string(encoder_list[0]->get_output_conversion()))
        {
            d_blocks.emplace_back(Pothos::BlockRegistry::make(
                                      "/gr/blocks/packed_to_unpacked",
                                      "packed_to_unpacked_bb",
                                      "GR_MSB_FIRST"));
        }

        if("11" != puncpat)
        {
            d_blocks.emplace_back(Pothos::BlockRegistry::make(
                                      "/gr/fec/puncture",
                                      "puncture_bb",
                                      puncpat.size(),
                                      read_bitlist(puncpat),
                                      0));
        }

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

static Pothos::BlockRegistry registerExtendedEncoder(
    "/gr/fec/extended_encoder",
    Pothos::Callable(&extended_encoder::make));
