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

    
    template<class A, class T>
    using rebind_t = typename std::allocator_traits<A>::template rebind_alloc<T>;
 
    template<class A_provided, class A_callee>
    concept AllocCompatibleFor =
        requires { typename rebind_t<A_provided,  A_callee>; }  &&
        std::constructible_from<rebind_t<A_provided, A_callee>, A_callee>;

    template<Callable F>
    struct as_function{
        using type = typename GenerateFunction<F::qualifiers, typename F::ReturnType, typename F::ArgTypes>::type;
    };

    template<Callable F>
    using as_function_t = typename as_function<F>::type;
}

// TODO: in callable_type_generator.hpp, from line 264, the types defined should be functions, not function pointers.
namespace ndof
{

    template <Function Fn, typename Alloc = std::allocator<Fn>>
    struct Proxy
    {
    private:
        // TODO: handle is_nothrow_convertible for arguments?
        // TODO: check noexcept for all methods declared here.

        using ArgTypes = typename CallableTraits<F>::ArgTypes;
        using ReturnType = typename CallableTraits<F>::ReturnType;
        using Members = std::tuple<std::any, Alloc>

        mutable Members members;

        consteval static bool is_noexcept() { return QualifiedBy<Fn, Qualifier::NoExcept>; }
        consteval static bool is_void_return() { return std::is_void_v<ReturnType>; }

        template<typename F, typename ...A>
        consteval static bool has_operator_parens() { 
            return requires(F f, A... a) { f(a...); }; 
        }

        template <typename>
        struct Inner;

      
        template<typename R, typename... A>
        struct Inner<R(A...) noexcept(is_noexcept())> {
            virtual ~Inner() = default;
            virtual ReturnType invoke(A&&...) noexcept(is_noexcept()) = 0;

            // TODO: add clone method that takes an allocator and returns a new instance of Inner with the same type.

        };

        // TODO: Test noexcept propagation in mismatched types.

        template<auto f, typename... A>
        struct InnerCallable;

        template<auto f, typename... A>
            requires StandaloneFunction<as_function_t<decltype(f)>>
        struct InnerCallable<f,A...> : Inner<f> {
            ReturnType invoke(A&&... a) noexcept(is_noexcept()) override {
                return std::invoke(f, std::forward<A>(a)...);
            }
        };
  
        // TODO: Member function reference?
        // TODO: Prototype this in godbolt. 
        //.       Concerned about the type deduction of T in the constructor.
        // TODO: A functor will have to be unique.
        template<auto f, typename... A>
            requires MemberFunctionPtr<decltype(f)> 
        struct InnerCallable<f,A...> : Inner<as_function_t<decltype(f)>> {
            using F = decltype(f);
            using T = typename CallableTraits<F>::ClassType;

            using InnerAlloc = rebind_t<Alloc, InnerCallable<f,A...>>;
            InnerAlloc alloc;
            T function_obj;

            // TODO: deduction guide.
            // TODO: prototype this.  can T be deduced correctly?
            template<typename U>
                requires std::is_convertible_v<U, T>
            InnerCallable(const T& obj, const Alloc& alloc) 
                : inner_alloc{alloc}, function_obj(std::make_obj_using_allocator<T>(inner_alloc, obj)) {
            }

            ReturnType invoke(A&&... a) noexcept(is_noexcept()) override {
                return std::invoke(f, function_obj, std::forward<A>(a)...);
            }
        };

        Alloc alloc;
        Inner<Fn>* inner;
  
        template<typename A>
        using InnerAlloc = rebind_t<Alloc, InnerCallable<A, ArgTypes...>>


    public:

        // TODO: The following two constructors do the same thing.  Condense.
        // TODO: Handle exceptions and properly attribute as noexcept as necessary.
        template<AllocCompatibleFor<Alloc> A>
        Proxy(StandaloneFunction auto f, const A alloc = A{}) noexcept(is_noexcept())
            : inner(std::uninitialized_construct_using_allocator<InnerCallable<f, ArgTypes...>>(alloc)), alloc(alloc) {

        }
         
        template<AllocCompatibleFor<Alloc> A>
        Proxy(Functor auto&& f, const A alloc = A{}) noexcept(is_noexcept())
            : alloc(alloc), inner(std::uninitialized_construct_using_allocator<InnerCallable<f, ArgTypes...>>(alloc))  {
            // TODO: Implement.
        }

        // TODO: Call the InnerCallable<f ,ArgTypes...> constructor by taking the compile time mfp.
        template<
            typename T, 
            auto mfp,
            AllocCompatibleFor<Alloc> A>
        Proxy(T&& t, A alloc = A{}) noexcept(is_noexcept()) // TODO: Implement member initializer list.
         {
            
        }
             
        template<
            typename T, 
            AllocCompatibleFor<Alloc> A>
        Proxy(T&& t,  A alloc = A{})
            // TODO: should check the operator() of T is compatible with Fn.
            requires requires {
                &T::operator(); } && 
                ParameterCompatible<Fn, decltype(&T::operator())> {
            // TODO: Implement.
        }

        // TODO: Copy constructor.
        template<AllocCompatibleFor<Alloc> A>
        Proxy(const Proxy<Fn,A>& other) {
            // TODO: Implement.
        }

        // TODO: Move constructor.
        template<AllocCompatibleFor<Alloc> A>
        Proxy(Proxy<Fn,A>&& other) {
            // TODO: Implement.
        }

        // TODO: Copy assignment operator.
        template<AllocCompatibleFor<Alloc> A>
        Proxy<Fn,A>& operator=(const Proxy<Fn,A>& other) {
            // TODO: Implement.
        }

        // TODO: Move assignment operator.
        template<AllocCompatibleFor<Alloc> A>
        Proxy<Fn,A>& operator=(Proxy<Fn,A>&& other) {
            // TODO: Implement.
        }

        // TODO: Destructor.
        ~Proxy() {
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