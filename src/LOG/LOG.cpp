/*
 * Copyright (c)  2022, YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "LOG.h"
#include "exception.h"
#include <ColorCout.hpp>

[[maybe_unused]] void LOG::INFO(const std::string &msg) {
    std::cout << cc::cyan << "[" << PROJECT_NAME << " INFO] " << msg << cc::reset << std::endl;
}

[[maybe_unused]] void LOG::DEBUG(const std::string &msg) {
    std::cout << cc::white << "[" << PROJECT_NAME << " DEBUG] " << msg << cc::reset << std::endl;
}

[[maybe_unused]] void LOG::WARNING(const std::string &msg) {
    std::cout << cc::yellow << "[" << PROJECT_NAME << " WARNING] " << msg << cc::reset << std::endl;
}

// expected unqualified-id before numeric constant fix.
#ifdef __WINDOWS__
#ifdef ERROR
#undef ERROR

[[maybe_unused]] void LOG::ERROR(const std::string &msg) {
    std::cout << cc::red << "[" << PROJECT_NAME << " ERROR] " << msg << cc::reset << std::endl;
    throw std::runtime_error(msg);
}

#endif
#endif
