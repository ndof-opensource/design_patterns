#pragma once

#include <exception>
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

// Custom deleter
template <typename T, typename Alloc>
struct CloneDeleter {
    // Bob's instructions were to hold these in a tuple so that compiler 
    // can optimize away allocating memory for the allocator if it's stateless.
    // [[no_unique_address]] accomplishes the same goal. Get Bob concurrence.
    // 10/29/25 Bob concurred based on prototyping this previously.
    
    [[no_unique_address]] Alloc alloc;

    CloneDeleter(const Alloc& a = Alloc()) : alloc(a) {}
    using alloc_traits = std::allocator_traits<Alloc>;

    // TODO: determine if allocator is noexcept, then handle accordingly
    static constexpr bool is_noexcept = noexcept(alloc_traits::destroy) && noexcept(alloc_traits::deallocate);

    void operator()(T* ptr) noexcept(is_noexcept) {
        if (!ptr) return;
        alloc_traits::destroy(alloc, ptr);
        alloc_traits::deallocate(alloc, ptr, 1);
    }
};

template <IsCloneable T, typename Alloc = std::allocator<T>>
class Prototype {
private:
    
private:

    using InternalDeleter = CloneDeleter<T, Alloc>;
    std::unique_ptr<T, InternalDeleter> held_object;
    
public:
    explicit Prototype(const T& obj, const Alloc& alloc = Alloc()) {
        using traits = std::allocator_traits<Alloc>;
        // convert from nested to checking for appropriate exception and handling
        // also add nested exception for what happens if deallocate throws an 
        // exception in handling of construction exception
        try {
            T* ptr = traits::allocate(alloc, 1);
            try {
                traits::construct(alloc, ptr, obj);
                held_object = std::unique_ptr<T, InternalDeleter>(ptr, InternalDeleter(alloc));
            } catch (...) {
                try {
                    traits::deallocate(alloc, ptr, 1);
                } catch (const std::exception& e) {
                    std::throw_with_nested(e);
                }
            }
        } catch (...) {

        }
    }

    // TODO: Peter: ended here after re-writing lvalue ref constructor.
    // Need to update rvalue ref constructor and following code.
    explicit Prototype(T&& obj, const Alloc& alloc = Alloc()) : held_object(std::move(obj)), allocator(alloc) {}

    // Uses allocator held by this Prototype
    T clone() const
        requires IsAllocatorCloneable<T>
    {
        return this->clone(this->allocator);
    }

    // Allocator-aware Clone: use the passed allocator, return a pointer to T
    template <typename OtherAlloc>
    T* clone(const OtherAlloc& alloc) const
        requires IsAllocatorCloneable<T>
    {
        if constexpr (HasAllocatorAwareClone<T, OtherAlloc>) {
            // Prioritize using object's allocator-aware clone method, if it exists
            return held_object.clone(alloc);
        } else {
            // Otherwise, fall back to using allocator-aware copy constructor
            using traits = std::allocator_traits<OtherAlloc>;
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
