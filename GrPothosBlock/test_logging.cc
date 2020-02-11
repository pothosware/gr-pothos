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

#ifdef ENABLE_GR_LOG

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>

#include <Poco/File.h>
#include <Poco/Logger.h>
#include <Poco/SimpleFileChannel.h>
#include <Poco/TemporaryFile.h>

#include <gnuradio/block.h>

#include <vector>

POTHOS_TEST_BLOCK("/gnuradio/tests", test_logging)
{
    const std::vector<std::vector<float>> initialMatrix = {{1.0,0.0},{0.0,1.0}};
    const std::vector<std::vector<float>> invalidMatrix = {{1.0,0.0},{0.0,1.0},{0.0,1.0}};

    constexpr gr::block::tag_propagation_policy_t tagPropagationPolicy = gr::block::TPP_ALL_TO_ALL;
    const std::string loggerName = "multiply_matrix_ff";
    const std::string errorMessage = "Attempted to set matrix with invalid dimensions.";

    // Add a channel to the relevant Poco logger to write the output
    // to file. This will allow us to make sure output is written as
    // expected.
    const std::string logPath = Poco::TemporaryFile::tempName();
    POTHOS_TEST_TRUE(!Poco::File(logPath).exists());
    Poco::TemporaryFile::registerForDeletion(logPath);

    auto& grBlockLogger = Poco::Logger::get(loggerName);
    grBlockLogger.setChannel(new Poco::SimpleFileChannel(logPath));

    // This block was chosen because we can reliably trigger its logger
    // by specifying an invalid matrix.
    auto multiplyMatrixFF = Pothos::BlockRegistry::make(
                                "/gr/blocks/multiply_matrix_ff",
                                initialMatrix,
                                tagPropagationPolicy);

    // This call should log an error message saying the matrix is of an invalid size.
    multiplyMatrixFF.call("set_A", invalidMatrix);
    POTHOS_TEST_TRUE(Poco::File(logPath).exists());

    // Get the contents of the log file.
    std::string fileContents;
    const auto fileSize = Poco::File(logPath).getSize();
    fileContents.resize(fileSize);

    std::fstream ifile(logPath.c_str(), std::ios::in);
    ifile.read(const_cast<char*>(fileContents.c_str()), fileSize);
    ifile.close();

    const std::vector<std::string> expectedStrings =
    {
        loggerName,
        errorMessage
    };
    for(const auto& expectedString: expectedStrings)
    {
        POTHOS_TEST_TRUE(std::string::npos != fileContents.find(expectedString));
    }
}

#endif
