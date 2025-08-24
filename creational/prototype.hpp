#pragma once

#include <memory>
#include <concepts>
#include <type_traits>
#include <memory_resource>

template <typename T>
concept HasSimpleClone = requires(const T& obj) {
    { obj.clone() } -> std::same_as<T>;
};

template <typename T, typename Alloc>
concept HasAllocatorAwareClone = requires(const T& obj, Alloc& alloc) {
    { obj.clone(alloc) } -> std::same_as<T*>;
};

template<typename T, typename Alloc>
concept HasAllocatorConstructor = std::is_constructible_v<T, const T&, const Alloc&>;

// concept to hide Alloc parameter from IsCloneable
template <typename T>
concept IsAllocatorCloneable = HasAllocatorAwareClone<T, std::allocator<T>> ||
                               HasAllocatorConstructor<T, std::allocator<T>>;

template <typename T>
concept HasPlacementClone = requires(const T& obj, void* ptr) {
    { obj.clone(ptr) } -> std::same_as<T*>;
};


template <typename T>
concept IsCloneable = std::is_copy_constructible_v<std::remove_cvref_t<T>> ||
                      HasSimpleClone<std::remove_cvref_t<T>> ||
                      IsAllocatorCloneable<std::remove_cvref_t<T>> ||
                      HasPlacementClone<std::remove_cvref_t<T>>;

template <IsCloneable T>
class Prototype {
private:
    
private:
    T held_object;
    
public:
    explicit Prototype(const T& obj) : held_object(obj) {}

    explicit Prototype(T&& obj) : held_object(std::move(obj)) {}

    // Simple, Value Clone: takes no arguments, returns a copy of T
    T clone() const {
        if constexpr (HasSimpleClone<T>) {
            // prefer to use class's clone() method if it exists
            return held_object.clone();
        } else {
            // otherwise, fall back to copy constructor
            return held_object;
        }
    }

    // Allocator-aware Clone: use the passed allocator, return a pointer to T
    template <typename Alloc>
    T* clone(const Alloc& alloc) const
        requires IsAllocatorCloneable<T>
    {
        if constexpr (HasAllocatorAwareClone<T, Alloc>) {
            // Prioritize using object's allocator-aware clone method, if it exists
            return held_object.clone(alloc);
        } else {
            // Otherwise, fall back to using allocator-aware copy constructor
            using traits = std::allocator_traits<Alloc>;
            T* ptr = traits::allocate(alloc, 1);
            try {
                // Pass the allocator to T's constructor
                traits::construct(alloc, ptr, held_object, alloc);
                return ptr;
            } catch (...) {
                // Catch any exception thrown by the constructor and deallocate
                traits::deallocate(alloc, ptr, 1);
                throw;
            }
        }
    }

    // Placement-stype clone method
    // requires a placement-style clone method in object, or copy constructor 
    // to use placement new
    T* clone(void* ptr) const
        requires HasPlacementClone<T> || std::is_copy_constructible_v<T>
    {
        if constexpr (HasPlacementClone<T>) {
            // Prioritize object's own placement clone method, if it exists
            return held_object.clone(ptr);
        } else {
            // Otherwise, fall back to using placement new with copy constructor
            return new(ptr) T(held_object);
        }
    }
};
