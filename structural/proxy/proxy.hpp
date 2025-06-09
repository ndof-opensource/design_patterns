// #pragma once

// #include <memory>
// #include <optional>
// #include <functional>
// #include <type_traits>
// #include <expected>
// #include <utility>   
// #include <stdexcept>

// // Should be "callable_traits/callable_concepts.hpp"
#include <ndof-os/callable_traits/callable_concepts.hpp>
#include <ndof-os/callable_traits/callable_traits.hpp>
#include <ndof-os/callable_traits/qualified_by.hpp>

#include <variant>
#include <utility>

// namespace ndof {


// // TODO: add support for exception handling callbacks on enter/exit.  
// // TODO: Define a class that defines the callback object interface requirements alternatively.
 

namespace ndof {
  
    template <Function F, bool const_required = false, bool volatile_required = false, template<typename> typename Alloc = std::allocator >
    class Proxy{
    public:
        using typename CallableTraits<F>::ReturnType;
        using typename CallableTraits<F>::ArgTypes;

        // TODO: Move to CallableTraits.
        consteval static bool is_noexcept(){ return QualifiedBy<F, Qualifier::NoExcept>; }
        consteval static bool is_void_return(){ return std::is_void_v<R>; }

    private:
        std::any inner;

        template<auto f, typename Alloc = void>
        struct Inner;

        template<StandaloneFunction auto f>
        struct Inner<f> {

            template<typename ...A>
            static R execute(std::any& a, A&&... args) noexcept(is_noexcept()) 
                requires (std::is_invocable_r_v<R, F,  A...> )       
            {   
                return f(std::forward<A>(args)...);
            }
        }; 

        // template<MemberFunction auto mf>
        // struct Inner<mf> {

        //     template<typename ...A>
        //     static R execute(std::any& a, A&&... args) noexcept(is_noexcept()) 
        //         requires (std::is_invocable_r_v<R, F,  A...> )       
        //     {   
                
        //     }
        // }; 

        template<typename ...A>
        requires std::same_as<ArgumentHelper<A...>, ArgTypes>
        using ExecutePtr = ReturnType (*)(std::any&, A&&...);

        ExecutePtr execute_ptr;

    public:
        constexpr explicit Proxy(Standalone auto f) :  execute_ptr(f) noexcept {
        }

        template<typename ...A>
        R operator()(auto&&... args) noexcept(is_noexcept()) const volatile
            requires (std::is_invocable_r_v<R, F, A...> && StandaloneFunction<F>)       
        {   
            return std::invoke(execute_ptr, inner, std::forward<A>(args)...);
        }
        

        Alloc get_allocator(){

        }

    };



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

    }; 
    

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