#pragma once

#include <algorithm>
#include <cstddef>
#include <format>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
using namespace std;

template <typename T, class Alloc = allocator<T>>
class Vector {
private:
    unsigned int capacity;
    size_t size;
    T* data;
    Alloc alloc;

    using AllocTraits = allocator_traits<Alloc>;

    void clearMemory() {
        if (data) {
            for (size_t i = 0; i < size; ++i) {
                AllocTraits::destroy(alloc, data + i);
            }
        }
        AllocTraits::deallocate(alloc, data, capacity);
        data = nullptr;
    }

    void resize(int newCapacity) noexcept {
        if (newCapacity > capacity) {
            T* newData = AllocTraits::allocate(alloc, newCapacity);
            for (size_t i = 0; i < size; ++i) {
                AllocTraits::construct(alloc, newData + i, data[i]);
            }
            clearMemory();
            data = newData;
            capacity = newCapacity;
        }
    }

public:
    template <bool isConst>
    class baseIterator {
    private:
        T* ptr;

    public:
        baseIterator(T* p) : ptr(p) {}

        using pointerType = conditional_t<isConst, const T*, T*>;
        using referenceType = conditional_t<isConst, const T&, T&>;
        using valueType = T;
        using difference_type = ptrdiff_t;
        using iterator_category = random_access_iterator_tag;

        baseIterator(const baseIterator&) = default;
        baseIterator& operator=(const baseIterator&) = default;

        referenceType operator*() const { return *ptr; }
        pointerType operator->() const { return ptr; }

        referenceType operator[](difference_type n) const {
            return *(ptr + n);
        }

        baseIterator& operator++() {
            ++ptr;
            return *this;
        }

        baseIterator operator++(int) {
            baseIterator tmp = *this;
            ++ptr;
            return tmp;
        }

        baseIterator& operator--() {
            --ptr;
            return *this;
        }

        baseIterator operator--(int) {
            baseIterator tmp = *this;
            --ptr;
            return tmp;
        }

        baseIterator operator-(difference_type n) const {
            return baseIterator(ptr - n);
        }

        baseIterator operator+(difference_type n) const {
            return baseIterator(ptr + n);
        }

        difference_type operator-(const baseIterator& other) const {
            return ptr - other.ptr;
        }

        bool operator==(const baseIterator& other) const { return ptr == other.ptr; }
        bool operator!=(const baseIterator& other) const { return ptr != other.ptr; }
        bool operator<(const baseIterator& other) const { return ptr < other.ptr; }
        bool operator<=(const baseIterator& other) const { return ptr <= other.ptr; }
        bool operator>(const baseIterator& other) const { return ptr > other.ptr; }
        bool operator>=(const baseIterator& other) const { return ptr >= other.ptr; }
    };

    template <bool isConst>
    class reverseIterator {
    private:
        T* ptr;

    public:
        reverseIterator(T* p) : ptr(p) {}

        using pointerType = conditional_t<isConst, const T*, T*>;
        using referenceType = conditional_t<isConst, const T&, T&>;
        using valueType = T;
        using difference_type = ptrdiff_t;
        using iterator_category = random_access_iterator_tag;

        reverseIterator(const reverseIterator&) = default;
        reverseIterator& operator=(const reverseIterator&) = default;

        referenceType operator*() const { return *ptr; }
        pointerType operator->() const { return ptr; }

        referenceType operator[](difference_type n) const {
            return *(ptr - n);
        }

        reverseIterator& operator++() {
            --ptr;
            return *this;
        }

        reverseIterator operator++(int) {
            reverseIterator tmp = *this;
            --ptr;
            return tmp;
        }

        reverseIterator& operator--() {
            ++ptr;
            return *this;
        }

        reverseIterator operator--(int) {
            reverseIterator tmp = *this;
            ++ptr;
            return tmp;
        }

        reverseIterator operator-(difference_type n) const {
            return reverseIterator(ptr + n);
        }

        reverseIterator operator+(difference_type n) const {
            return reverseIterator(ptr - n);
        }

        bool operator==(const reverseIterator& other) const { return ptr == other.ptr; }
        bool operator!=(const reverseIterator& other) const { return ptr != other.ptr; }
        bool operator<(const reverseIterator& other) const { return ptr < other.ptr; }
        bool operator<=(const reverseIterator& other) const { return ptr <= other.ptr; }
        bool operator>(const reverseIterator& other) const { return ptr > other.ptr; }
        bool operator>=(const reverseIterator& other) const { return ptr >= other.ptr; }
    };

    using Iterator = baseIterator<false>;
    using constIterator = baseIterator<true>;
    using RIterator = reverseIterator<false>;
    using constRIterator = reverseIterator<true>;

    Iterator begin() {
        return { data };
    }

    Iterator end() {
        return { data + size };
    }

