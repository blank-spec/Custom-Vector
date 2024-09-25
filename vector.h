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

    baseIterator(const baseIterator&) = default;
    baseIterator& operator=(const baseIterator&) = default;

    referenceType operator*() const { return *ptr; }
    pointerType operator->() const { return ptr; }

    baseIterator& operator++() {
      ++ptr;
      return *this;
    }

    baseIterator& operator++(int) {
      baseIterator tmp = *this;
      ++ptr;
      return tmp;
    }

    bool operator==(const baseIterator& other) const { return ptr == other.ptr; }
    bool operator!=(const baseIterator& other) const { return ptr != other.ptr; }
  };

  using Iterator = baseIterator<false>;
  using constIterator = baseIterator<true>;

  Iterator begin() {
    return {data};
  }

  Iterator end() {
    return {data + size};
  }

  constIterator begin() const {
    return {data};
  }

  constIterator end() const {
    return {data + size};
  }

  Vector() : alloc(Alloc()), size(0), capacity(5), data(AllocTraits::allocate(alloc, capacity)) {}

  Vector(std::initializer_list<T> init) : Vector() {
    for (const auto& elem : init) {
      emplace_back(elem);
    }
  }

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
      data = other.data;

      other.size = 0;
      other.capacity = 0;
      other.data = nullptr;
    }
    return *this;
  }

  Vector(const Vector&) = delete;
  Vector& operator=(const Vector&) = delete;

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

  template <typename... Args>
  void push_back(Args&&... args) {
    (emplace_back(std::forward<Args>(args)), ...);
  }

  void pop_back() {
    if (size > 0) {
      size--;
      AllocTraits::destroy(alloc, data + size);
    } else {
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
      if (index < 0 || index >= size) {
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
    } catch (const out_of_range& e) {
      cout << format("While inserting elememt at index {} happend an error: ", index) << e.what() << endl;
    }
  }

  void erase(int index) {
    try {
      if (index < 0 || index >= size) {
        throw out_of_range("Index out of range");
      } else {
        AllocTraits::destroy(alloc, data + index);

        for (int i = index; i < size; ++i) {
          data[i] = std::move(data[i + 1]);
        }
        --size;
      }
    } catch (const out_of_range& e) {
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
