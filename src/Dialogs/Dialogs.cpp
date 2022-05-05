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

//
// Created by gloom on 2022/5/2.
//

#include "Dialogs.h"
#include "LOG.h"

Dialogs::Dialogs() {
    // Check that a backend is available
    if (!pfd::settings::available()) {
        YALL_WARN_ << ("Portable File Dialogs are not available on this platform.";
    }
    // Set verbosity to true
    pfd::settings::verbose(true);
}

void Dialogs::notify(const std::string &message, const std::string &title) {
    // Notification
    pfd::notify(title, message, pfd::icon::info);
}