    constIterator begin() const {
        return { data };
    }

    constIterator end() const {
        return { data + size };
    }
    
    RIterator rbegin() {
        return { data + size - 1 };
    }

    constRIterator rbegin() const {
        return { data + size - 1};
    }

    RIterator rend() {
        return { data - 1 };
    }

    constRIterator rend() const {
        return { data - 1 };
    }

    Vector() : alloc(Alloc()), size(0), capacity(5), data(AllocTraits::allocate(alloc, capacity)) {}

    Vector(std::initializer_list<T> init) : Vector() {
        reserve(init.size());
        for (auto&& elem : init) {
            emplace_back(elem);
        }
    }

    Vector(int size) : alloc(Alloc()), size(size), capacity(size * 2), data(AllocTraits::allocate(alloc, capacity)) {}

    ~Vector() { clearMemory(); }

    Vector(Vector&& other) noexcept
        : size(other.size), capacity(other.capacity), data(other.data) {
        other.size = 0;
        other.capacity = 0;
        other.data = nullptr;
    }

    Vector& operator=(Vector&& other) noexcept {
        if (this != &other) {
            clearMemory();

            size = other.size;
            capacity = other.capacity;
            data = exchange(other.data, nullptr);
            alloc = move(other.alloc);

            other.size = 0;
            other.cap = 0;
        }
        return *this;
    }

    Vector(const Vector& other) noexcept {
        capacity = other.capacity;
        size = other.size;
        alloc = other.alloc;
        data = AllocTraits::allocate(alloc, capacity);

        for (size_t i = 0; i < size; ++i) {
            AllocTraits::construct(alloc, data + i, move(other.data[i]));
        }
    }

    Vector& operator=(const Vector& other) noexcept {
        if (this != &other) {
            clearMemory();

            size = other.size;
            capacity = other.capacity;
            alloc = other.alloc;
            data = AllocTraits::allocate(alloc, capacity);

            for (size_t i = 0; i < size; ++i) {
                AllocTraits::construct(alloc, data + i, move(other.data[i]));
            }
        }
        return *this;
    }

    void reserve(int newCapacity) {
        if (newCapacity > capacity) {
            resize(newCapacity);
        }
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (size == capacity) {
            resize(capacity * 2);
        }
        AllocTraits::construct(alloc, data + size, std::forward<Args>(args)...);
        ++size;
    }

    void push_back(const T& value) {
        emplace_back(value);
    }

    void push_back(T&& value) {
        emplace_back(std::move(value));
    }

    void push_back(initializer_list<T>&& init) {
        for (const auto& i : init) {
            emplace_back(i);
        }
    }

    void pop_back() {
        if (size > 0) {
            size--;
            AllocTraits::destroy(alloc, data + size);
        }
        else {
            throw out_of_range("Vector is empty");
        }
    }

    T& operator[](int index) const {
        if (index < 0 || index >= size) {
            throw out_of_range("Index out of range");
        }
        return data[index];
    }

    T& back() const {
        if (size <= 0) {
            throw out_of_range("Vector is empty");
        }
        return data[size - 1];
    }

    void print() const {
        if (size == 0) {
            cout << "{}" << endl;
        }

        for (size_t i = 0; i < size; ++i) {
            cout << data[i] << ' ';
        }
        cout << endl;
    }

    bool find(const T& element) const {
        for (int i = 0; i < size; ++i) {
            if (data[i] == element) {
                return true;
            }
        }
        return false;
    }

    int index(const T& element) const {
        for (int i = 0; i < size; ++i) {
            if (data[i] == element) {
                return i;
            }
        }
        return -1;
    }

    void insert(const T& element, int index) {
        try {
            if (index < 0 || index > size) {
                throw out_of_range("Index out of bound");
            }

            if (size == capacity) {
                resize(size * 2);
            }

            for (int i = size; i > index; --i) {
                data[i] = std::move(data[i - 1]);
            }

            AllocTraits::construct(alloc, data + index, element);
            size++;
        }
        catch (const out_of_range& e) {
            cout << format("While inserting elememt at index {} happend an error: ", index) << e.what() << endl;
        }
    }

    void erase(int index) {
        try {
            if (index < 0 || index >= size) {
                throw out_of_range("Index out of range");
            }
            else {
                AllocTraits::destroy(alloc, data + index);

                for (int i = index; i < size; ++i) {
                    data[i] = std::move(data[i + 1]);
                }
                --size;
            }
        }
        catch (const out_of_range& e) {
            cout << format("While removing elememt at index {} happend an error: ", index) << e.what() << endl;
        }
    }

    void clear() {
        for (int i = 0; i < size; ++i) {
            AllocTraits::destroy(alloc, data + i);
        }
        size = 0;
    }

    int getSize() const { return size; }
    int getCapacity() const { return capacity; }
};
