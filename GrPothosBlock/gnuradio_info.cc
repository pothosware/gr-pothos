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

#include <Poco/StringTokenizer.h>

#include <gnuradio/constants.h>

#include <Pothos/Exception.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/Testing.hpp>

#include <json.hpp>

#include <volk/volk_common.h> //VOLK_API

// For most of VOLK's history, the version header wasn't installed,
// so just declare the function we need here.
__VOLK_DECL_BEGIN
    VOLK_API const char* volk_version();
__VOLK_DECL_END

using json = nlohmann::json;

static constexpr int PocoTokenizerOptions = Poco::StringTokenizer::TOK_IGNORE_EMPTY
                                          | Poco::StringTokenizer::TOK_TRIM;

static std::string getLine(const std::string& str, size_t lineNum)
{
    Poco::StringTokenizer tokenizer(str, "\n", PocoTokenizerOptions);

    if(tokenizer.count() < lineNum)
    {
        throw Pothos::InvalidArgumentException(
                  "Invalid line number",
                  std::to_string(lineNum));
    }

    return tokenizer[lineNum];
}

static std::string getCompilerFlags(size_t lineNum)
{
    Poco::StringTokenizer tokenizer(getLine(gr::compiler_flags(), lineNum), ":::", PocoTokenizerOptions);
    if(tokenizer.count() != 2)
    {
        throw Pothos::AssertionViolationException("Unexpected gr::compiler_flags() format");
    }

    return tokenizer[1];
}

static std::string gnuradioInfo()
{
    json topObject;
    auto& gnuradioInfo = topObject["GNU Radio Info"];
    gnuradioInfo["Version"] = gr::version();
    gnuradioInfo["VOLK Version"] = ::volk_version();
    gnuradioInfo["C Compiler"] = getLine(gr::c_compiler(), 0);
    gnuradioInfo["C Compiler Flags"] = getCompilerFlags(0);
    gnuradioInfo["C++ Compiler"] = getLine(gr::cxx_compiler(), 0);
    gnuradioInfo["C++ Compiler Flags"] = getCompilerFlags(1);
    gnuradioInfo["Install Prefix"] = gr::prefix();
    gnuradioInfo["System Config Dir"] = gr::sysconfdir();
    gnuradioInfo["Prefs Dir"] = gr::prefsdir();

    return topObject.dump();
}

pothos_static_block(registerGnuRadioInfo)
{
    Pothos::PluginRegistry::addCall(
        "/devices/gnuradio/info", &gnuradioInfo);
}

POTHOS_TEST_BLOCK("/gnuradio/tests", test_gnuradio_info)
{
    // Just make sure the JSON string is valid. The JSON class will
    // throw an exception if this isn't valid.
    json versionJSON(gnuradioInfo());
}
