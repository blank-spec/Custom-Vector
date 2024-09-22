# Vector Class

## Overview

The `Vector` class is a dynamic array implementation in C++, providing a flexible and efficient way to manage a collection of elements. This custom implementation leverages allocator traits for memory management and supports various functionalities to manipulate the vector.

## Features

- **Dynamic Resizing**: Automatically resizes to accommodate new elements.
- **Custom Allocators**: Supports custom memory allocators through template parameters.
- **Multiple Element Addition**: Easily add multiple elements using `push_back` with initializer lists or variadic templates.
- **Iterators**: Provides a simple iterator interface for range-based loops.

## Usage

### Constructor

You can initialize a `Vector` in several ways:

1. **Default Constructor**:
   ```cpp
   Vector<int> vec;
   ```
2. **Initializer List**:
  ```cpp
  Vector<int> vec = {1, 2, 3, 4};
  ```

### Adding Elements

You can add elements to the vector using:

*push_back*: Add a single element.
  ```cpp
  vec.push_back(5);
  ```
*Multiple Elements*: You can also use push_back with an initializer list or by passing multiple arguments.
  ```cpp
  vec.push_back(6, 7, 8); // Adds 6, 7, and 8 to the vector
  ```
*emplace_back*: Construct and add elements in place.
  ```cpp
  vec.emplace_back(9);
  ```

### Accessing Elements

*Indexing*:
  ```cpp
  int firstElement = vec[0];
  ```
*Back Element*:
  ```cpp
  int lastElement = vec.back();
  ```

### Example
Here’s an example demonstrating how to use the Vector class:
  ```cpp
  #include "Vector.h"

int main() {
    // Create a Vector and initialize with some values
    Vector<int> vec = {1, 2, 3};

    // Resize capacity
    vec.resize(10);

    // Add a single element
    vec.push_back(4);
    
    // Add multiple elements
    vec.push_back(5, 6, 7); // Adds 5, 6, and 7 to the vector
    
    // Print the current state of the vector
    vec.print(); // Output: 1 2 3 4 5 6 7

    // Remove the last element
    vec.pop_back();
    
    // Print the updated state of the vector
    vec.print(); // Output: 1 2 3 4 5 6

    // Accessing elements
    int firstElement = vec[0]; // Access the first element
    int lastElement = vec.back(); // Access the last element
    
    // Output the accessed elements
    cout << "First Element: " << firstElement << endl; // Output: First Element: 1
    cout << "Last Element: " << lastElement << endl;   // Output: Last Element: 6

    return 0;
}
  ```


