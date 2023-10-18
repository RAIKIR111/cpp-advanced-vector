#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <stdexcept>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    RawMemory(const RawMemory&) = delete;

    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept 
        : buffer_(std::move(other.buffer_))
        , capacity_(other.capacity_) {

        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        std::destroy_n(GetAddress(), capacity_);
        buffer_ = std::move(rhs.buffer_);
        capacity_ = rhs.capacity_;
        rhs.buffer_ = nullptr;
        rhs.capacity_ = 0;
        return *this;
    }

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)  //
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
    }

    Vector(Vector&& rhs)
        : data_(RawMemory(std::move(rhs.data_))) {

        size_ = rhs.size_;
        rhs.size_ = 0;
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            } else {
                if (rhs.size_ < size_) {
                    for (size_t counter = 0; counter < rhs.size_; ++counter) {
                        data_[counter] = rhs.data_[counter];
                    }
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                }
                else {
                    for (size_t counter = 0; counter < size_; ++counter) {
                        data_[counter] = rhs.data_[counter];
                    }
                    std::uninitialized_copy_n(rhs.data_ + size_, rhs.size_ - size_, data_ + size_);
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) {
        if (this != &rhs) {
            Swap(rhs);
        }
        return *this;
    }

    void Swap(Vector& other) {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(data_ + new_size, (size_ - new_size));
            size_ = new_size;
        }
        else if (new_size > size_) {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_ + size_, new_size - size_);
            size_ = new_size;
        }
    }

    void PushBack(const T& value) {
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ != 0 ? (Capacity() * 2) : 1);
            new (new_data + size_) T(value);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }

            data_.Swap(new_data);
            std::destroy_n(new_data.GetAddress(), size_);
        }
        else {
            new (data_ + size_) T(value);
        }
        size_ += 1;
    }
    
    void PushBack(T&& value) {
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ != 0 ? (Capacity() * 2) : 1);
            new (new_data + size_) T(std::move(value));
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }

            data_.Swap(new_data);
            std::destroy_n(new_data.GetAddress(), size_);
        }
        else {
            new (data_ + size_) T(std::move(value));
        }
        size_ += 1;
    }

    void PopBack() noexcept {
        std::destroy_n(data_ + (size_ - 1), 1);
        size_ -= 1;
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ != 0 ? (Capacity() * 2) : 1);
            new (new_data + size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }

            data_.Swap(new_data);
            std::destroy_n(new_data.GetAddress(), size_);
        }
        else {
            new (data_ + size_) T(std::forward<Args>(args)...);
        }
        size_ += 1;
        return data_[size_ - 1];
    }

    using iterator = T*;
    using const_iterator = const T*;
    
    iterator begin() noexcept {
        return data_ + 0;
    }
    iterator end() noexcept {
        return data_ + size_;
    }
    const_iterator begin() const noexcept {
        return data_ + 0;
    }
    const_iterator end() const noexcept {
        return data_ + size_;
    }
    const_iterator cbegin() const noexcept {
        return data_ + 0;
    }
    const_iterator cend() const noexcept {
        return data_ + size_;
    }

    iterator Insert(const_iterator pos, const T& value) {
        const size_t dist_save = std::distance(begin(), iterator(pos));
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ != 0 ? (Capacity() * 2) : 1);

            new (new_data + dist_save) T(value);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move(begin(), data_ + dist_save, new_data.GetAddress());
                std::uninitialized_move(begin() + dist_save, end(), new_data + dist_save + 1);
            }
            else {
                try {
                    std::uninitialized_copy(begin(), data_ + dist_save, new_data.GetAddress());
                }
                catch (...) {
                    std::destroy_n(new_data + dist_save, 1);
                    throw ("Execption!");
                }
                try {
                    std::uninitialized_copy(begin() + dist_save, end(), new_data + dist_save + 1);
                }
                catch (...) {
                    std::destroy_n(new_data.GetAddress(), dist_save + 1);
                    throw ("Execption!");
                }
            }

            data_.Swap(new_data);
            std::destroy_n(new_data.GetAddress(), size_);
        }
        else {
            if (size_ == 0) {
                new (iterator(pos)) T(value);
            }
            else {
                new (data_ + size_) T(std::move(data_[size_ - 1]));
                std::move_backward(iterator(pos), end(), end() + 1);

                std::destroy_n(begin() + dist_save, 1);
                new (begin() + dist_save) T(value);
            }
        }
        size_ += 1;
        return iterator(begin() + dist_save);
    }

    iterator Insert(const_iterator pos, T&& value) {
        const size_t dist_save = std::distance(begin(), iterator(pos));
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ != 0 ? (Capacity() * 2) : 1);

            new (new_data + dist_save) T(std::move(value));
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move(begin(), data_ + dist_save, new_data.GetAddress());
                std::uninitialized_move(begin() + dist_save, end(), new_data + dist_save + 1);
            }
            else {
                try {
                    std::uninitialized_copy(begin(), data_ + dist_save, new_data.GetAddress());
                }
                catch (...) {
                    std::destroy_n(new_data + dist_save, 1);
                    throw ("Execption!");
                }
                try {
                    std::uninitialized_copy(begin() + dist_save, end(), new_data + dist_save + 1);
                }
                catch (...) {
                    std::destroy_n(new_data.GetAddress(), dist_save + 1);
                    throw ("Execption!");
                }
            }

            data_.Swap(new_data);
            std::destroy_n(new_data.GetAddress(), size_);
        }
        else {
            if (size_ == 0) {
                new (iterator(pos)) T(std::move(value));
            }
            else {
                new (data_ + size_) T(std::move(data_[size_ - 1]));
                std::move_backward(iterator(pos), end(), end() + 1);

                std::destroy_n(begin() + dist_save, 1);
                new (begin() + dist_save) T(std::move(value));
            }
        }
        size_ += 1;
        return iterator(begin() + dist_save);
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        const size_t dist_save = std::distance(begin(), iterator(pos));
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ != 0 ? (Capacity() * 2) : 1);

            new (new_data + dist_save) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move(begin(), data_ + dist_save, new_data.GetAddress());
                std::uninitialized_move(begin() + dist_save, end(), new_data + dist_save + 1);
            }
            else {
                try {
                    std::uninitialized_copy(begin(), data_ + dist_save, new_data.GetAddress());
                }
                catch (...) {
                    std::destroy_n(new_data + dist_save, 1);
                    throw ("Execption!");
                }
                try {
                    std::uninitialized_copy(begin() + dist_save, end(), new_data + dist_save + 1);
                }
                catch (...) {
                    std::destroy_n(new_data.GetAddress(), dist_save + 1);
                    throw ("Execption!");
                }
            }

            data_.Swap(new_data);
            std::destroy_n(new_data.GetAddress(), size_);
        }
        else {
            if (size_ == 0) {
                new (iterator(pos)) T(std::forward<Args>(args)...);
            }
            else {
                new (data_ + size_) T(std::move(data_[size_ - 1]));
                std::move_backward(iterator(pos), end(), end() + 1);

                std::destroy_n(begin() + dist_save, 1);
                new (begin() + dist_save) T(std::forward<Args>(args)...);
            }
        }
        size_ += 1;
        return iterator(begin() + dist_save);
    }

    iterator Erase(const_iterator pos) {
        const size_t dist_save = std::distance(begin(), iterator(pos));
        std::move(iterator(pos) + 1, end(), iterator(pos));
        std::destroy_n(data_ + (size_ - 1), 1);
        size_ -= 1;
        return iterator(begin() + dist_save);
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};