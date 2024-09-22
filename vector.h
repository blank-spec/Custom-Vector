#pragma once

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
using namespace std;

template <typename T, class Alloc = allocator<T>> class Vector {
private:
  unsigned int capacity;
  size_t size;
  T *data;
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
  class Iterator {
  private:
    T *ptr;

  public:
    explicit Iterator(T *p) : ptr(p) {}

    T &operator*() const { return *ptr; }
    T *operator->() const { return ptr; }

    Iterator &operator++() {
      ++ptr;
      return *this;
    }

    Iterator &operator++(int) {
      Iterator tmp = *this;
      ++ptr;
      return tmp;
    }

    bool operator==(const Iterator &other) const { return ptr == other.ptr; }
    bool operator!=(const Iterator &other) const { return ptr != other.ptr; }
  };

  Vector()
      : alloc(Alloc()), size(0), capacity(1),
        data(AllocTraits::allocate(alloc, capacity)) {}

  Vector(std::initializer_list<T> init) : Vector() {
    for (const auto &elem : init) {
      emplace_back(elem);
    }
  }

  ~Vector() { clearMemory(); }

  Vector(Vector &&other) noexcept
      : size(other.size), capacity(other.capacity), data(other.data) {
    other.size = 0;
    other.capacity = 0;
    other.data = nullptr;
  }

  Vector &operator=(Vector &&other) noexcept {
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

  Vector(const Vector &) = delete;
  Vector &operator=(const Vector &) = delete;

  void reserve(int newCapacity) {
    if (newCapacity > capacity) {
      resize(newCapacity);
    }
  }

  Iterator begin() { return Iterator(data); }
  Iterator end() { return Iterator(data + size); }

  template <typename... Args> void emplace_back(Args &&...args) {
    if (size == capacity) {
      resize(capacity * 2);
    }
    AllocTraits::construct(alloc, data + size, std::forward<Args>(args)...);
    ++size;
  }

  void push_back(const T &value) { emplace_back(value); }

  void push_back(T &&value) { emplace_back(std::move(value)); }

  template <typename... Args> void push_back(Args &&...args) {
    (emplace_back(std::forward<Args>(args)), ...);
  }

  void push_back(std::initializer_list<T> init) {
    for (const auto &elem : init) {
      emplace_back(elem);
    }
  }

  void pop_back() {
    if (size > 0) {
      size--;
      AllocTraits::destroy(alloc, data + size);
    } else {
      throw out_of_range("Vector is empty");
    }
  }

  T &operator[](int index) const {
    if (index < 0 || index >= size) {
      throw out_of_range("Index out of range");
    }
    return data[index];
  }

  T &back() const {
    if (size <= 0) {
      throw out_of_range("Vector is empty");
    }
    return data[size - 1];
  }

  void print() const {
    for (size_t i = 0; i < size; ++i) {
      cout << data[i] << ' ';
    }
    cout << endl;
  }

  int getSize() const { return size; }
};
