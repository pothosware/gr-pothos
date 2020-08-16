/*
 * Copyright 2014-2017 Free Software Foundation, Inc.
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

#pragma once
#include <Pothos/Object/Object.hpp>
#include <Pothos/Framework/DType.hpp>
#include <gnuradio/runtime_types.h>
#include <pmt/pmt.h>
#include <boost/shared_ptr.hpp>
#include <memory>
#include <string>
#include <type_traits>

// Compile-time check for what shared_ptr this build uses

constexpr bool GRUsesStdSPtr = std::is_same<gr::block_sptr, std::shared_ptr<gr::block>>::value;

//template <typename T, std::enable_if<GRUsesStdSPtr>::type* = nullptr>
template <typename T>
struct GRTraits
{
    using SPtr = typename std::shared_ptr<T>;

    template <typename U>
    inline static std::shared_ptr<U> dynamicPointerCast(SPtr input)
    {
        return std::dynamic_pointer_cast<U>(input);
    }
};

/*
template <typename T, std::enable_if<!GRUsesStdSPtr>::type* = nullptr>
struct GRTraits
{
    using SPtr = typename boost::shared_ptr<T>;

    template <typename U>
    inline static boost::shared_ptr<U> dynamicPointerCast(SPtr input)
    {
        return boost::dynamic_pointer_cast<U>(input);
    }
}
*/

/*!
 * Conversions between Object and pmt_t types.
 * This is a simple type-based conversion.
 * In the future, we can make this more efficient
 * by directly typedefing pmt_t as Object
 * and reimplementing pmt calls on top of STL.
 */

pmt::pmt_t obj_to_pmt(const Pothos::Object &obj);

Pothos::Object pmt_to_obj(const pmt::pmt_t &pmt);

//! try our best to infer the data type given the info at hand
Pothos::DType inferDType(const size_t ioSize, const std::string &name, const bool isInput, const size_t vlen=1);
