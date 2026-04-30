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
#include <atomic>
#include <mutex>
namespace ArgusEngine::Memory {

template <typename T>
class VirtualLinearAllocator {
private:
    T* base_ptr = nullptr;
    std::atomic<size_t> committed_size{0};
    size_t reserved_cap = 0;
    size_t page_size = 0;


    std::mutex commit_mutex;
    std::atomic<size_t> last_mapped_bytes{0};

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
        : base_ptr(other.base_ptr), committed_size(other.committed_size.load(std::memory_order_relaxed)),
          reserved_cap(other.reserved_cap), page_size(other.page_size) {
        other.base_ptr = nullptr;
        other.committed_size = 0;
    }

    VirtualLinearAllocator& operator=(VirtualLinearAllocator&& other) noexcept {
        if (this != &other) {
            this->~VirtualLinearAllocator();
            base_ptr = other.base_ptr;
            committed_size = other.committed_size.load(std::memory_order_relaxed);
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

        size_t old_size = committed_size.fetch_add(n, std::memory_order_relaxed);
        size_t new_size = old_size + n;

        if (new_size > reserved_cap) {
            fprintf(stderr, "Out of Memory! %s\n", __PRETTY_FUNCTION__);
            std::abort();
        }

        size_t needed_bytes = new_size * sizeof(T);

        if (needed_bytes > last_mapped_bytes.load(std::memory_order_acquire)) {
            std::lock_guard<std::mutex> lock(commit_mutex);

            if (needed_bytes > last_mapped_bytes.load(std::memory_order_relaxed)) {
                size_t current_mapped = last_mapped_bytes.load(std::memory_order_relaxed);
                size_t end_offset = align_to_page(needed_bytes);
                size_t bytes_to_commit = end_offset - current_mapped;

                void* commit_addr = (char*)base_ptr + current_mapped;

                FastBin::CommitMemory(commit_addr, bytes_to_commit);

                last_mapped_bytes.store(end_offset, std::memory_order_release);
            }
        }

        return base_ptr + old_size;
    }

    void push_back(const T& value) {
        T* ptr = this->alloc(1);
        *ptr = value;
    }

    size_t size() const { return committed_size.load(std::memory_order_relaxed); }
    ~VirtualLinearAllocator() {
        if (!base_ptr) return;
        size_t total_bytes = align_to_page(reserved_cap * sizeof(T));
#ifdef _WIN32
        VirtualFree(base_ptr, 0, MEM_RELEASE);
#else
        munmap(base_ptr, total_bytes);
#endif
    }

    void clear() noexcept {
        committed_size.store(0, std::memory_order_relaxed);
    }

    void fit() {
        if (!base_ptr || committed_size == 0) return;

        size_t used_bytes = align_to_page(committed_size * sizeof(T));
        size_t total_reserver_bytes = align_to_page(reserved_cap * sizeof(T));

        if (used_bytes < total_reserver_bytes) {

            void* decommit_addr = (char*)base_ptr + used_bytes;
            size_t decommit_size = total_reserver_bytes - used_bytes;

#ifdef _WIN32
            VirtualFree(decommit_addr, decommit_size, MEM_DECOMMIT);
#else
            madvise(decommit_addr, decommit_size, MADV_DONTNEED);
#endif

        }
    }
};

} // namespace ArgusEngine::Memory

#endif