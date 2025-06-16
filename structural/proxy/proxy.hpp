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
//                 

// namespace ndof {

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
    template<typename T>
    struct CopyableUniquePtr {
        std::unique_ptr<T> ptr;

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

    template <Function F, typename AllocDummy, bool const_required = false, bool volatile_required = false, template <typename> typename Alloc = std::allocator>
    class Proxy
    {
    public:
        using return_type = typename CallableTraits<F>::ReturnType;
        using arg_types = typename CallableTraits<F>::ArgTypes;

        // TODO: Move to CallableTraits.
        consteval static bool is_noexcept() { return QualifiedBy<F, Qualifier::NoExcept>; }
        consteval static bool is_void_return() { return std::is_void_v<return_type>; }

    private:
        // TODO: Promote to CallableTraits.

        template <typename T>
        using add_const_if_required = std::conditional_t<const_required, std::add_const_t<T>, T>;

        template <typename T>
        using add_volatile_if_required = std::conditional_t<volatile_required, std::add_volatile_t<T>, T>;

        template <typename T>
        using similarly_qualified_t = add_volatile_if_required<add_const_if_required<T>>;

        using qualified_any = similarly_qualified_t<std::any>;
        qualified_any inner;

        template <auto f, typename ...AllocatorType>
        requires (sizeof...(AllocatorType)<2)
        struct Inner;

        template <StandaloneFunction auto f>
        struct Inner<f>
        {

            template <typename... A>
            static return_type execute(qualified_any& a, A &&...args) noexcept(is_noexcept())
                requires(std::is_invocable_r_v<return_type, F, A...>)
            {
                return f(std::forward<A>(args)...);
            }
        };

        template<MemberFunctionPtr auto mf >
        struct Inner<mf> {
            using typename CallableTraits<decltype(mf)>::ClassType;

            template<typename ...A>
            static return_type execute(qualified_any& a, A&&... args) noexcept(is_noexcept())
                requires (std::is_invocable_r_v<return_type, F,  A...> )
            {

            }
        };

        // TODO: Consider r-value member functions and member function pointers.

        // TODO: Use NDoF GenerateFunctionPointerTraits to generate the function pointer type.
        // TODO: Fix this. it's broken.  F is a Function, so we'll need to use the other parameters too.
        using ExecutePtr = ndof::as_function_ptr_t<F>;
        ExecutePtr execute_ptr;

        template<typename Fn>
        using DefaultMethodPtr =  GenerateFunctionPtr<Fn::qualifiers, typename F::ReturnType, typename F::ArgTypes>::type;
        
        // Define a deleter that uses the allocator
        template <typename T>
        // TODO: This will have a unique_ptr to an allocator.
        // TODO: Add the nocopyconstructible wrapper.
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
        constexpr explicit Proxy(StandaloneFunction auto f) noexcept : execute_ptr(&Inner<f>::execute)  
        {
        }

        // LOOKEE HERE ------------------------------------------------------------------------------------<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        // TODO: Make the constructor allocator aware.
        template<typename T, typename Allocator>
        constexpr explicit Proxy(T&& object, MemberFunctionPtr auto f, Allocator& alloc) noexcept 
            // TODO: requires (std::is_nothrow_move_constructible_v<>)
                : execute_ptr(&Inner<f>::execute),  
        {

        }

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

    template <typename T, typename Alloc>
    using ReboundAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;

    // TODO: CallableTraits that takes instances.
template<
    typename T,
    typename Dummy,
    bool const_required = std::is_const_v<std::remove_reference_t<std::decay_t<T>>>,
    bool volatile_required = std::is_volatile_v<std::remove_reference_t<std::decay_t<T>>>,
    template<typename> typename PassedAlloc = std::allocator
>

Proxy(T&& object, auto f, PassedAlloc<Dummy>& alloc) ->
    Proxy<
        decltype(f),
        int,
        std::is_const_v<std::remove_reference_t<std::decay_t<T>>>,
        std::is_volatile_v<std::remove_reference_t<std::decay_t<T>>>,
        PassedAlloc
    >;

}



// template<typename ...A>
// R operator()(A&&... args) const noexcept(QualifiedBy<F, Qualifier::NoExcept>)
//     requires std::is_invocable_v<F, A...>
//     -> std::invoke_result_t<F, A...>
// {
//     auto x = []() noexcept{};
//     if (noexcept(x)) {

//     }

//     if constexpr (QualifiedBy<F, Qualifier::NoExcept>) {
//         return std::invoke(callable, std::forward<A>(args)...);
//     } else {
//         try {
//             return std::invoke(callable, std::forward<A>(args)...);
//         }
//         } catch (...) {

//             if (std::current_exception()) {
//                 try {
//                     std::rethrow_exception(std::current_exception());
//                 } catch (const std::exception& e) {
//                     throw bad_proxy_call(e.what());
//                 } catch (...) {
//                     // TODO: Bundle the caught exception into the bad_proxy_call exception.
//                     throw bad_proxy_call("Unknown exception in Proxy call");
//                 }
//             }
//             throw bad_proxy_call("Unknown exception in Proxy call");
//         }
//     }
// }
 

// TODO: add support for const, volatile, const volatile qualifiers
// TODO: add support for variadic functions.
// TODO: add support for noexcept

// // Function pointer
// template <FunctionPtr F>
// class Proxy<F> {
//     std::optional<F> func_ptr;
//     std::weak_ptr<F> func_wp;

// public:
//     explicit Proxy(F f) : func_ptr(f) {}
//     explicit Proxy(std::weak_ptr<F> wp) : func_wp(std::move(wp)) {}

//     template <typename... Args>
//     auto operator()(Args&&... args) const
//         -> std::expected<std::invoke_result_t<F, Args...>, bad_proxy_call>
//     {
//         if (func_ptr && *func_ptr) {
//             return std::invoke(*func_ptr, std::forward<Args>(args)...);
//         }
//         if (auto sp = func_wp.lock()) {
//             return std::invoke(*sp, std::forward<Args>(args)...);
//         }
//         return std::unexpected(bad_proxy_call("Proxy: callable target is expired or uninitialized"));
//     }

//     bool is_valid() const {
//         return func_ptr.has_value() || !func_wp.expired();
//     }
// };

// // Functor (callable object with operator())
// template <Functor F>
// class Proxy<F> {
//     std::optional<std::reference_wrapper<F>> ref;
//     std::weak_ptr<F> wp;

// public:
//     explicit Proxy(F& f) : ref(std::ref(f)) {}
//     explicit Proxy(std::weak_ptr<F> wp) : wp(std::move(wp)) {}

//     template <typename... Args>
//     auto operator()(Args&&... args) const
//         -> std::expected<std::invoke_result_t<F, Args...>, bad_proxy_call>
//     {
//         if (ref) {
//             return std::invoke(ref->get(), std::forward<Args>(args)...);
//         }
//         if (auto sp = wp.lock()) {
//             return std::invoke(*sp, std::forward<Args>(args)...);
//         }
//         return std::unexpected(bad_proxy_call("Proxy: callable target is expired or uninitialized"));
//     }

//     bool is_valid() const {
//         return ref.has_value() || !wp.expired();
//     }
// };

// // std::function
// template <StdFunction F>
// class Proxy<F> {
//     std::optional<F> fn;

// public:
//     explicit Proxy(F f) : fn(std::move(f)) {}

//     template <typename... Args>
//     auto operator()(Args&&... args) const
//         -> std::expected<std::invoke_result_t<F, Args...>, bad_proxy_call>
//     {
//         if (!fn.has_value()) {
//             return std::unexpected(bad_proxy_call("std::function proxy target uninitialized"));
//         }

//         try {
//             return std::invoke(*fn, std::forward<Args>(args)...);
//         } catch (const std::bad_function_call& e) {
//             return std::unexpected(bad_proxy_call("std::function call to empty target"));
//         }
//     }

//     bool is_valid() const {
//         return fn.has_value();
//     }
// };

// // helper function to extract class type from member function pointer
// template <typename T>
// struct memfn_class;

// template <typename R, typename C, typename... Args>
// struct memfn_class<R (C::*)(Args...)> {
//     using type = C;
// };

// template <typename R, typename C, typename... Args>
// struct memfn_class<R (C::*)(Args...) const> {
//     using type = const C;
// };

// // Member function pointer
// template <MemberFunctionPtr F>
// class Proxy<F> {
// public:
//     using ObjectType = typename memfn_class<F>::type;

// private:
//     F member_ptr;
//     ObjectType* object_ptr;

// public:
//     Proxy(F fn, ObjectType* obj) : member_ptr(fn), object_ptr(obj) {}

//     template <typename... Args>
//     auto operator()(Args&&... args) const
//         -> std::expected<std::invoke_result_t<F, ObjectType*, Args...>, bad_proxy_call>
//     {
//         if (object_ptr) {
//             return std::invoke(member_ptr, object_ptr, std::forward<Args>(args)...);
//         }
//         return std::unexpected(bad_proxy_call("Member function proxy object is null"));
//     }

//     bool is_valid() const {
//         return object_ptr != nullptr;
//     }
// };

// } // namespace ndof

//     template <typename... Args>
//         requires (std::is_invocable_v<F, Args...> and !QualifiedBy<F,Qualifier::NoExcept>)
//     auto operator()(Args&&... args)
//     {
//         return std::invoke(func, std::forward<Args>(args)...);
//     }

//     template <typename... Args>
//         requires (std::is_invocable_v<F, Args...> and QualifiedBy<F,Qualifier::NoExcept>)
//     auto operator()(Args&&... args) noexcept
//     {
//         try {
//             return std::invoke(func, std::forward<Args>(args)...);
//         }
//         catch (const std::exception& e) {

//         }
//         catch (...) {
//             // Handle other exceptions
//         }
//     }

// };

// template<MemberFunction auto f>
// struct Inner<f> {

//     template<typename ...A>
//     static R execute(std::any& a, A&&...) noexcept(is_noexcept())
//         requires (std::is_invocable_r_v<R, F, A...> )
//     {
//         return t(std::forward<A>(args)...);
//     }

//     template<typename ...A>
//     R execute(std::any& a, A&&...) const noexcept(is_noexcept())
//         requires (std::is_invocable_r_v<R, F, A...> && !volatile_required)
//     {
//         return f(std::forward<A>(args)...);
//     }

//     template<typename ...A>
//     R execute(std::any& a, A&&...) volatile noexcept(is_noexcept())
//         requires (std::is_invocable_r_v<R, F, A...> && !const_required)
//     {
//         return f(std::forward<A>(args)...);
//     }

//     template<typename ...A>
//     R execute(std::any& a, A&&...)  noexcept(is_noexcept())
//         requires
//             (std::is_invocable_r_v<R, F, A...>
//                 && !(const_required || !volatile_required))
//     {
//         return t(std::forward<A>(args)...);
//     }
// };

#endif // NDOF_OS_CALLABLE_TRAITS_PROXY_HPP
