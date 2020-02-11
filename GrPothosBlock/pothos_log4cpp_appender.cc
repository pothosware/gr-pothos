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

#include "pothos_log4cpp_appender.h"

#include <Poco/Message.h>

#include <unordered_map>

PothosLog4CppAppender::PothosLog4CppAppender(const std::string& name):
    log4cpp::LayoutAppender(name),
    pocoLogger(Poco::Logger::get(name))
{
}

PothosLog4CppAppender::~PothosLog4CppAppender() {}

void PothosLog4CppAppender::close() {}

static inline int coercePriority(int priority)
{
    return log4cpp::Priority::getPriorityValue(log4cpp::Priority::getPriorityName(priority));
}

void PothosLog4CppAppender::_append(const log4cpp::LoggingEvent& loggingEvent)
{
    std::string message(_getLayout().format(loggingEvent));

    using Log4CppPriority = log4cpp::Priority::PriorityLevel;
    using PocoPriority = Poco::Message::Priority;

    static const std::unordered_map<Log4CppPriority, PocoPriority> priorityMap =
    {
        {log4cpp::Priority::EMERG,  Poco::Message::PRIO_FATAL},
        {log4cpp::Priority::ALERT,  Poco::Message::PRIO_FATAL},
        {log4cpp::Priority::CRIT,   Poco::Message::PRIO_CRITICAL},
        {log4cpp::Priority::ERROR,  Poco::Message::PRIO_ERROR},
        {log4cpp::Priority::WARN,   Poco::Message::PRIO_WARNING},
        {log4cpp::Priority::NOTICE, Poco::Message::PRIO_NOTICE},
        {log4cpp::Priority::INFO,   Poco::Message::PRIO_INFORMATION},
        {log4cpp::Priority::DEBUG,  Poco::Message::PRIO_DEBUG},
        {log4cpp::Priority::NOTSET, Poco::Message::PRIO_TRACE},
    };

    // This coerces the logging event's priority to one in the map above.
    auto coercedLog4CppPriority = static_cast<Log4CppPriority>(coercePriority(loggingEvent.priority));
    assert(0 < priorityMap.count(coercedLog4CppPriority));
    auto pocoPriority = priorityMap.at(coercedLog4CppPriority);

    pocoLogger.log(Poco::Message(this->getName(), message, pocoPriority));
}
