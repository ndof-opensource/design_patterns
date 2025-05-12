#include <gtest/gtest.h>
#include "ndof-os/callable_traits/proxy.hpp"
#include <string>
#include <memory>

using namespace ndof;

// ✅ Free functions (match StandaloneFunction<F>)
std::string greet(const std::string& name) {
    return "Hello, " + name;
}

std::string repeat(const std::string& word) {
    return word + " " + word;
}

static_assert(StandaloneFunction<decltype(greet)>);

// ✅ Proxy with function reference
TEST(ProxyTest, FreeFunctionReference) {
    Proxy<decltype(greet)> proxy(greet);

    auto result = proxy("Alice");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "Hello, Alice");
}

TEST(ProxyTest, FreeFunctionPointerWeakPtrValid) {
    using FnPtr = std::string (*)(const std::string&);
    auto func = std::make_shared<FnPtr>(&greet);
    std::weak_ptr<FnPtr> weak = func;

    Proxy<FnPtr> proxy(weak);  // FnPtr matches FunctionPtr<F> → StandaloneFunction

    auto result = proxy("Bob");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "Hello, Bob");
}

// ✅ Proxy with expired weak_ptr
TEST(ProxyTest, FreeFunctionPointerWeakPtrExpired) {
    using FnPtr = std::string (*)(const std::string&);
    std::weak_ptr<FnPtr> weak;

    {
        auto func = std::make_shared<FnPtr>(&greet);
        weak = func;
    }  // func is destroyed here

    Proxy<FnPtr> proxy(weak);
    auto result = proxy("Charlie");
    ASSERT_FALSE(result.has_value());
    
    // Check the actual error type/message
    const auto& error = result.error();
    EXPECT_STREQ(error.what(), "Proxy: callable target is expired or uninitialized");
}

