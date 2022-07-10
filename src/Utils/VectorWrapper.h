/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/6/18.
//

#ifndef LESSAMPLER_VECTORWRAPPER_H
#define LESSAMPLER_VECTORWRAPPER_H

#include <iostream>
#include <vector>

template<typename T, int N>
struct VectorWrapper {
    explicit VectorWrapper(T (&D)[N]) {
        std::copy(D, D + N, std::back_inserter(v));
    }

    std::vector<T> v;
};

#endif //LESSAMPLER_VECTORWRAPPER_H
