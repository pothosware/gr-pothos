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
#include <gnuradio/fec/generic_encoder.h>

#include <memory>
#include <string>
#include <vector>

/*
 * Ported from: gr-fec/python/threaded_encoder.py
 */

template <typename CoderType>
class threaded_coder: public Pothos::Topology
{
public:
    using Class = threaded_coder<CoderType>;
    
    static Pothos::Topology* make(
        const std::vector<std::shared_ptr<CoderType>>& generic_coders,
        const Pothos::DType& input_size,
        const Pothos::DType& output_size,
        const std::string& coder_name)
    {
        return new threaded_coder(generic_coders, input_size, output_size, coder_name);
    }

    threaded_coder(const std::vector<std::shared_ptr<CoderType>>& generic_coders,
                   const Pothos::DType& input_size,
                   const Pothos::DType& output_size,
                   const std::string& coder_name):
        Pothos::Topology(),
        d_generic_coders(generic_coders),
        d_input_size(input_size),
        d_output_size(output_size),
        d_coder_name(coder_name)
    {
        if(d_generic_coders.empty())
        {
            auto cap_coder_name = d_coder_name;
            cap_coder_name[0] = ::toupper(cap_coder_name[0]);

            throw Pothos::InvalidArgumentException(cap_coder_name+" list cannot be empty.");
        }

        d_deinterleave = Pothos::BlockRegistry::make(
                             "/gr/blocks/deinterleave",
                             d_input_size,
                             d_generic_coders[0]->get_input_size());
        d_interleave = Pothos::BlockRegistry::make(
                           "/gr/blocks/interleave",
                           d_output_size,
                           d_generic_coders[0]->get_output_size());

        for(size_t encoder_index = 0; encoder_index < d_generic_coders.size(); ++encoder_index)
        {
            d_fec_coders.emplace_back(Pothos::BlockRegistry::make(
                                            "/gr/fec/"+coder_name,
                                            d_generic_coders[encoder_index],
                                            d_input_size,
                                            d_output_size));

            this->connect(
                d_deinterleave, encoder_index,
                d_fec_coders[encoder_index], 0);
            this->connect(
                d_fec_coders[encoder_index], 0,
                d_interleave, encoder_index);
        }

        this->registerCall(this, "generic_"+d_coder_name+"s", &Class::generic_coders);

        this->connect(this, 0, d_deinterleave, 0);
        this->connect(d_interleave, 0, this, 0);
    }

    std::vector<std::shared_ptr<CoderType>> generic_coders()
    {
        return d_generic_coders;
    }

private:
    std::vector<std::shared_ptr<CoderType>> d_generic_coders;
    Pothos::DType d_input_size;
    Pothos::DType d_output_size;
    std::string d_coder_name;

    Pothos::Proxy d_deinterleave;
    Pothos::Proxy d_interleave;
    std::vector<Pothos::Proxy> d_fec_coders;
};

static Pothos::BlockRegistry registerThreadedEncoder(
    "/gr/fec/threaded_encoder",
    Pothos::Callable(&threaded_coder<gr::fec::generic_encoder>::make)
        .bind("encoder", 3));

static Pothos::BlockRegistry registerThreadedDecoder(
    "/gr/fec/threaded_decoder",
    Pothos::Callable(&threaded_coder<gr::fec::generic_decoder>::make)
        .bind("decoder", 3));
