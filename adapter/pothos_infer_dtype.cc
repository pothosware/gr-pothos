/*
 * Copyright 2016-2017 Free Software Foundation, Inc.
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
#include <cctype>

Pothos::DType inferDType(const size_t ioSize, const std::string &name, const bool isInput)
{
    Pothos::DType dtype("custom", ioSize); //default if we cant figure it out

    //make a guess based on size and the usual types
    if ((ioSize%sizeof(gr_complex)) == 0) dtype = Pothos::DType(typeid(gr_complex), ioSize/sizeof(gr_complex));
    else if ((ioSize%sizeof(float)) == 0) dtype = Pothos::DType(typeid(float), ioSize/sizeof(float));
    else if ((ioSize%sizeof(short)) == 0) dtype = Pothos::DType(typeid(short), ioSize/sizeof(short));
    else if ((ioSize%sizeof(char)) == 0) dtype = Pothos::DType(typeid(char), ioSize/sizeof(char));

    const auto lastUnder = name.find_last_of("_");
    if (lastUnder == std::string::npos) return dtype;

    //grab data type suffix
    auto suffix = name.substr(lastUnder+1);
    if (suffix.empty()) return dtype;
    if (suffix == "sink") return dtype;
    if (suffix == "source") return dtype;

    //strip leading v (for vector)
    if (std::tolower(suffix.front()) == 'v') suffix = suffix.substr(1);
    if (suffix.empty()) return dtype;

    //extract signature character
    char sig = suffix.front();
    if (not isInput and suffix.size() >= 2) sig = suffix.at(1);
    sig = std::tolower(sig);

    //inspect signature and io type for size-multiple
    #define inspectSig(sigName, sigType) \
        if (sig == sigName and ioSize%sizeof(sigType) == 0) \
            return Pothos::DType(typeid(sigType), ioSize/sizeof(sigType))
    inspectSig('b', char);
    inspectSig('s', short);
    inspectSig('i', int);
    inspectSig('f', float);
    inspectSig('c', gr_complex);
    return dtype;
}
