#pragma once
#pragma once

#include <algorithm>
#include <cstddef>
#include <format>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
using namespace std;

template <typename T, class Alloc = allocator<T>>
class Vector {
private:
    size_t capacity_;
    size_t size_;
    T* data_;
    [[no_unique_address]] Alloc alloc_;

    using AllocTraits = allocator_traits<Alloc>;

    void clearMemory() noexcept {
        if (data_) {
            std::destroy_n(data_, size_);
            AllocTraits::deallocate(alloc_, data_, capacity_);
            data_ = nullptr;
            size_ = 0;
            capacity_ = 0;
        }
    }

    void resize(size_t newCapacity) {
        size_t newCap = std::max(newCapacity, capacity_ == 0 ? 1 : capacity_ + (capacity_ >> 1) + 1);
        T* newData = AllocTraits::allocate(alloc_, newCap);
        size_t oldSize = size_;

        if constexpr (std::is_trivially_copyable_v<T>) {
            if (size_ > 0) {
                memcpy(newData, data_, oldSize);
            }
        }
        else {
            try {
                uninitialized_move_n(data_, size_, newData);
            }
            catch (...) {
                AllocTraits::deallocate(alloc_, data_, oldSize);
                throw;
            }
        }

        clearMemory();
        data_ = newData;
        size_ = oldSize;
        capacity_ = newCap;
    }

    template<typename... Args>
    T* create_object(T* where, Args&&... args) {
        AllocTraits::construct(alloc_, where, std::forward<Args>(args)...);
        return where;
    }

    void check_size(size_t new_size) const {
        if (new_size > AllocTraits::max_size(alloc_)) {
            throw length_error("Vector size would exceed maximum allocation size");
        }
    }

public:
    using value_type = T;
    using allocator_type = Alloc;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;

    Vector() noexcept(noexcept(Alloc()))
        : capacity_(10)
        , size_(0)
        , data_(AllocTraits::allocate(alloc_, capacity_, data_))
        , alloc_(Alloc()) {
    }

    explicit Vector(const Alloc& allocator) noexcept
        : capacity_(10)
        , size_(0)
        , data_(AllocTraits::allocate(allocator, capacity_, data_))
        , alloc_(allocator) {
    }

    Vector(size_t count, const T& value, const Alloc& allocator = Alloc())
    : capacity_(count)
    , size_(0)
    , data_(nullptr)
    , alloc_(allocator) {
        check_size(count);
        Alloc& nonConstAlloc = const_cast<Alloc&>(alloc_); // Temporarily cast away const
        data_ = AllocTraits::allocate(nonConstAlloc, count);
        try {
            std::uninitialized_fill_n(data_, count, value);
            size_ = count;
        }
        catch (...) {
            AllocTraits::deallocate(nonConstAlloc, data_, count);
            throw;
        }
    }


    explicit Vector(size_t count, const Alloc& allocator = Alloc())
        : capacity_(count)
        , size_(0)
        , data_(nullptr)
        , alloc_(allocator) {
        check_size(count);
        data_ = AllocTraits::allocate(allocator, count);
        try {
            std::uninitialized_default_construct_n(data_, count);
            size_ = count;
        }
        catch (...) {
            AllocTraits::deallocate(alloc_, data_, count);
            throw;
        }
    }

    Vector(const Vector& other)
        : capacity_(other.capacity_)
        , size_(0)
        , data_(nullptr)
        , alloc_(AllocTraits::select_on_container_copy_construction(other.alloc_)) {
        data_ = AllocTraits::allocate(alloc_, other.size_);
        try {
            std::uninitialized_copy_n(other.data_, other.size_, data_);
            size_ = other.size_;
        }
        catch (...) {
            AllocTraits::deallocate(alloc_, data_, other.size_);
            throw;
        }
    }

    Vector& operator=(const Vector& other) {
        if (this != &other) {
            AllocTraits::deallocate(alloc_, data_, size_);

            size_ = other.size_;
            capacity_ = other.capacity_;
            data_ = AllocTraits::allocate(alloc_, capacity_);

            std::uninitialized_copy(other.data_, other.data_ + other.size_, data_);
        }
        return *this;
    }

    Vector(Vector&& other) noexcept
        : capacity_(std::exchange(other.capacity_, 0))
        , size_(std::exchange(other.size_, 0))
        , data_(std::exchange(other.data_, nullptr))
        , alloc_(std::move(other.alloc_)) {
    }

