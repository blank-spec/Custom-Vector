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
    size_t capacity;
    size_t size;
    T* data;
    [[no_unique_address]] Alloc alloc;

    using AllocTraits = allocator_traits<Alloc>;

    void clearMemory() noexcept {
        if (data) {
            std::destroy_n(data, size);
            AllocTraits::deallocate(alloc, data, capacity);
            data = nullptr;
            size = 0;
            capacity = 0;
        }
    }

    void resize(size_t newCapacity) {
        size_t newCap = std::max(newCapacity, capacity == 0 ? 1 : capacity + (capacity >> 1) + 1);
        T* newData = AllocTraits::allocate(alloc, newCap);
        size_t oldSize = size;

        if constexpr (std::is_trivially_copyable_v<T>) {
            if (size > 0 ) {
                memcpy(newData, data, oldSize);
            }
        }
        else {
            try {
                uninitialized_move_n(data, size, newData);
            }
            catch (...) {
                AllocTraits::deallocate(alloc, data, oldSize);
                throw;
            }
        }

        clearMemory();
        data = newData;
        size = oldSize;
        capacity = newCap;
    }

    template<typename... Args>
    T* create_object(T* where, Args&&... args) {
        AllocTraits::construct(alloc, where, std::forward<Args>(args)...);
        return where;
    }

    void check_size(size_t new_size) const {
        if (new_size > AllocTraits::max_size(alloc)) {
            throw length_error("Vector size would exceed maximum allocation size");
        }
    }

