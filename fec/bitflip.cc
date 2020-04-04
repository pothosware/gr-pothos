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

int read_bitlist(const std::string& bitlist)
{
    const auto len = bitlist.size();

    int res = 0;
    for(size_t i = 0; i < len; ++i)
    {
        if('1' == bitlist[i])
        {
            res += (1 << (len - i + 1));
        }
    }

    return res;
}

std::vector<unsigned long long> read_big_bitlist(const std::string& bitlist)
{
    std::vector<unsigned long long> ret;

    const auto lenDiv64 = (bitlist.size() / 64);
    const auto lenMod64 = (bitlist.size() % 64);

    size_t j = 0;
    unsigned long long res = 0;

    for(; j < lenDiv64; ++j)
    {
        for(size_t i = 0; i < 64; ++i)
        {
            const auto bitStr = bitlist.substr((j*64+i),1);

            const unsigned long long bitULL = std::atoi(bitStr.c_str());
            if(1 == bitULL)
            {
                res += (1 << (64 - i - 1));
            }
            ret.emplace_back(bitULL);
        }
    }

    j = 0;
    res = 0;
    for(size_t i = 0; i < lenMod64; ++i)
    {
        const auto bitStr = bitlist.substr((j*64+i),1);

        const unsigned long long bitULL = std::atoi(bitStr.c_str());
        if(1 == bitULL)
        {
            res += (1 << (64 - i - 1));
        }
        ++j;
        ret.emplace_back(bitULL);
    }
    ret.emplace_back(res);

    return ret;
}
