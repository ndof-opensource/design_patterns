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
    
    // Note: This is not thread safe

    // template<typename T, typename Alloc = std::allocator<T>>
    // struct CopyableUniquePtr {
    //     std::unique_ptr<T, Alloc> ptr;

    //     CopyableUniquePtr(std::unique_ptr<T> p) : ptr(std::move(p)) {}

    //     CopyableUniquePtr(const CopyableUniquePtr& other) 
    //         : ptr(other.ptr ? std::make_unique<T>(*other.ptr) : nullptr) {}

    //     CopyableUniquePtr& operator=(const CopyableUniquePtr& other) {
    //         if (this != &other) {
    //             ptr = other.ptr ? std::make_unique<T>(*other.ptr) : nullptr;
    //         }
    //         return *this;
    //     }

    //     // Also allow move for efficiency
    //     CopyableUniquePtr(CopyableUniquePtr&&) noexcept = default;
    //     CopyableUniquePtr& operator=(CopyableUniquePtr&&) noexcept = default;
    // };

    // Helper alias for readability
    template<class A, class T>
    using rebind_t = typename std::allocator_traits<A>::template rebind_alloc<T>;

    // --- Compile-time compatibility concept ---
    // "A_user is compatible with A_callee for T" iff:
    //   - both are rebindable to T
    //   - the rebound types are the same
    //   - the rebound type is constructible from the original (avoids dead-end rebinds)
    template<class A_provided, class A_callee>
    concept AllocCompatibleFor =
        requires { typename rebind_t<A_provided,  A_callee>; } &&
        std::is_same_v<rebind_t<A_provided, A_callee>> &&
        std::constructible_from<rebind_t<A_provided, A_callee>
}

// TODO: in callable_type_generator.hpp, from line 264, the types defined should be functions, not function pointers.
namespace ndof
{

    template<typename TestFn, typename F>
    concept ParameterCompatible = 
        Callable<F> &&  
        // is TestFn convertible to F.
        std::is_convertible_v<typename CallableTraits<TestFn>::ReturnType, typename CallableTraits<F>::ReturnType> &&
        std::is_convertible_v<typename CallableTraits<TestFn>::ArgTypes,   typename CallableTraits<F>::ArgTypes>;

    template <Function Fn, 
        typename Alloc = std::allocator<Fn>>
    struct Proxy
    {
    private:
        // TODO: handle is_nothrow_convertible for arguments?

        using ArgTypes = typename CallableTraits<F>::ArgTypes;
        using ReturnType = typename CallableTraits<F>::ReturnType;
        using Members = std::tuple<Fn*, std::any, Alloc>

        mutable Members members;
 
       consteval static bool is_noexcept() { return QualifiedBy<F, Qualifier::NoExcept>; }
       consteval static bool is_void_return() { return std::is_void_v<ReturnType>; }

       template<typename F, typename ...A>
       consteval static bool has_operator_parens() { 
            return requires(F f, A... a) { f(a...); }; 
        }

       template<typename... A>
       struct Inner {
           virtual ~Inner() = default;
           virtual ReturnType invoke(A&&...) noexcept(is_noexcept()) = 0;
           // TODO: Use clone.
           virtual std::unique_ptr<Inner> clone() const = 0;
       };

       template<typename... A>
       struct InnerFunction : Inner<A...> {
           InnerFunction(Fn f) {}
           
           ReturnType invoke(A&&... a) noexcept(is_noexcept()) override {
               if constexpr (is_void_return()) {
                   func(std::forward<A>(a)...);
               } else {
                   return func(std::forward<A>(a)...);
               }
           }

           std::unique_ptr<InnerBase> clone() const override {
               return std::make_unique<InnerFunction>(func);
           }
       };

    public:  
 
        template<AllocCompatibleFor<Alloc> A>
        Proxy(StandaloneFunction auto f, const A alloc = A{}) {
            // TODO: Implement.
        }
         
        template<AllocCompatibleFor<Alloc> A>
        Proxy(Functor auto&& f, const A alloc = A{}){
            // TODO: Implement.
        }

        template<
            typename T, 
            typename R, 
            typename ...Args, 
            AllocCompatibleFor<Alloc> A>
        Proxy(T&& t, R(*mfp)(Args...), A alloc = A{}){
            // TODO: Implement.
        }
             
        template<typename ...A>
        return_type operator(this auto&& self, A... a) & noexcept(is_noexcept())  
        // TODO: Add constraints.
        {
            // TODO: Implement.
        }   

        // TODO: Implement all the other constructors and assignment operators.

        Alloc get_allocator() const{
            // TODO: Implement.
        }
    private:
    };
}