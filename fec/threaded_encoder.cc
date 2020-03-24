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

#include <gnuradio/fec/generic_encoder.h>

#include <vector>

/*
 * Ported from: gr-fec/python/threaded_encoder.py
 */

class threaded_encoder: public Pothos::Topology
{
public:
    static Pothos::Topology* make(
        const std::vector<gr::fec::generic_encoder::sptr>& generic_encoders,
        const Pothos::DType& input_size,
        const Pothos::DType& output_size)
    {
        return new threaded_encoder(generic_encoders, input_size, output_size);
    }

    threaded_encoder(const std::vector<gr::fec::generic_encoder::sptr>& generic_encoders,
                     const Pothos::DType& input_size,
                     const Pothos::DType& output_size):
        Pothos::Topology(),
        d_generic_encoders(generic_encoders),
        d_input_size(input_size),
        d_output_size(output_size)
    {
        if(d_generic_encoders.empty())
        {
            throw Pothos::InvalidArgumentException("Encoder list cannot be empty.");
        }

        d_deinterleave = Pothos::BlockRegistry::make(
                             "/gr/blocks/deinterleave",
                             d_input_size,
                             d_generic_encoders[0]->get_input_size());
        d_interleave = Pothos::BlockRegistry::make(
                           "/gr/blocks/interleave",
                           d_output_size,
                           d_generic_encoders[0]->get_output_size());

        for(size_t encoder_index = 0; encoder_index < d_generic_encoders.size(); ++encoder_index)
        {
            d_fec_encoders.emplace_back(Pothos::BlockRegistry::make(
                                            "/gr/fec/encoder",
                                            d_generic_encoders[encoder_index],
                                            d_input_size,
                                            d_output_size));

            this->connect(
                d_deinterleave, encoder_index,
                d_fec_encoders[encoder_index], 0);
            this->connect(
                d_fec_encoders[encoder_index], 0,
                d_interleave, encoder_index);
        }

        this->registerCall(this, POTHOS_FCN_TUPLE(threaded_encoder, generic_encoders));

        this->connect(this, 0, d_deinterleave, 0);
        this->connect(d_interleave, 0, this, 0);
    }

    std::vector<gr::fec::generic_encoder::sptr> generic_encoders()
    {
        return d_generic_encoders;
    }

private:
    std::vector<gr::fec::generic_encoder::sptr> d_generic_encoders;
    Pothos::DType d_input_size;
    Pothos::DType d_output_size;

    Pothos::Proxy d_deinterleave;
    Pothos::Proxy d_interleave;
    std::vector<Pothos::Proxy> d_fec_encoders;
};

static Pothos::BlockRegistry registerThreadedEncoder(
    "/gr/fec/threaded_encoder",
    Pothos::Callable(&threaded_encoder::make));
