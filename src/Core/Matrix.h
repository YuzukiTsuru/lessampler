/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_MATRIX_H
#define LESSAMPLER_MATRIX_H

#include <cstddef>
#include <span>
#include <utility>
#include <vector>

namespace lessampler {

    /*
 * Row-major dense matrix backed by one contiguous allocation. Rows are exposed
 * as spans, so callers can read or write a row without copying it.
 */
    class Matrix {
    public:
        Matrix() = default;

        Matrix(std::size_t rows, std::size_t cols) : rows_(rows), cols_(cols), data_(rows * cols) {}

        Matrix(const Matrix &) = delete;
        Matrix &operator=(const Matrix &) = delete;

        Matrix(Matrix &&) noexcept = default;
        Matrix &operator=(Matrix &&) noexcept = default;

        [[nodiscard]] std::size_t rows() const noexcept { return rows_; }

        [[nodiscard]] std::size_t cols() const noexcept { return cols_; }

        [[nodiscard]] bool empty() const noexcept { return data_.empty(); }

        [[nodiscard]] std::span<double> operator[](std::size_t row) {
            return {data_.data() + row * cols_, cols_};
        }

        [[nodiscard]] std::span<const double> operator[](std::size_t row) const {
            return {data_.data() + row * cols_, cols_};
        }

        [[nodiscard]] std::span<double> row(std::size_t index) { return (*this)[index]; }

        [[nodiscard]] std::span<const double> row(std::size_t index) const { return (*this)[index]; }

        [[nodiscard]] double *data() noexcept { return data_.data(); }

        [[nodiscard]] const double *data() const noexcept { return data_.data(); }

        [[nodiscard]] std::vector<double> &values() noexcept { return data_; }

        [[nodiscard]] const std::vector<double> &values() const noexcept { return data_; }

        void assign(std::size_t rows, std::size_t cols, double value) {
            rows_ = rows;
            cols_ = cols;
            data_.assign(rows * cols, value);
        }

        void resize(std::size_t rows, std::size_t cols) {
            rows_ = rows;
            cols_ = cols;
            data_.resize(rows * cols);
        }

        /*
     * Temporary row-pointer view for the WORLD C API. No sample data is copied.
     */
        [[nodiscard]] std::vector<double *> row_pointers() {
            std::vector<double *> pointers;
            pointers.reserve(rows_);
            for (std::size_t i = 0; i < rows_; ++i) {
                pointers.push_back(data_.data() + i * cols_);
            }
            return pointers;
        }

        [[nodiscard]] std::vector<const double *> row_pointers() const {
            std::vector<const double *> pointers;
            pointers.reserve(rows_);
            for (std::size_t i = 0; i < rows_; ++i) {
                pointers.push_back(data_.data() + i * cols_);
            }
            return pointers;
        }

        /*
     * WORLD's AddParameters() takes double** even though it does not modify the
     * parameter rows. This view avoids copying the matrix for that API.
     */
        [[nodiscard]] std::vector<double *> mutable_row_pointers() const {
            std::vector<double *> pointers;
            pointers.reserve(rows_);
            for (std::size_t i = 0; i < rows_; ++i) {
                pointers.push_back(const_cast<double *>(data_.data() + i * cols_));
            }
            return pointers;
        }

    private:
        std::size_t rows_ = 0;
        std::size_t cols_ = 0;
        std::vector<double> data_;
    };

}// namespace lessampler

#endif//LESSAMPLER_MATRIX_H
