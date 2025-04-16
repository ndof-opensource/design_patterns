#ifndef NDOF_PROXY_HPP
#define NDOF_PROXY_HPP
#include <memory>
#include <exception>
#include <expected>
#include <functional>

#include "../callable_traits/include/callable_concepts.hpp"

namespace ndof{

    struct bad_proxy_call : std::bad_function_call {
        explicit bad_proxy_call(std::string msg) : _msg(std::move(msg)) {}
        const char* what() const noexcept override { return _msg.c_str(); }
    private:
        std::string _msg;
    };

    template<Callable F>
    struct Proxy {

    };

    template<Callable F>
    requires StandaloneFunction<F>
    struct Proxy<F>{
        std::optional<std::reference_wrapper<F> > f_ref;
        std::weak_ptr<F> f_wp;

        Proxy(F& f_ref) : f_ref(std::ref(f_ref)), f_wp() {}

        Proxy(std::weak_ptr<F> f_wp) : f_wp(f_wp), f_ref(std::nullopt) {}


        // User burden: exception must inherit from std::exception.
        // Question: memory allocation? 
        // P2: tombstone. 
        template<typename ...A>
        auto operator()(A&&... a) const 
            -> std::expected<std::invoke_result_t<F, A...>, bad_proxy_call> {
            if (f_ref) {
                return std::invoke(*f_ref, std::forward<A>(a)...);
            }
            
            if (auto sp = f_wp.lock()) {
                return std::invoke(*sp, std::forward<A>(a)...);
            }

            return std::unexpected(bad_proxy_call{"Proxy: callable target is expired or uninitialized"});
        }
    };

    template<Callable F>
    requires (!StandaloneFunction<F>)
    struct Proxy<F>{
        template<typename ...A>
        auto operator()(A&&... a){

        }
    };


}

#endif

 

