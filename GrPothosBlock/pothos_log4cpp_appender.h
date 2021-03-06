/*
 * Copyright 2019 Free Software Foundation, Inc.
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

#include <Poco/Logger.h>

#include <log4cpp/LayoutAppender.hh>

#include <string>

class PothosLog4CppAppender: public log4cpp::LayoutAppender
{
    public:
        PothosLog4CppAppender(const std::string& name);
        virtual ~PothosLog4CppAppender();

        void close() override;

        void _append(const log4cpp::LoggingEvent& loggingEvent) override;

    private:
        Poco::Logger& pocoLogger;
};
