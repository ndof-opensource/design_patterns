
#include <gtest/gtest.h>
#include "../proxy.hpp"
#include <memory>
#include <string>

using ndof::Proxy;

struct SimpleFunction {
    std::string operator()(const std::string& name) const {
        return "Hello, " + name;
    }
};

std::string greet(const std::string& name) {
    return "Hi, " + name;
}

TEST(ProxyTest, FunctionObjectReference) {
    SimpleFunction func;
    Proxy<SimpleFunction> proxy(func);
    auto result = proxy("Alice");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "Hello, Alice");
}

TEST(ProxyTest, FunctionObjectWeakPtr) {
    auto func = std::make_shared<SimpleFunction>();
    Proxy<SimpleFunction> proxy(std::weak_ptr(func));
    auto result = proxy("Bob");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "Hello, Bob");

    func.reset();  // expire weak_ptr
    auto result2 = proxy("Charlie");
    ASSERT_FALSE(result2.has_value());
}

TEST(ProxyTest, FunctionPointer) {
    Proxy<decltype(&greet)> proxy(&greet);
    auto result = proxy("David");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "Hi, David");
}

TEST(ProxyTest, NullFunctionPointer) {
    Proxy<decltype(&greet)> proxy(nullptr);
    auto result = proxy("Eve");
    ASSERT_FALSE(result.has_value());
}
