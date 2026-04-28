#ifndef ARGUSENGINE_LINEARALLOCATOR_HPP
#define ARGUSENGINE_LINEARALLOCATOR_HPP

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <atomic>

namespace ArgusEngine::Memory {

template <typename T>
class LinearAllocator {
private:
    T* buffer = nullptr;
    std::atomic<size_t> _size{0};
    size_t _capacity = 0;

    void grow(size_t min_capacity) {
        size_t new_cap = (_capacity == 0) ? 1024 : _capacity * 2;
        while (new_cap < min_capacity) new_cap *= 2;

        size_t alignment = 64;
        size_t size_bytes = (new_cap * sizeof(T) + alignment - 1) & ~(alignment - 1);

#ifdef _WIN32
        T* new_buf = static_cast<T*>(_aligned_malloc(size_bytes, alignment));
#else
        T* new_buf = static_cast<T*>(std::aligned_alloc(alignment, size_bytes));
#endif
        if (!new_buf) std::abort();

        if (buffer) {
            std::memcpy(new_buf, buffer, _size.load() * sizeof(T));
            std::free(buffer);
        }

        buffer = new_buf;
        _capacity = new_cap;
    }

public:
    explicit LinearAllocator(size_t cap = 1024) : _capacity(cap) {
        if (_capacity > 0) {
            buffer = static_cast<T*>(std::malloc(sizeof(T) * _capacity));
        }
    }

    LinearAllocator(LinearAllocator&& other) noexcept
        : buffer(other.buffer), _size(other._size.load()), _capacity(other._capacity) {
        other.buffer = nullptr;
        other._size = 0;
        other._capacity = 0;
    }

    LinearAllocator& operator=(LinearAllocator&& other) noexcept {
        if (this != &other) {
            if (buffer) {
                std::free(buffer);
            }

            buffer = other.buffer;
            _capacity = other._capacity;
            _size = other._size.load();

            other.buffer = nullptr;
            other._capacity = 0;
            other._size = 0;
        }
        return *this;
    }

    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    ~LinearAllocator() { std::free(buffer); }

    T& operator[](size_t index) { return buffer[index]; }
    const T& operator[](size_t index) const { return buffer[index]; }

    T& front() { return buffer[0]; }
    const T& front() const { return buffer[0]; }

    T& back() { return buffer[_size - 1]; }
    const T& back() const { return buffer[_size - 1]; }

    T* data() { return buffer; }
    const T* data() const { return buffer; }

    T* begin() { return buffer; }
    T* end() { return buffer + _size; }
    const T* begin() const { return buffer; }
    const T* end() const { return buffer + _size; }

    [[nodiscard]] size_t size() const { return _size.load(); }
    [[nodiscard]] size_t capacity() const { return _capacity; }
    [[nodiscard]] bool empty() const { return _size == 0; }

    void push_back(const T& value) {
        if (_size >= _capacity) grow(_size + 1);
        buffer[_size++] = value;
    }

    void push_back(T&& value) {
        if (_size >= _capacity) grow(_size + 1);
        buffer[_size++] = std::move(value);
    }

    void pop_back() {
        if (_size > 0) --_size;
    }

    void clear() {
        _size = 0;
    }

    void reallocate_last(size_t count) {
        size_t required = _size + count;
        if (required > _capacity) grow(required);
        _size = required;
    }

    void erase(size_t index) {
        if (index >= _size) return;

        if (index < _size - 1) {
            std::memmove(&buffer[index], &buffer[index + 1], (_size - index - 1) * sizeof(T));
        }
        --_size;
    }

    void resize(size_t new_size) {
        if (new_size > _capacity) grow(new_size);
        _size = new_size;
    }

    void reserve(size_t new_cap) {
        if (new_cap > _capacity) grow(new_cap);
    }

    void fast_erase(size_t index) {
        if (index >= _size) return;
        buffer[index] = std::move(buffer[_size - 1]);
        --_size;
    }

    T* alloc(size_t count = 1) {
        size_t required = _size + count;
        if (required > _capacity) grow(required);

        T* res = &buffer[_size];
        _size += count;
        return res;
    }

    T* atomic_alloc(size_t count = 1) {
        size_t aligned_count = (count + 15) & ~15;

        size_t current_size = _size.load(std::memory_order_relaxed);
        while (true) {
            if (current_size + aligned_count > _capacity) {
                return nullptr;
            }
            if (_size.compare_exchange_weak(current_size, current_size + aligned_count)) {
                return &buffer[current_size];
            }
        }
    }

    T* alloc_zeroed(size_t count = 1) {
        T* res = alloc(count);
        std::memset(res, 0, count * sizeof(T));
        return res;
    }
};
}

#endif