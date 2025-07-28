#ifndef NDOF_OS_CALLABLE_TRAITS_PROXY_HPP
#define NDOF_OS_CALLABLE_TRAITS_PROXY_HPP

// #include <memory>
// #include <optional>
// #include <functional>
// #include <type_traits>
// #include <expected>
// #include <utility>
// #include <stdexcept>

#include <ndof-os/callable_traits/callable_concepts.hpp>
#include <ndof-os/callable_traits/callable_traits.hpp>
#include <ndof-os/callable_traits/qualified_by.hpp>
#include <ndof-os/callable_traits/callable_type_generator.hpp>

#include <variant>
#include <utility>

// TODO: [6/16/25] use std::common_type to allow any type to be cast to a wrapper of that type.
//                 noncopyconstructible wrapper that can be used to store any type in a std::any.


// // TODO: add support for exception handling callbacks on enter/exit.
// // TODO: Define a class that defines the callback object interface requirements alternatively.
// // TODO: Check alignment requirement, to make sure it doesn't exceed the size of the SBO space.
// //       Alignment ≤ SBO buffer alignment (usually 16)


// TODO: Verify this AI code and put in a utilities header.
namespace ndof {

    // TODO: Add checks for nothrow constructible and nothrow move constructible.
    //       I.E., If T has a nothrow constructor, then CopyableUniquePtr<T> should be nothrow constructible.

    // TODO: make sure this type supports allocator.
    // TODO: will have to add the deleter and also use the constructor of std::unique_ptr that takes a deleter
    //       and should rebind the unique pointers allocator type to the allocator type passed in.
    // TODO: allocators should not move or be copied into the target instance.  
    // TODO: if the allocator pointer is different across moves, copies or assignments, then space should be allocated for the new object 
    //       using the new type's allocator and the old object should be destroyed using the old allocator.
    //       I.E., allocators stay with their respective objects and are not shared across copies or moves.
    //       This is to ensure that the allocator is always valid and can be used to deallocate the object.

    // TODO: support the ability to pass a deleter in addition to an allocator, as with (6): 
    //    https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr.html
    template<typename T, typename Alloc = std::allocator<T>>
    struct CopyableUniquePtr {
        std::unique_ptr<T, Alloc> ptr;

        CopyableUniquePtr(std::unique_ptr<T> p) : ptr(std::move(p)) {}

        CopyableUniquePtr(const CopyableUniquePtr& other) 
            : ptr(other.ptr ? std::make_unique<T>(*other.ptr) : nullptr) {}

        CopyableUniquePtr& operator=(const CopyableUniquePtr& other) {
            if (this != &other) {
                ptr = other.ptr ? std::make_unique<T>(*other.ptr) : nullptr;
            }
            return *this;
        }

        // Also allow move for efficiency
        CopyableUniquePtr(CopyableUniquePtr&&) noexcept = default;
        CopyableUniquePtr& operator=(CopyableUniquePtr&&) noexcept = default;
    };
}

// TODO: in callable_type_generator.hpp, from line 264, the types defined should be functions, not function pointers.
namespace ndof
{

    template <Function F>
    struct Proxy
    {
    private:
    
    public:
        using return_type = typename CallableTraits<F>::ReturnType;
        using arg_types = typename CallableTraits<F>::ArgTypes;

        // TODO: Move to CallableTraits.
        consteval static bool is_noexcept() { return QualifiedBy<F, Qualifier::NoExcept>; }
        consteval static bool is_void_return() { return std::is_void_v<return_type>; }

    private:
 

        std::any inner;

        // TODO: consider deleter.
        template <auto f, typename ...AllocatorType>
        requires (sizeof...(AllocatorType)<2)
        struct Inner;

        template <StandaloneFunction auto f>
        // todo: requires f was an r-value?
        struct Inner<f>
        {

            using ReturnType = decltype(std::forward<decltype(f)>(std::forward<A>(args)...));

            template <typename... A>
            static decltype(auto)  execute(qualified_any& a, A &&...args) noexcept(is_noexcept())
                requires(std::is_invocable_r_v<return_type, F, A...>)
            {
 
            }
        };

        template<MemberFunctionPtr auto mf >
        struct Inner<mf> {
            using typename CallableTraits<decltype(mf)>::ClassType;

            template<typename ...A>
            static decltype(auto) execute(qualified_any& a, A&&... args) noexcept(is_noexcept())
                requires (std::is_invocable_r_v<return_type, F,  A...> )
            {

            }
        };

        // TODO: Consider r-value member functions and member function pointers.

        // TODO: Use NDoF GenerateFunctionPointerTraits to generate the function pointer type.
        // TODO: Fix this. it's broken.  F is a Function, so we'll need to use the other parameters too.
        //       Don't forget about const and volatile pointer qualifiers.
        using ExecutePtr = ndof::as_function_ptr_t<F>;
        ExecutePtr execute_ptr;

        template<typename Fn>
        using DefaultMethodPtr =  GenerateFunctionPtr<Fn::qualifiers, typename F::ReturnType, typename F::ArgTypes>::type;
        
        // Define a deleter that uses the allocator
        template <typename T>
        // TODO: This will have a unique_ptr to an allocator.
        // TODO: Add the nocopyconstructible wrapper.
        // TODO: Make exception proof.
        struct Delete {
            
            using AllocTraits = std::allocator_traits<Alloc<T>>;
            Alloc<T> alloc;

            Delete(const Alloc<T>& a = Alloc<T>()) : alloc(a) {}

            void operator()(T* ptr) const {
                if (ptr) {
                    AllocTraits::destroy(alloc, ptr);
                    AllocTraits::deallocate(alloc, ptr, 1);
                }
            }
        };

    public:
        // TODO: versions that take allocator and deleter.
        template<typename T>
        constexpr explicit Proxy(T&& functor_obj, Alloc& a) noexcept {

        }

        // NOTE: Added self to line 165.
        template<typename ...A>
        return_type operator(this auto&& self, A... a) noexcept(is_no_except())
        require (std::is_invocable_r_v<return_type,ExecutePtr,A...>) {
            return std::invoke(ExecutePtr, self, std::forward<A>(a));
        }       

        constexpr explicit Proxy(StandaloneFunction auto f) noexcept : execute_ptr(&Inner<f>::execute)  
        {
            
        }


        // TODO: r-value?
      
        template <typename... A>
        return_type operator()(auto &&...args) noexcept(is_noexcept())
            requires(std::is_invocable_r_v<return_type, F, A...> && StandaloneFunction<F>)
        {
            return std::invoke(execute_ptr, inner, std::forward<A>(args)...);
        }

        template <typename... A>
        return_type operator()(auto &&...args) const noexcept(is_noexcept()) 
            requires(std::is_invocable_r_v<return_type, F, A...> && StandaloneFunction<F>)
        {
            return std::invoke(execute_ptr, inner, std::forward<A>(args)...);
        }

        template <typename... A>
        return_type operator()(auto &&...args) volatile noexcept(is_noexcept())
            requires(std::is_invocable_r_v<return_type, F, A...> && StandaloneFunction<F>) {
                return std::invoke(execute_ptr, inner, std::forward<A>(args)...);
        }

        template <typename... A>
        return_type operator()(auto &&...args) const volatile noexcept(is_noexcept())
            requires(std::is_invocable_r_v<return_type, F, A...> && StandaloneFunction<F>) {
                return std::invoke(execute_ptr, inner, std::forward<A>(args)...);
        }

        // // TODO: Implement.
        // Alloc get_allocator()
        // {
        // }
    };
