/*
 * Copyright 2016-2020 Free Software Foundation, Inc.
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

#include <Pothos/Framework.hpp>

#include <Poco/RegularExpression.h>

#include <gnuradio/types.h>

#include <string>

static inline bool isBlockSourceOrSink(const std::string &name)
{
    return (name.find("source") != std::string::npos) || (name.find("sink") != std::string::npos);
}

std::string getConversionBlockTypeString(const std::string &name, const bool isInput)
{
    // ex. char_to_float, returns "char" for isInput, returns "float" for !isInput
    using RE = Poco::RegularExpression;

    static const RE conversionBlockRE("([a-z]+)_to_([a-z]+)");

    RE::MatchVec matches;
    const auto numMatches = conversionBlockRE.match(name, 0, matches);

    std::string typeStr;

    // Note: we're looking for two substrings, but this will return 3, as the first will have
    // the full value.
    if(3 == numMatches)
    {
        typeStr = isInput ? name.substr(matches[1].offset, matches[1].length)
                          : name.substr(matches[2].offset, matches[2].offset);
    }

    return typeStr;
}

Pothos::DType inferDType(const size_t ioSize, const std::string &name, const bool isInput, const size_t vlen)
{
    const auto singleElemSize = ioSize / vlen;

    // Default
    Pothos::DType dtype("custom", ioSize);

    // gr_complex and short's sizes don't match any others, so this is enough.
    if(singleElemSize == sizeof(gr_complex)) dtype = Pothos::DType(typeid(gr_complex), vlen);
    else if(singleElemSize == sizeof(short)) dtype = Pothos::DType(typeid(short), vlen);
    else if((singleElemSize == sizeof(char)) || (singleElemSize == sizeof(float)))
    {
        char suffix = 'x';
        const auto lastUnder = name.find_last_of("_");
        
        if(isBlockSourceOrSink(name))
        {
            suffix = name.back();
        }
        else if(lastUnder != std::string::npos)
        {
            auto suffixStr = name.substr(lastUnder+1);
            if((suffixStr.size() == 3) && ('v' == suffixStr[0]))
            {
                suffixStr = suffixStr.substr(1);
            }

            if(suffixStr.size() == 2)
            {
                suffix = isInput ? suffixStr[0] : suffixStr[1];
            }
            else
            {
                const auto typeStr = getConversionBlockTypeString(name, isInput);
                if("int" == typeStr) suffix = 'i';
                else if("float" == typeStr) suffix = 'f';
                else if("uchar" == typeStr) suffix = 'b';
                else if("char" == typeStr) suffix = 'a'; // GNU Radio doesn't have one, so this will do.
            }
        }

        switch(suffix)
        {
            case 'a':
                dtype = Pothos::DType(typeid(char), vlen);
                break;

            case 'b':
                dtype = Pothos::DType(typeid(unsigned char), vlen);
                break;

            case 'i':
                dtype = Pothos::DType(typeid(int), vlen);
                break;

            case 'f':
                dtype = Pothos::DType(typeid(float), vlen);
                break;

            default:
                break;
        }

        // We tried but couldn't find a way to distinguish between int8/uint8 or
        // int/float. Oh well.
        if(dtype.isCustom())
        {
            dtype = (1 == singleElemSize) ? Pothos::DType(typeid(unsigned char), vlen)
                                          : Pothos::DType(typeid(float), vlen);
        }
    }

    return dtype;
}