    Vector& operator=(Vector&& other) noexcept {
        if (this != &other) {
            clearMemory();
            capacity_ = std::exchange(other.capacity_, 0);
            size_ = std::exchange(other.size_, 0);
            data_ = std::exchange(other.data_, nullptr);
            if (AllocTraits::propagate_on_container_move_assignment::value) {
                alloc_ = std::move(other.alloc_);
            }
        }
        return *this;
    }

    Vector(std::initializer_list<T> init, const Alloc& allocator = Alloc())
        : capacity_(init.size())
        , size_(0)
        , data_(nullptr)
        , alloc_(allocator) {
        data_ = AllocTraits::allocate(alloc_, init.size());
        try {
            std::uninitialized_copy(init.begin(), init.end(), data_);
            size_ = init.size();
        }
        catch (...) {
            AllocTraits::deallocate(alloc_, data_, init.size());
            throw;
        }
    }

    void swap(Vector& other) noexcept {
        //using std::swap;
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        std::swap(data_, other.data_);
        if (AllocTraits::propagate_on_container_swap::value) {
            swap(alloc_, other.alloc_);
        }
    }

    [[nodiscard]] allocator<T> get_allocator() const noexcept {
        return alloc_;
    }

    template <bool isConst>
    class baseIterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<isConst, const T*, T*>;
        using reference = std::conditional_t<isConst, const T&, T&>;

    private:
        T* ptr;

    public:
        baseIterator(T* p = nullptr) noexcept : ptr(p) {}
        baseIterator(const baseIterator& other) noexcept = default;
        baseIterator& operator=(const baseIterator& other) noexcept = default;

        reference operator*() const noexcept { return *ptr; }
        pointer operator->() const noexcept { return ptr; }
        reference operator[](difference_type n) const noexcept { return ptr[n]; }

        baseIterator& operator++() noexcept {
            ++ptr;
            return *this;
        }

        baseIterator operator++(int) noexcept {
            baseIterator tmp(*this);
            ++ptr;
            return tmp;
        }

        baseIterator& operator--() noexcept {
            --ptr;
            return *this;
        }

        baseIterator operator--(int) noexcept {
            baseIterator tmp(*this);
            --ptr;
            return tmp;
        }

        baseIterator& operator+=(difference_type n) noexcept {
            ptr += n;
            return *this;
        }

        baseIterator operator+(difference_type n) const noexcept {
            baseIterator tmp(*this);
            tmp += n;
            return tmp;
        }

        baseIterator& operator-=(difference_type n) noexcept {
            ptr -= n;
            return *this;
        }

        baseIterator operator-(difference_type n) const noexcept {
            baseIterator tmp(*this);
            tmp -= n;
            return tmp;
        }

        difference_type operator-(const baseIterator& other) const noexcept {
            return ptr - other.ptr;
        }

        bool operator==(const baseIterator& other) const noexcept {
            return ptr == other.ptr;
        }

        bool operator!=(const baseIterator& other) const noexcept {
            return !(*this == other);
        }

        bool operator<(const baseIterator& other) const noexcept {
            return ptr < other.ptr;
        }

        bool operator>(const baseIterator& other) const noexcept {
            return other < *this;
        }

        bool operator<=(const baseIterator& other) const noexcept {
            return !(other < *this);
        }

