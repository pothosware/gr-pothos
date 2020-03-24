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

#include <gnuradio/fec/generic_decoder.h>

#include <vector>

/*
 * Ported from: gr-fec/python/threaded_decoder.py
 */

class threaded_decoder: public Pothos::Topology
{
public:
    static Pothos::Topology* make(
        const std::vector<gr::fec::generic_decoder::sptr>& generic_decoders,
        const Pothos::DType& input_size,
        const Pothos::DType& output_size)
    {
        return new threaded_decoder(generic_decoders, input_size, output_size);
    }

    threaded_decoder(const std::vector<gr::fec::generic_decoder::sptr>& generic_decoders,
                     const Pothos::DType& input_size,
                     const Pothos::DType& output_size):
        Pothos::Topology(),
        d_generic_decoders(generic_decoders),
        d_input_size(input_size),
        d_output_size(output_size)
    {
        if(d_generic_decoders.empty())
        {
            throw Pothos::InvalidArgumentException("decoder list cannot be empty.");
        }

        d_interleave = Pothos::BlockRegistry::make(
                           "/gr/blocks/interleave",
                           d_input_size,
                           d_generic_decoders[0]->get_input_size());
        d_deinterleave = Pothos::BlockRegistry::make(
                             "/gr/blocks/deinterleave",
                             d_output_size,
                             d_generic_decoders[0]->get_output_size());

        for(size_t decoder_index = 0; decoder_index < d_generic_decoders.size(); ++decoder_index)
        {
            d_fec_decoders.emplace_back(Pothos::BlockRegistry::make(
                                            "/gr/fec/decoder",
                                            d_generic_decoders[decoder_index],
                                            d_input_size,
                                            d_output_size));

            this->connect(
                d_deinterleave, decoder_index,
                d_fec_decoders[decoder_index], 0);
            this->connect(
                d_fec_decoders[decoder_index], 0,
                d_interleave, decoder_index);
        }

        this->registerCall(this, POTHOS_FCN_TUPLE(threaded_decoder, generic_decoders));

        this->connect(this, 0, d_deinterleave, 0);
        this->connect(d_interleave, 0, this, 0);
    }

    std::vector<gr::fec::generic_decoder::sptr> generic_decoders()
    {
        return d_generic_decoders;
    }

private:
    std::vector<gr::fec::generic_decoder::sptr> d_generic_decoders;
    Pothos::DType d_input_size;
    Pothos::DType d_output_size;

    Pothos::Proxy d_interleave;
    Pothos::Proxy d_deinterleave;
    std::vector<Pothos::Proxy> d_fec_decoders;
};

static Pothos::BlockRegistry registerThreadeddecoder(
    "/gr/fec/threaded_decoder",
    Pothos::Callable(&threaded_decoder::make));
