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

static inline bool is_power_of_2(size_t val)
{
    constexpr double epsilon = 1e-6;
    const auto log2_val = std::log2(val);

    return std::abs(log2_val - std::floor(log2_val)) <= epsilon;
}

template <typename T>
static inline size_t log2_vector_len(const std::vector<T>& vec)
{
    return static_cast<size_t>(std::log2(vec.size()));
}

/*
 * Ported from: gr-fec/python/capillary_threaded_coder.py
 */

template <typename CoderType>
class capillary_threaded_coder: public Pothos::Topology
{
public:
    using Class = capillary_threaded_coder<CoderType>;

    static Pothos::Topology* make(
        const std::vector<std::shared_ptr<CoderType>>& generic_coders,
        const Pothos::DType& input_size,
        const Pothos::DType& output_size,
        const std::string& coder_name)
    {
        return new capillary_threaded_coder(generic_coders, input_size, output_size, coder_name);
    }

    capillary_threaded_coder(const std::vector<std::shared_ptr<CoderType>>& generic_coders,
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
        else if(!is_power_of_2(d_generic_coders.size()))
        {
            throw Pothos::InvalidArgumentException("Number of "+d_coder_name+"s must be a power of 2.");
        }

        for(size_t i = 0; i < log2_vector_len(d_generic_coders); ++i)
        {
            for(size_t j = 0; j < static_cast<size_t>(std::pow(2, i)); ++j)
            {
                d_deinterleaves.emplace_back(Pothos::BlockRegistry::make(
                                                 "/gr/blocks/deinterleave",
                                                 d_input_size,
                                                 d_generic_coders[0]->get_input_size()));
                d_interleaves.emplace_back(Pothos::BlockRegistry::make(
                                               "/gr/blocks/interleave",
                                               d_output_size,
                                               d_generic_coders[0]->get_output_size()));
            }
        }

        // Now to connect everything.

        size_t rootcount = 0;
        size_t branchcount = 0;
        size_t codercount = 0;

        branchcount = 1;
        for(size_t i = 0; i < log2_vector_len(d_generic_coders)-1; ++i)
        {
            for(size_t j = 0; j < static_cast<size_t>(std::pow(2, i)); ++j)
            {
                this->connect(
                    d_deinterleaves[rootcount], 0,
                    d_deinterleaves[branchcount], 0);
                this->connect(
                    d_deinterleaves[rootcount], 1,
                    d_deinterleaves[branchcount+1], 0);

                ++rootcount;
                branchcount += 2;
            }
        }

        for(size_t i = 0; i < d_generic_coders.size()/2; ++i)
        {
            this->connect(
                d_deinterleaves[rootcount], 0,
                d_fec_coders[codercount], 0);
            this->connect(
                d_deinterleaves[rootcount], 1,
                d_fec_coders[codercount+1], 0);
        }

        rootcount = 0;
        branchcount = 0;
        for(size_t i = 0; i < log2_vector_len(d_generic_coders)-1; ++i)
        {
            for(size_t j = 0; j < static_cast<size_t>(std::pow(2, i)); ++j)
            {
                this->connect(
                    d_interleaves[branchcount], 0,
                    d_interleaves[rootcount], 0);
                this->connect(
                    d_interleaves[branchcount+1], 0,
                    d_interleaves[rootcount], 1);

                ++rootcount;
                branchcount += 2;
            }
        }

        codercount = 0;
        for(size_t i = 0; i < d_generic_coders.size()/2; ++i)
        {
            this->connect(
                d_fec_coders[codercount], 0,
                d_interleaves[rootcount], 0);
            this->connect(
                d_fec_coders[codercount+1], 0,
                d_interleaves[rootcount], 1);
        }

        if(d_generic_coders.size() > 1)
        {
            this->connect(
                this, 0,
                d_deinterleaves[0], 0);
            this->connect(
                d_interleaves[0], 0,
                this, 0);
        }
        else
        {
            this->connect(
                this, 0,
                d_fec_coders[0], 0);
            this->connect(
                d_fec_coders[0], 0,
                this, 0);
        }

        this->registerCall(this, "generic_"+d_coder_name+"s", &Class::generic_coders);
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

    std::vector<Pothos::Proxy> d_deinterleaves;
    std::vector<Pothos::Proxy> d_interleaves;
    std::vector<Pothos::Proxy> d_fec_coders;
};

static Pothos::BlockRegistry registerThreadedEncoder(
    "/gr/fec/capillary_threaded_encoder",
    Pothos::Callable(&capillary_threaded_coder<gr::fec::generic_encoder>::make)
        .bind("encoder", 3));

static Pothos::BlockRegistry registerThreadedDecoder(
    "/gr/fec/capillary_threaded_decoder",
    Pothos::Callable(&capillary_threaded_coder<gr::fec::generic_decoder>::make)
        .bind("decoder", 3));