        bool operator>=(const baseIterator& other) const noexcept {
            return !(*this < other);
        }
    };

    friend baseIterator<false> operator+(typename baseIterator<false>::difference_type n,
        const baseIterator<false>& it) noexcept {
        return it + n;
    }

    friend baseIterator<true> operator+(typename baseIterator<true>::difference_type n,
        const baseIterator<true>& it) noexcept {
        return it + n;
    }

    template <bool isConst>
    class reverseIterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<isConst, const T*, T*>;
        using reference = std::conditional_t<isConst, const T&, T&>;

    private:
        T* ptr;

    public:
        reverseIterator(T* p = nullptr) noexcept : ptr(p) {}
        reverseIterator(const reverseIterator& other) noexcept = default;
        reverseIterator& operator=(const reverseIterator& other) noexcept = default;

        reference operator*() const noexcept { return *ptr; }
        pointer operator->() const noexcept { return ptr; }
        reference operator[](difference_type n) const noexcept { return ptr[-n]; }

        reverseIterator& operator++() noexcept {
            --ptr;
            return *this;
        }

        reverseIterator operator++(int) noexcept {
            reverseIterator tmp(*this);
            --ptr;
            return tmp;
        }

        reverseIterator& operator--() noexcept {
            ++ptr;
            return *this;
        }

        reverseIterator operator--(int) noexcept {
            reverseIterator tmp(*this);
            ++ptr;
            return tmp;
        }

        reverseIterator& operator+=(difference_type n) noexcept {
            ptr -= n;
            return *this;
        }

        reverseIterator operator+(difference_type n) const noexcept {
            reverseIterator tmp(*this);
            tmp += n;
            return tmp;
        }

        reverseIterator& operator-=(difference_type n) noexcept {
            ptr += n;
            return *this;
        }

        reverseIterator operator-(difference_type n) const noexcept {
            reverseIterator tmp(*this);
            tmp -= n;
            return tmp;
        }

        difference_type operator-(const reverseIterator& other) const noexcept {
            return other.ptr - ptr;
        }

        bool operator==(const reverseIterator& other) const noexcept {
            return ptr == other.ptr;
        }

        bool operator!=(const reverseIterator& other) const noexcept {
            return !(*this == other);
        }

        bool operator<(const reverseIterator& other) const noexcept {
            return ptr > other.ptr;
        }

        bool operator>(const reverseIterator& other) const noexcept {
            return other < *this;
        }

        bool operator<=(const reverseIterator& other) const noexcept {
            return !(other < *this);
        }

        bool operator>=(const reverseIterator& other) const noexcept {
            return !(*this < other);
        }
    };

    friend reverseIterator<false> operator+(typename reverseIterator<false>::difference_type n,
        const reverseIterator<false>& it) noexcept {
        return it + n;
    }

    friend reverseIterator<true> operator+(typename reverseIterator<true>::difference_type n,
        const reverseIterator<true>& it) noexcept {
        return it + n;
    }

    using Iterator = baseIterator<false>;
    using ConstIterator = baseIterator<true>;
    using RIterator = reverseIterator<false>;
    using ConstRIterator = reverseIterator<true>;

    Iterator begin() noexcept { return Iterator(data_); }
    Iterator end() noexcept { return Iterator(data_ + size_); }

    ConstIterator begin() const noexcept { return ConstIterator(data_); }
    ConstIterator end() const noexcept { return ConstIterator(data_ + size_); }

    ConstIterator cbegin() const noexcept { return ConstIterator(data_); }
    ConstIterator cend() const noexcept { return ConstIterator(data_ + size_); }

    RIterator rbegin() noexcept { return RIterator(data_ + size_ - 1); }
    RIterator rend() noexcept { return RIterator(data_ - 1); }

    ConstRIterator rbegin() const noexcept { return ConstRIterator(data_ + size_ - 1); }
    ConstRIterator rend() const noexcept { return ConstRIterator(data_ - 1); }

    ConstRIterator crbegin() const noexcept { return ConstRIterator(data_ + size_ - 1); }
    ConstRIterator crend() const noexcept { return ConstRIterator(data_ - 1); }

    void reserve(size_t newCapacity) {
        if (newCapacity > capacity_) {
            resize(newCapacity);
        }
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (size_ == capacity_) {
            size_t newCap = capacity_ == 0 ? 1 : capacity_ + (capacity_ >> 1) + 1;
            resize(newCap);
        }
        if constexpr (std::is_trivially_constructible_v<T, Args...>) {
            new (data_ + size_) T(std::forward<Args>(args)...);
        }
        else {
            create_object(data_ + size_, std::forward<Args>(args)...);
        }
        ++size_;
    }

    void push_back(const T& value) {
        emplace_back(value);
    }

    void push_back(T&& value) {
        emplace_back(std::move(value));
    }

    void push_back(initializer_list<T>&& init) {
        for (const auto&& i : init) {
            emplace_back(forward<decltype(i)>(i));
        }
    }

    void pop_back() {
        if (empty()) {
            throw std::out_of_range("Vector is empty");
        }

        if constexpr (std::is_trivially_destructible_v<T>) {
            (data_ + size_ - 1)->~T();
            --size_;
            return;
        }

        AllocTraits::destroy(alloc_, data_ + size_);
        --size_;
    }

    reference operator[](int index) {
        return *(data_ + index);
    }

    const_reference operator[](int index) const {
        return *(data_ + index);
    }

    reference back() {
        if (size_ <= 0) {
            throw out_of_range("Vector is empty");
        }
        return data_[size_ - 1];
    }

    const_reference back() const {
        if (size_ <= 0) {
            throw out_of_range("Vector is empty");
        }
        return data_[size_ - 1];
    }

    bool find(const T& element) const {
        return std::find(data_, data_ + size_, element) != data_ + size_;
    }

    size_t index(const T& element) const {
        auto it = std::find(data_, data_ + size_, element);
        return it != data_ + size_ ? static_cast<int>(it - data_) : -1;
    }

    void insert(const T& element, size_t index) {
        if (index > size_) {
            throw out_of_range(format("Index {} out of range (size: {})", index, size_));
        }

        if (size_ == capacity_) {
            size_t newCapacity = capacity_ == 0 ? 1 : capacity_ + (capacity_ >> 1);
            reserve(newCapacity);
        }

        if (index < size_) {
            std::move_backward(data_ + index, data_ + size_, data_ + size_ + 1);
        }

        create_object(data_ + index, element);
        ++size_;
    }

    void insert(const T& element, Iterator pos) {
        auto index = distance(begin(), pos);

        if (size_ == capacity_) {
            if (size_ == capacity_) {
                size_t newCapacity = capacity_ == 0 ? 1 : capacity_ + (capacity_ >> 1);
                reserve(newCapacity);
            }
        }

        if (index < size_) {
            std::move_backward(data_ + index, data_ + size_, data_ + size_ + 1);
        }

        create_object(data_ + index, element);
        ++size_;
    }

    void erase(size_t index) {
        if (index >= size_) {
            throw out_of_range("Index out of range");
        }
        std::move(data_ + index + 1, data_ + size_, data_ + index);
        AllocTraits::destroy(alloc_, data_ + size_ - 1);
        --size_;
    }

    Iterator erase(Iterator pos) {
        if (pos >= end() || pos < begin()) {
            throw std::out_of_range("Iterator out of range");
        }

        auto index = std::distance(begin(), pos);
        std::move(pos + 1, end(), pos);
        --size_;
        AllocTraits::destroy(alloc_, data_ + size_);
        return Iterator(data_ + index);
    }

    void erase(size_t first_index, size_t last_index) {
        if (first_index > last_index || last_index > size_) {
            throw out_of_range("Invalid index range");
        }

        if (first_index == last_index) {
            return;
        }

        auto count = last_index - first_index;
        std::move(data_ + last_index, data_ + size_, data_ + first_index);

        for (size_t i = 0; i < count; ++i) {
            AllocTraits::destroy(alloc_, data_ + size_ - 1 - i);
        }
        size_ -= count;
    }

    Iterator erase(Iterator first, Iterator last) {
        if (first > last || first < begin() || last > end()) {
            throw std::out_of_range("Invalid iterator range");
        }

        if (first == last) {
            return first;
        }

        auto index = std::distance(begin(), first);
        auto count = std::distance(first, last);

        std::move(last, end(), first);
        for (auto it = end() - count; it != end(); ++it) {
            AllocTraits::destroy(alloc_, std::addressof(*it));
        }
        size_ -= count;
        return Iterator(data_ + index);
    }

    [[nodiscard]] bool empty() const noexcept {
        return size_ == 0;
    }

    void clear() {
        destroy_n(data_, size_);
        size_ = 0;
    }

    [[nodiscard]] size_t size() const noexcept {
        return size_;
    }

    [[nodiscard]] size_t capacity() const noexcept {
        return capacity_;
    }

    [[nodiscard]] T& at(size_t index) {
        if (index >= size_) {
            throw out_of_range(format("Index {} out of range (size: {})", index, size_));
        }
        return data_[index];
    }

    [[nodiscard]] const T& at(size_t index) const {
        if (index >= size_) {
            throw out_of_range(format("Index {} out of range (size: {})", index, size_));
        }
        return data_[index];
    }

    void shrink_to_fit() {
        if (size_ < capacity_) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                if (T* newData = static_cast<T*>(std::realloc(data_, size_ * sizeof(T)))) {
                    data_ = newData;
                    capacity_ = size_;
                    return;
                }
            }
            T* newData = AllocTraits::allocate(alloc_, size_);
            std::uninitialized_move_n(data_, size_, newData);
            clearMemory();
            data_ = newData;
            capacity_ = size_;
        }
    }

    ~Vector() { clearMemory(); }
};