public:
    Vector() noexcept(noexcept(Alloc()))
        : capacity(10)
        , size(0)
        , data(AllocTraits::allocate(alloc, capacity, data))
        , alloc(Alloc()) {}

    explicit Vector(const Alloc& allocator) noexcept
        : capacity(10)
        , size(0)
        , data(AllocTraits::allocate(alloc, capacity, data))
        , alloc(allocator) {}

    Vector(size_t count, const T& value, const Alloc& allocator = Alloc())
        : capacity(count)
        , size(0)
        , data(nullptr)
        , alloc(allocator) {
        check_size(count);
        data = AllocTraits::allocate(allocator, count);
        try {
            std::uninitialized_fill_n(data, count, value);
            size = count;
        }
        catch (...) {
            AllocTraits::deallocate(alloc, data, count);
            throw;
        }
    }

    explicit Vector(size_t count, const Alloc& allocator = Alloc())
        : capacity(count)
        , size(0)
        , data(nullptr)
        , alloc(allocator) {
        check_size(count);
        data = AllocTraits::allocate(allocator, count);
        try {
            std::uninitialized_default_construct_n(data, count);
            size = count;
        }
        catch (...) {
            AllocTraits::deallocate(alloc, data, count);
            throw;
        }
    }

    Vector(const Vector& other)
        : capacity(other.size)
        , size(0)
        , data(nullptr)
        , alloc(AllocTraits::select_on_container_copy_construction(other.alloc)) {
        data = AllocTraits::allocate(alloc, other.size);
        try {
            std::uninitialized_copy_n(other.data, other.size, data);
            size = other.size;
        }
        catch (...) {
            AllocTraits::deallocate(alloc, data, other.size);
            throw;
        }
    }

    Vector& operator=(const Vector& other) {
        if (this != &other) {
            if (AllocTraits::propagate_on_container_copy_assignment::value && alloc != other.alloc) {
                Vector tmp(other);
                swap(tmp);
            }
            else {
                Vector tmp(other, alloc);
                swap(tmp);
            }
        }
        return *this;
    }

    Vector(Vector&& other) noexcept
        : capacity(std::exchange(other.capacity, 0))
        , size(std::exchange(other.size, 0))
        , data(std::exchange(other.data, nullptr))
        , alloc(std::move(other.alloc)) {}

    Vector& operator=(Vector&& other) noexcept {
        if (this != &other) {
            clearMemory();
            capacity = std::exchange(other.capacity, 0);
            size = std::exchange(other.size, 0);
            data = std::exchange(other.data, nullptr);
            if (AllocTraits::propagate_on_container_move_assignment::value) {
                alloc = std::move(other.alloc);
            }
        }
        return *this;
    }

    Vector(std::initializer_list<T> init, const Alloc& allocator = Alloc())
        : capacity(init.size())
        , size(0)
        , data(nullptr)
        , alloc(allocator) {
        data = AllocTraits::allocate(alloc, init.size());
        try {
            std::uninitialized_copy(init.begin(), init.end(), data);
            size = init.size();
        }
        catch (...) {
            AllocTraits::deallocate(alloc, data, init.size());
            throw;
        }
    }

    void swap(Vector& other) noexcept {
        using std::swap;
        swap(size, other.size);
        swap(capacity, other.capacity);
        swap(data, other.data);
        if (AllocTraits::propagate_on_container_swap::value) {
            swap(alloc, other.alloc);
        }
    }

    [[nodiscard]] allocator<T> get_allocator() const noexcept {
        return alloc;
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

    Iterator begin() noexcept { return Iterator(data); }
    Iterator end() noexcept { return Iterator(data + size); }

    ConstIterator begin() const noexcept { return ConstIterator(data); }
    ConstIterator end() const noexcept { return ConstIterator(data + size); }

    ConstIterator cbegin() const noexcept { return ConstIterator(data); }
    ConstIterator cend() const noexcept { return ConstIterator(data + size); }

    RIterator rbegin() noexcept { return RIterator(data + size - 1); }
    RIterator rend() noexcept { return RIterator(data - 1); }

    ConstRIterator rbegin() const noexcept { return ConstRIterator(data + size - 1); }
    ConstRIterator rend() const noexcept { return ConstRIterator(data - 1); }

    ConstRIterator crbegin() const noexcept { return ConstRIterator(data + size - 1); }
    ConstRIterator crend() const noexcept { return ConstRIterator(data - 1); }

    ~Vector() { clearMemory(); }

    void reserve(size_t newCapacity) {
        if (newCapacity > capacity) {
            resize(newCapacity);
        }
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (size == capacity) {
            size_t newCap = capacity == 0 ? 1 : capacity + (capacity >> 1) + 1;
            resize(newCap);
        }
        if constexpr (std::is_trivially_constructible_v<T, Args...>) {
            new (data + size) T(std::forward<Args>(args)...);
        } else {
            create_object(data + size, std::forward<Args>(args)...);
        }
        ++size;
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
            (data + size - 1)->~T();
        }

        AllocTraits::destroy(alloc, data + size);
        --size;
    }

    T& operator[](int index) const {
        return *(data + index);
    }

    T& back() const {
        if (size <= 0) {
            throw out_of_range("Vector is empty");
        }
        return data[size - 1];
    }

    bool find(const T& element) const {
        return std::find(data, data + size, element) != data + size;
    }

    int index(const T& element) const {
        auto it = std::find(data, data + size, element);
        return it != data + size ? static_cast<int>(it - data) : -1;
    }

    void insert(const T& element, size_t index) {
        if (index > size) {
            throw out_of_range(format("Index {} out of range (size: {})", index, size));
        }

        if (size == capacity) {
            size_t newCapacity = capacity == 0 ? 1 : capacity + (capacity >> 1);
            reserve(newCapacity);
        }

        if (index < size) {
            std::move_backward(data + index, data + size, data + size + 1);
        }

        create_object(data + index, element);
        ++size;
    }

    void erase(size_t index) {
        if (index >= size) {
            throw out_of_range("Index out of range");
        }
        std::move(data + index + 1, data + size, data + index);
        AllocTraits::destroy(alloc, data + size - 1);
        --size;
    }

    Iterator erase(Iterator pos) {
        if (pos >= end() || pos < begin()) {
            throw std::out_of_range("Iterator out of range");
        }

        auto index = std::distance(begin(), pos);
        std::move(pos + 1, end(), pos);
        --size;
        AllocTraits::destroy(alloc, data + size);
        return Iterator(data + index);
    }

    void erase(size_t first_index, size_t last_index) {
        if (first_index > last_index || last_index > size) {
            throw out_of_range("Invalid index range");
        }

        if (first_index == last_index) {
            return ;
        }

        auto count = last_index - first_index;
        std::move(data + last_index, data + size, data + first_index);

        for (size_t i = 0; i < count; ++i) {
            AllocTraits::destroy(alloc, data + size - 1 - i);
        }
        size -= count;
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
            AllocTraits::destroy(alloc, std::addressof(*it));
        }
        size -= count;
        return Iterator(data + index);
    }

    [[nodiscard]] bool empty() const noexcept {
        return size == 0;
    }

    void clear() {
        destroy_n(data, size);
        size = 0;
    }

    [[nodiscard]] size_t getSize() const noexcept {
        return size;
    }

    [[nodiscard]] size_t getCapacity() const noexcept {
        return capacity;
    }

    [[nodiscard]] T& at(size_t index) {
        if (index >= size) {
            throw out_of_range(format("Index {} out of range (size: {})", index, size));
        }
        return data[index];
    }

    [[nodiscard]] const T& at(size_t index) const {
        if (index >= size) {
            throw out_of_range(format("Index {} out of range (size: {})", index, size));
        }
        return data[index];
    }

    void shrink_to_fit() {
        if (size < capacity) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                if (T* newData = static_cast<T*>(std::realloc(data, size * sizeof(T)))) {
                    data = newData;
                    capacity = size;
                    return;
                }
            }
            T* newData = AllocTraits::allocate(alloc, size);
            std::uninitialized_move_n(data, size, newData);
            clearMemory();
            data = newData;
            capacity = size;
        }
    }
};
