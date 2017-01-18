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
#include <Pothos/Object.hpp>
#include <Pothos/Framework.hpp>
#include <gnuradio/basic_block.h>
#include <gnuradio/block.h>
#include <pmt/pmt.h>

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
Pothos::DType inferDType(const size_t ioSize, const std::string &name, const bool isInput);
