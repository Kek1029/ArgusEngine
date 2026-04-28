#ifndef ARGUSENGINE_VIRTUAL_LINEAR_ALLOCATOR_HPP
#define ARGUSENGINE_VIRTUAL_LINEAR_ALLOCATOR_HPP

#include <iostream>
#include <cstdint>

#include <FastBin.hpp>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
    #include <errno.h>
    #include <cstring>
#endif

#include "utils/Utils.hpp"

namespace ArgusEngine::Memory {

template <typename T>
class VirtualLinearAllocator {
private:
    T* base_ptr = nullptr;
    size_t committed_size = 0;
    size_t reserved_cap = 0;
    size_t page_size = 0;

    size_t align_to_page(size_t size) const {
        if (page_size == 0) return size;
        return (size + page_size - 1) & ~(page_size - 1);
    }

public:
    explicit VirtualLinearAllocator(size_t max_elements = 1024 * 1024 * 1024) {
#ifdef _WIN32
        SYSTEM_INFO si; GetSystemInfo(&si); page_size = si.dwPageSize;
#else
        page_size = sysconf(_SC_PAGESIZE);
#endif
        size_t total_bytes = max_elements * sizeof(T);
        size_t rounded_reserve = align_to_page(total_bytes);

        void* reserved_ptr = FastBin::ReserveAddressSpace(rounded_reserve);

#ifdef _WIN32
        if (reserved_ptr == NULL) {
#else
        if (reserved_ptr == MAP_FAILED) {
#endif
            base_ptr = nullptr;
        } else {
            base_ptr = reinterpret_cast<T*>(reserved_ptr);
        }

        if (!base_ptr) {
            std::cerr << "CRITICAL: Virtual Memory Reservation Failed! Error: " <<
#ifdef _WIN32
                GetLastError()
#else
                strerror(errno)
#endif
                << std::endl;
            return;
        }
        reserved_cap = rounded_reserve / sizeof(T);
    }

    VirtualLinearAllocator(const VirtualLinearAllocator&) = delete;
    VirtualLinearAllocator& operator=(const VirtualLinearAllocator&) = delete;

    VirtualLinearAllocator(VirtualLinearAllocator&& other) noexcept
        : base_ptr(other.base_ptr), committed_size(other.committed_size),
          reserved_cap(other.reserved_cap), page_size(other.page_size) {
        other.base_ptr = nullptr;
        other.committed_size = 0;
    }

    VirtualLinearAllocator& operator=(VirtualLinearAllocator&& other) noexcept {
        if (this != &other) {
            this->~VirtualLinearAllocator();
            base_ptr = other.base_ptr;
            committed_size = other.committed_size;
            reserved_cap = other.reserved_cap;
            page_size = other.page_size;
            other.base_ptr = nullptr;
        }
        return *this;
    }

    [[nodiscard]] inline T& operator[](size_t index) noexcept {
        return base_ptr[index];
    }

    [[nodiscard]] inline const T& operator[](size_t index) const noexcept {
        return base_ptr[index];
    }

    T* data() { return base_ptr; }
    const T* data() const { return base_ptr; }

    T* alloc(size_t n) {
        if (!base_ptr || n == 0) return nullptr;

        size_t new_size = committed_size + n;
        if (new_size > reserved_cap) {
            std::cerr << "Out of reserved virtual memory!" << std::endl;
            return nullptr;
        }

        size_t needed_bytes = new_size * sizeof(T);
        size_t current_mapped_bytes = align_to_page(committed_size * sizeof(T));

        if (needed_bytes > current_mapped_bytes) {
            size_t start_offset = current_mapped_bytes;
            size_t end_offset = align_to_page(needed_bytes);
            size_t bytes_to_commit = end_offset - start_offset;

            void* commit_addr = (char*)base_ptr + start_offset;

            FastBin::CommitMemory(commit_addr, bytes_to_commit);
        }

        T* ptr = base_ptr + committed_size;
        committed_size = new_size;
        return ptr;
    }

    void push_back(const T& value) {
        T* ptr = this->alloc(1);
        *ptr = value;
    }

    size_t size() const { return committed_size; }
    ~VirtualLinearAllocator() {
        if (!base_ptr) return;
        size_t total_bytes = align_to_page(reserved_cap * sizeof(T));
#ifdef _WIN32
        VirtualFree(base_ptr, 0, MEM_RELEASE);
#else
        munmap(base_ptr, total_bytes);
#endif
    }
};

} // namespace ArgusEngine::Memory

#endif