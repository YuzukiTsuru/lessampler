/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/5/2.
//

#include "Dialogs.h"
#include "Utils/LOG.h"

Dialogs::Dialogs() {
    // Check that a backend is available
    if (!pfd::settings::available()) {
        YALL_WARN_ << "Portable File Dialogs are not available on this platform.";
    }
    // Set verbosity to true
    pfd::settings::verbose(true);
}

void Dialogs::notify(const std::string &message, const std::string &title) {
    // Notification
    pfd::notify(title, message, pfd::icon::info);
}
