/*
 * Copyright 2014 Free Software Foundation, Inc.
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

#include "pothos_support.h"
#include <Pothos/Object.hpp>
#include <Pothos/Framework/Packet.hpp>
#include <Poco/Types.h> //POCO_LONG_IS_64_BIT
#include <cstdint>
#include <utility>
#include <vector>
#include <tuple>
#include <set>
#include <map>

pmt::pmt_t obj_to_pmt(const Pothos::Object &obj)
{
    //the container is null
    if (not obj) return pmt::pmt_t();

    //Packet support
    if (obj.type() == typeid(Pothos::Packet))
    {
        const auto &packet = obj.extract<Pothos::Packet>();

        //create metadata
        auto meta = pmt::make_dict();
        for (const auto &pr : packet.metadata)
        {
            meta = pmt::dict_add(meta, pmt::string_to_symbol(pr.first), obj_to_pmt(pr.second));
        }

        //create blob (creates a copy)
        auto blob = pmt::make_blob(packet.payload.as<const void *>(), packet.payload.length);

        return pmt::cons(meta, blob);
    }

    #define decl_obj_to_pmt(t, conv) \
        if (obj.type() == typeid(t)) return conv(obj.extract<t>())

    //bool
    decl_obj_to_pmt(bool, pmt::from_bool);

    //string
    decl_obj_to_pmt(std::string, pmt::string_to_symbol);

    //numeric types
    #ifdef POCO_LONG_IS_64_BIT
        decl_obj_to_pmt(signed long, pmt::from_long);
        decl_obj_to_pmt(unsigned long, pmt::from_long);
    #else
        decl_obj_to_pmt(signed long long, pmt::from_uint64);
        decl_obj_to_pmt(unsigned long long, pmt::from_uint64);
    #endif
    decl_obj_to_pmt(int8_t, pmt::from_long);
    decl_obj_to_pmt(int16_t, pmt::from_long);
    decl_obj_to_pmt(int32_t, pmt::from_long);
    decl_obj_to_pmt(uint8_t, pmt::from_long);
    decl_obj_to_pmt(uint16_t, pmt::from_long);
    decl_obj_to_pmt(uint32_t, pmt::from_long);
    decl_obj_to_pmt(int64_t, pmt::from_uint64);
    decl_obj_to_pmt(uint64_t, pmt::from_uint64);
    decl_obj_to_pmt(float, pmt::from_double);
    decl_obj_to_pmt(double, pmt::from_double);
    #define pmt_from_complex(x) pmtpmt::make_rectangular((x).real(), (x).imag())
    decl_obj_to_pmt(std::complex<float>, pmt::from_complex);
    decl_obj_to_pmt(std::complex<double>, pmt::from_complex);

    //pair container
    if (obj.type() == typeid(std::pair<Pothos::Object, Pothos::Object>))
    {
        const auto &pr = obj.extract<std::pair<Pothos::Object, Pothos::Object>>();
        return pmt::cons(obj_to_pmt(pr.first), obj_to_pmt(pr.second));
    }

    //skipping tuples -- not really used

    //vector container
    if (obj.type() == typeid(std::vector<Pothos::Object>))
    {
        const auto &l = obj.extract<std::vector<Pothos::Object>>();
        auto v = pmt::make_vector(l.size(), pmt::pmt_t());
        for (size_t i = 0; i < l.size(); i++)
        {
            pmt::vector_set(v, i, obj_to_pmt(l[i]));
        }
        return v;
    }

    //numeric arrays
    #define decl_obj_to_pmt_numeric_array(t, suffix) \
    if (obj.type() == typeid(std::vector<t>)) return pmt::init_ ## suffix ## vector(obj.extract<std::vector<t>>().size(), obj.extract<std::vector<t>>().data())
    decl_obj_to_pmt_numeric_array(uint8_t, u8);
    decl_obj_to_pmt_numeric_array(uint16_t, u16);
    decl_obj_to_pmt_numeric_array(uint32_t, u32);
    decl_obj_to_pmt_numeric_array(uint64_t, u64);
    decl_obj_to_pmt_numeric_array(int8_t, s8);
    decl_obj_to_pmt_numeric_array(int16_t, s16);
    decl_obj_to_pmt_numeric_array(int32_t, s32);
    decl_obj_to_pmt_numeric_array(int64_t, s64);
    decl_obj_to_pmt_numeric_array(float, f32);
    decl_obj_to_pmt_numeric_array(double, f64);
    decl_obj_to_pmt_numeric_array(std::complex<float>, c32);
    decl_obj_to_pmt_numeric_array(std::complex<double>, c64);


    //dictionary container
    if (obj.type() == typeid(std::map<Pothos::Object, Pothos::Object>))
    {
        auto d = pmt::make_dict();
        for (const auto &pr : obj.extract<std::map<Pothos::Object, Pothos::Object>>())
        {
            d = pmt::dict_add(d, obj_to_pmt(pr.first), obj_to_pmt(pr.second));
        }
        return d;
    }

    //set container
    if (obj.type() == typeid(std::set<Pothos::Object>))
    {
        auto l = pmt::PMT_NIL;
        for (const auto &elem : obj.extract<std::set<Pothos::Object>>())
        {
            l = pmt::list_add(l, obj_to_pmt(elem));
        }
        return l;
    }

    //is it already a pmt?
    if (obj.type() == typeid(pmt::pmt_t)) return obj.extract<pmt::pmt_t>();

    //backup plan... boost::any
    return pmt::make_any(obj);
}

struct SharedPMTHolder
{
    SharedPMTHolder(const pmt::pmt_t &p): ref(p){}
    pmt::pmt_t ref;
};

Pothos::Object pmt_to_obj(const pmt::pmt_t &p)
{
    //if the container null?
    if (not p) return Pothos::Object();

    //PDU support
    if (pmt::is_pair(p) and pmt::is_dict(pmt::car(p)) and pmt::is_blob(pmt::cdr(p)))
    {
        Pothos::Packet packet;

        //create a metadata map from the pmt dict
        pmt::pmt_t meta(pmt::car(p));
        auto items = pmt::dict_items(p);
        for (size_t i = 0; i < pmt::length(items); i++)
        {
            auto item = pmt::nth(i, items);
            auto key = pmt::symbol_to_string(pmt::car(item));
            auto val = pmt_to_obj(pmt::cdr(item));
            packet.metadata[key] = val;
        }

        //create a payload from the blob (zero-copy)
        pmt::pmt_t vect(pmt::cdr(p));
        packet.payload = Pothos::SharedBuffer(size_t(pmt::blob_data(vect)), pmt::length(vect), std::make_shared<SharedPMTHolder>(vect));
    }

    #define decl_pmt_to_obj(check, conv) if (check(p)) return Pothos::Object(conv(p))

    //bool
    decl_pmt_to_obj(pmt::is_bool, pmt::to_bool);

    //string (do object interning for strings)
    decl_pmt_to_obj(pmt::is_symbol, pmt::symbol_to_string);

    //numeric types
    //long can typedef to int64, force this to int32
    if (pmt::is_integer(p)) return Pothos::Object(int32_t(pmt::to_long(p)));
    //decl_pmt_to_obj(pmt::is_integer, pmt::to_long);
    decl_pmt_to_obj(pmt::is_uint64, pmt::to_uint64);
    decl_pmt_to_obj(pmt::is_real, pmt::to_double);
    decl_pmt_to_obj(pmt::is_complex, pmt::to_complex);

    //is it a boost any holding a Object?
    if (pmt::is_any(p))
    {
        const auto &a = pmt::any_ref(p);
        if (a.type() == typeid(Pothos::Object)) return boost::any_cast<Pothos::Object>(a);
    }

    //pair container
    if (pmt::is_pair(p))
    {
        auto pr = std::make_pair(pmt_to_obj(pmt::car(p)), pmt_to_obj(pmt::cdr(p)));
        return Pothos::Object(pr);
    }

    //skipping tuples -- not really used

    //vector container
    if (pmt::is_vector(p))
    {
        std::vector<Pothos::Object> l(pmt::length(p));
        for (size_t i = 0; i < l.size(); i++)
        {
            l[i] = pmt_to_obj(pmt::vector_ref(p, i));
        }
        return Pothos::Object(l);
    }

    //numeric arrays
    #define decl_pmt_to_obj_numeric_array(type, suffix) \
    if (is_ ## suffix ## vector(p)) \
    { \
        size_t n; const type* i = pmt:: suffix ## vector_elements(p, n); \
        return Pothos::Object(std::vector<type>(i, i+n)); \
    }
    decl_pmt_to_obj_numeric_array(uint8_t, u8);
    decl_pmt_to_obj_numeric_array(uint16_t, u16);
    decl_pmt_to_obj_numeric_array(uint32_t, u32);
    decl_pmt_to_obj_numeric_array(uint64_t, u64);
    decl_pmt_to_obj_numeric_array(int8_t, s8);
    decl_pmt_to_obj_numeric_array(int16_t, s16);
    decl_pmt_to_obj_numeric_array(int32_t, s32);
    decl_pmt_to_obj_numeric_array(int64_t, s64);
    decl_pmt_to_obj_numeric_array(float, f32);
    decl_pmt_to_obj_numeric_array(double, f64);
    decl_pmt_to_obj_numeric_array(std::complex<float>, c32);
    decl_pmt_to_obj_numeric_array(std::complex<double>, c64);

    //dictionary container
    if (pmt::is_dict(p))
    {
        std::map<Pothos::Object, Pothos::Object> m;
        auto items = pmt::dict_items(p);
        for (size_t i = 0; i < pmt::length(items); i++)
        {
            auto item = pmt::nth(i, items);
            auto key = pmt_to_obj(pmt::car(item));
            auto val = pmt_to_obj(pmt::cdr(item));
            m[key] = val;
        }
        return Pothos::Object(m);
    }

    //set container
    //FIXME no pmt_is_list...

    //backup plan... store the pmt
    return Pothos::Object(p);
}

/***********************************************************************
 * Register conversions for Pothos::Object conversion support:
 * We could register support additional conversions but this should
 * allow for PMTs to be used as input parameters for most types.
 **********************************************************************/
#include <Pothos/Plugin.hpp>
#include <Pothos/Callable.hpp>

pothos_static_block(pothosObjectRegisterPMTSupport)
{
    Pothos::PluginRegistry::add("/object/convert/gr/bool_to_pmt", Pothos::Callable(&pmt::from_bool));
    Pothos::PluginRegistry::add("/object/convert/gr/string_to_pmt", Pothos::Callable(&pmt::string_to_symbol));
    Pothos::PluginRegistry::add("/object/convert/gr/long_to_pmt", Pothos::Callable(&pmt::from_long));
    Pothos::PluginRegistry::add("/object/convert/gr/uint64_to_pmt", Pothos::Callable(&pmt::from_uint64));
    Pothos::PluginRegistry::add("/object/convert/gr/double_to_pmt", Pothos::Callable(&pmt::from_double));
    Pothos::PluginRegistry::add("/object/convert/gr/complex_to_pmt", Pothos::Callable::make<pmt::pmt_t, const std::complex<double> &>(&pmt::from_complex));
}
