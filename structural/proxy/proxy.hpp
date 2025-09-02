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


    template <Function F>
    struct Proxy
    {
    private:
        // TODO: handle is_nothrow_convertible for arguments?

        using ArgTypes = typename CallableTraits<F>::ArgTypes;
        using ReturnType = typename CallableTraits<F>::ReturnType;

        template<typename ...A>
        consteval bool is_noexcept() {
            // Make sure the conversions of A... to ArgTypes is non-throwing?
            if constexpr (std::is_nothrow_invocable_v<F, A...> ) {
                return true;
            } else {
                return false;
            }
        }

        struct InnerInterface {

        }

        template <auto f, typename ...AllocatorType>
        requires (sizeof...(AllocatorType)<2)
        struct Inner;

        template <StandaloneFunction auto f>
        struct Inner<f>
        {
            template<typename ...A>
            static decltype(auto) execute(std::any& inner, A&&...args) noexcept(is_noexcept<A...>())
            // TODO: Fix.  Should use either is_nothrow_invocable or is_invocable, depending on the specification.
                requires(std::is_invocable_v<F, A...>)
            {
                return std::forward<return_type>(std::invoke_r(f,std::forward<A>(args)...));
            }
        };



        // TODO: Change MemberFunctionPtr to T.  the operator() will be selected based on the inner type.
        template<MemberFunctionPtr auto mf >
        struct Inner<mf> {
            using ClassType = typename CallableTraits<decltype(mf)>::ClassType;

            template<typename Any, typename ...A>
            static decltype(auto) execute(Any& inner, A&&... args) noexcept(is_noexcept())
                requires (std::is_invocable_v<F, ClassType, A...> )  
            {
                
                return std::forward<return_type>(std::invoke_r(mf, std::any_cast<ClassType&>(inner), std::forward<A>(args)...));
            }
        };

        // Note:  Allow a wrapped method to take an allocator if one is not provided as a parameter, but the argument list allows it as the last parameter.
        //        or, if the other form is used, i.e., using the allocator tag as the first argument and the allocator as the second.
        
        template<typename T>
        struct ExecutionExpansion;

        template<typename ...A>
        struct ExecutionExpansion<std::tuple<A...>> {
            using type = ReturnType(*)(std::any& inner, A&&...args) noexcept(is_noexcept<A...>());
        }; 

        using ExecutionPtr = typename ExecutionExpansion<ArgTypes>::type;
        ExecutionPtr execute_ptr;

    public:
        using return_type = typename CallableTraits<F>::ReturnType;
        using arg_types = typename CallableTraits<F>::ArgTypes;

        // TODO: Move to CallableTraits.
        consteval static bool is_noexcept() { return QualifiedBy<F, Qualifier::NoExcept>; }
        consteval static bool is_void_return() { return std::is_void_v<return_type>; }

        constexpr Proxy(StandaloneFunction auto&& f) noexcept
            : execute_ptr(&Inner<f>::execute)
        {
            // Do nothing.
        }

        // TODO: versions that take allocator and deleter.
        // TODO: Consider checking if function is allocator-aware and if it has one of the two
        //       acceptable forms, and if the arguments don't already include an allocator.
        template<typename T, typename Alloc = std::allocator<void>>
        constexpr explicit Proxy(T&& functor_obj, Alloc& a = Alloc{}) noexcept {

        }
 
        
        template<typename ...A>
        return_type operator(this auto&& self, A... a) & noexcept(is_noexcept())  
        // TODO: Add constraints.
        {
            return std::invoke(ExecutePtr, self.inner, std::forward<A>(a)...);
        }   

        // TODO: Implement all the other constructors and assignment operators.

    private:
 

        // TODO: consider deleter.


        // TODO: Consider r-value member functions and member function pointers.

        // TODO: Use NDoF GenerateFunctionPointerTraits to generate the function pointer type.
        // TODO: Fix this. it's broken.  F is a Function, so we'll need to use the other parameters too.
        //       Don't forget about const and volatile pointer qualifiers.





        // template<typename Fn>
        // using DefaultMethodPtr =  GenerateFunctionPtr<Fn::qualifiers, typename F::ReturnType, typename F::ArgTypes>::type;
        
        // // Define a deleter that uses the allocator
        // template <typename T>
        // // TODO: This will have a unique_ptr to an allocator.
        // // TODO: Add the nocopyconstructible wrapper.
        // // TODO: Make exception proof.
        // struct Delete {
            
        //     using AllocTraits = std::allocator_traits<Alloc<T>>;
        //     Alloc<T> alloc;

        //     Delete(const Alloc<T>& a = Alloc<T>()) : alloc(a) {}

        //     void operator()(T* ptr) const {
        //         if (ptr) {
        //             AllocTraits::destroy(alloc, ptr);
        //             AllocTraits::deallocate(alloc, ptr, 1);
        //         }
        //     }
        // };

    public:


        // NOTE: Added self to line 165.



        // TODO: r-value?
      
        // template <typename... A>
        // return_type operator()(auto &&...args) noexcept(is_noexcept())
        //     requires(std::is_invocable_r_v<return_type, F, A...> && StandaloneFunction<F>)
        // {
        //     return std::invoke(execute_ptr, inner, std::forward<A>(args)...);
        // }

        // template <typename... A>
        // return_type operator()(auto &&...args) const noexcept(is_noexcept()) 
        //     requires(std::is_invocable_r_v<return_type, F, A...> && StandaloneFunction<F>)
        // {
        //     return std::invoke(execute_ptr, inner, std::forward<A>(args)...);
        // }

        // template <typename... A>
        // return_type operator()(auto &&...args) volatile noexcept(is_noexcept())
        //     requires(std::is_invocable_r_v<return_type, F, A...> && StandaloneFunction<F>) {
        //         return std::invoke(execute_ptr, inner, std::forward<A>(args)...);
        // }

        // template <typename... A>
        // return_type operator()(auto &&...args) const volatile noexcept(is_noexcept())
        //     requires(std::is_invocable_r_v<return_type, F, A...> && StandaloneFunction<F>) {
        //         return std::invoke(execute_ptr, inner, std::forward<A>(args)...);
        // }

        // // TODO: Implement.
        // Alloc get_allocator()
        // {
        // }
    };
