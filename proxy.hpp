#pragma once

#include <memory>
#include <optional>
#include <functional>
#include <type_traits>
#include <expected>
#include <utility>
#include <stdexcept>
#include "../callable_traits/include/callable_concepts.hpp"

namespace ndof {

struct bad_proxy_call : std::bad_function_call {
    explicit bad_proxy_call(std::string msg) : _msg(std::move(msg)) {}
    const char* what() const noexcept override { return _msg.c_str(); }
private:
    std::string _msg;
};

template <Callable F>
class Proxy;

// Function (actual function type, not pointer)
template <Function F>
class Proxy<F> {
    F& func;
public:
    explicit Proxy(F& f) : func(f) {}

    template <typename... Args>
    auto operator()(Args&&... args) const
        -> std::expected<std::invoke_result_t<F, Args...>, bad_proxy_call>
    {
        return std::invoke(func, std::forward<Args>(args)...);
    }

    bool is_valid() const { return true; }
};

// Function pointer
template <FunctionPtr F>
class Proxy<F> {
    std::optional<F> func_ptr;
    std::weak_ptr<F> func_wp;

public:
    explicit Proxy(F f) : func_ptr(f) {}
    explicit Proxy(std::weak_ptr<F> wp) : func_wp(std::move(wp)) {}

    template <typename... Args>
    auto operator()(Args&&... args) const
        -> std::expected<std::invoke_result_t<F, Args...>, bad_proxy_call>
    {
        if (func_ptr && *func_ptr) {
            return std::invoke(*func_ptr, std::forward<Args>(args)...);
        }
        if (auto sp = func_wp.lock()) {
            return std::invoke(*sp, std::forward<Args>(args)...);
        }
        return std::unexpected(bad_proxy_call("Proxy: callable target is expired or uninitialized"));
    }

    bool is_valid() const {
        return func_ptr.has_value() || !func_wp.expired();
    }
};

// Functor (callable object with operator())
template <Functor F>
class Proxy<F> {
    std::optional<std::reference_wrapper<F>> ref;
    std::weak_ptr<F> wp;

public:
    explicit Proxy(F& f) : ref(std::ref(f)) {}
    explicit Proxy(std::weak_ptr<F> wp) : wp(std::move(wp)) {}

    template <typename... Args>
    auto operator()(Args&&... args) const
        -> std::expected<std::invoke_result_t<F, Args...>, bad_proxy_call>
    {
        if (ref) {
            return std::invoke(ref->get(), std::forward<Args>(args)...);
        }
        if (auto sp = wp.lock()) {
            return std::invoke(*sp, std::forward<Args>(args)...);
        }
        return std::unexpected(bad_proxy_call("Proxy: callable target is expired or uninitialized"));
    }

    bool is_valid() const {
        return ref.has_value() || !wp.expired();
    }
};

// std::function
template <StdFunction F>
class Proxy<F> {
    std::optional<F> fn;

public:
    explicit Proxy(F f) : fn(std::move(f)) {}

    template <typename... Args>
    auto operator()(Args&&... args) const
        -> std::expected<std::invoke_result_t<F, Args...>, bad_proxy_call>
    {
        if (!fn.has_value()) {
            return std::unexpected(bad_proxy_call("std::function proxy target uninitialized"));
        }
    
        try {
            return std::invoke(*fn, std::forward<Args>(args)...);
        } catch (const std::bad_function_call& e) {
            return std::unexpected(bad_proxy_call("std::function call to empty target"));
        }    
    }

    bool is_valid() const {
        return fn.has_value();
    }
};

// helper function to extract class type from member function pointer
template <typename T>
struct memfn_class;

template <typename R, typename C, typename... Args>
struct memfn_class<R (C::*)(Args...)> {
    using type = C;
};

template <typename R, typename C, typename... Args>
struct memfn_class<R (C::*)(Args...) const> {
    using type = const C;
};

// Member function pointer
template <MemberFunctionPtr F>
class Proxy<F> {
public:
    using ObjectType = typename memfn_class<F>::type;

private:
    F member_ptr;
    ObjectType* object_ptr;

public:
    Proxy(F fn, ObjectType* obj) : member_ptr(fn), object_ptr(obj) {}

    template <typename... Args>
    auto operator()(Args&&... args) const
        -> std::expected<std::invoke_result_t<F, ObjectType*, Args...>, bad_proxy_call>
    {
        if (object_ptr) {
            return std::invoke(member_ptr, object_ptr, std::forward<Args>(args)...);
        }
        return std::unexpected(bad_proxy_call("Member function proxy object is null"));
    }

    bool is_valid() const {
        return object_ptr != nullptr;
    }
};

} // namespace ndof

