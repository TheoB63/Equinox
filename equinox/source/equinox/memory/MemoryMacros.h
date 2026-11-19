#pragma once

#include "equinox/memory/MemoryTracker.h"
#include "equinox/core/Profiler.h"

// ===================================================================================
// Tracked allocation macros — opt-in, per call-site category tagging
// ===================================================================================
// Usage:
//   auto* ctx = LEQN_NEW(Memory::Category::Rendering, VulkanContext);
//   EQN_DELETE(Memory::Category::Rendering, ctx);

// Single object new/delete
#define EQN_NEW(Category, Type, ...)                                                    \
    [&]() -> Type* {                                                                    \
        Type* _eqn_ptr = new Type(__VA_ARGS__);                                         \
        ::Equinox::Memory::MemoryTracker::RecordAlloc(Category, sizeof(Type));          \
        EQN_PROFILE_ALLOC(_eqn_ptr, sizeof(Type));                                      \
        return _eqn_ptr;                                                                \
    }()

#define EQN_DELETE(Category, ptr)                                                       \
    do {                                                                                \
        if (ptr) {                                                                      \
            ::Equinox::Memory::MemoryTracker::RecordFree(Category, sizeof(*(ptr)));     \
            EQN_PROFILE_FREE(ptr);                                                      \
            delete (ptr);                                                               \
            (ptr) = nullptr;                                                            \
        }                                                                               \
    } while(0)

// Raw byte allocation (operator new / operator delete)
#define EQN_ALLOC(Category, size)                                                       \
    [&]() -> void* {                                                                    \
        void* _eqn_ptr = ::operator new(size);                                          \
        ::Equinox::Memory::MemoryTracker::RecordAlloc(Category, size);                  \
        EQN_PROFILE_ALLOC(_eqn_ptr, size);                                              \
        return _eqn_ptr;                                                                \
    }()

#define EQN_FREE(Category, ptr, size)                                                   \
    do {                                                                                \
        if (ptr) {                                                                      \
            ::Equinox::Memory::MemoryTracker::RecordFree(Category, size);               \
            EQN_PROFILE_FREE(ptr);                                                      \
            ::operator delete(ptr);                                                     \
            (ptr) = nullptr;                                                            \
        }                                                                               \
    } while(0)

// Array new/delete
#define EQN_NEW_ARRAY(Category, Type, count)                                            \
    [&]() -> Type* {                                                                    \
        Type* _eqn_ptr = new Type[count];                                               \
        ::Equinox::Memory::MemoryTracker::RecordAlloc(Category, sizeof(Type) * (count));\
        EQN_PROFILE_ALLOC(_eqn_ptr, sizeof(Type) * (count));                            \
        return _eqn_ptr;                                                                \
    }()

#define EQN_DELETE_ARRAY(Category, ptr, count)                                                \
    do {                                                                                      \
        if (ptr) {                                                                            \
            ::Equinox::Memory::MemoryTracker::RecordFree(Category, sizeof(*(ptr)) * (count)); \
            EQN_PROFILE_FREE(ptr);                                                            \
            delete[] (ptr);                                                                   \
            (ptr) = nullptr;                                                                  \
        }                                                                                     \
    } while(0)