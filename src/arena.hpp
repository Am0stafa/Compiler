// This code defines a custom memory allocator class named `ArenaAllocator`. Memory allocation is a critical operation in many programs, and custom memory allocators are often used in performance-sensitive or memory-constrained environments such as compilers. Here's a breakdown of the code:

// 1. Class Definition:
//     - `class ArenaAllocator`: This is the declaration of the `ArenaAllocator` class.

// 2. Constructor:
//     - `inline explicit ArenaAllocator(size_t bytes)`: The constructor initializes an instance of the `ArenaAllocator` with a specified size in bytes.
//         - `m_size(bytes)`: Initializes `m_size` with the value of `bytes`.
//         - `malloc(m_size)`: Allocates a block of memory of size `m_size`.
//         - `static_cast<std::byte*>(malloc(m_size))`: Casts the returned pointer to `std::byte*`.
//         - `m_buffer = static_cast<std::byte*>(malloc(m_size))`: Assigns the casted pointer to `m_buffer`.
//         - `m_offset = m_buffer`: Initializes `m_offset` to point to the beginning of the allocated buffer.

// 3. Allocation Method:
//     - `template <typename T> inline T* alloc()`: This is a template method that allocates memory for an object of type `T`.
//         - `void* offset = m_offset`: Stores the current offset.
//         - `m_offset += sizeof(T)`: Advances `m_offset` by the size of `T`.
//         - `static_cast<T*>(offset)`: Casts `offset` to `T*`.
//         - `return static_cast<T*>(offset)`: Returns the casted pointer.

// 4. Copy Constructor and Assignment Operator:
//     - The copy constructor and the copy assignment operator are deleted to prevent copying of `ArenaAllocator` objects, which is a common practice with custom allocators to prevent issues like double-freeing memory.

// 5. Destructor:
//     - `inline ~ArenaAllocator()`: The destructor releases the allocated memory.
//         - `free(m_buffer)`: Frees the memory block pointed to by `m_buffer`.

// 6. Private Members:
//     - `size_t m_size`: Stores the size of the buffer.
//     - `std::byte* m_buffer`: Points to the beginning of the allocated buffer.
//     - `std::byte* m_offset`: Points to the current offset within the buffer, where the next allocation will occur.

// This `ArenaAllocator` class implements a very simple form of arena (or stack) allocation, where memory is allocated in a single block (the "arena"), and individual allocations are made by simply moving a pointer (the "offset") within that block. This can be much faster than heap allocation, but it's more restrictive: there's no way to free individual objects, and the entire arena is freed at once when the `ArenaAllocator` is destroyed. This kind of allocator is often useful in compilers, where many temporary objects may be created during compilation, and all can be freed at once when compilation is finished.


#pragma once

class ArenaAllocator {
public:
  inline explicit ArenaAllocator(size_t bytes) : m_size(bytes){
    m_buffer = static_cast<std::byte*>(malloc(m_size));
    m_offset = m_buffer;
  }

  // Tell it what type of object you want to allocate and it will determine the size of that object and allocate it for you
  template <typename T>
  inline T* alloc(){
    void* offset = m_offset;
    m_offset += sizeof(T);
    return static_cast<T*>(offset);
  }

  inline ArenaAllocator(const ArenaAllocator& other) = delete;

  inline ArenaAllocator operator=(const ArenaAllocator& other) = delete;

  inline ~ArenaAllocator(){
    free(m_buffer);
  }

private:
  size_t m_size;
  std::byte* m_buffer;
  std::byte* m_offset;
};