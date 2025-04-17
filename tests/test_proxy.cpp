
#include <gtest/gtest.h>
#include "../proxy.hpp"
#include <memory>
#include <string>
#include <functional>

using namespace ndof;

// Free function
std::string greet(const std::string& name) {
    return "Hello, " + name;
}
int square(int x) {
    return x * x;
}

// Functor
struct Multiplier {
    int factor;
    Multiplier(int f) : factor(f) {}
    int operator()(int x) const { return x * factor; }
};

struct Greeter {
    std::string operator()(const std::string& name) const {
        return "Hi, " + name;
    }
};

// Member function
struct Person {
    std::string name;
    Person(std::string n) : name(std::move(n)) {}
    std::string say_hello(const std::string& to) const {
        return "Hi " + to + ", I'm " + name;
    }
    int add(int x) const { return x + 10; }
};

// Test Function
TEST(ProxyTest, FunctionReference_Success) {
    Proxy<decltype(greet)> proxy(greet);
    auto result = proxy("Alice");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "Hello, Alice");
}

TEST(ProxyTest, FunctionReference_Failure) {
    // impossible to fail: actual reference
    SUCCEED();
}

// Test Function Pointer
TEST(ProxyTest, FunctionPointer_Success) {
    using FnPtr = decltype(&square);
    Proxy<FnPtr> proxy(&square);
    auto result = proxy(4);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 16);
}

TEST(ProxyTest, FunctionPointer_Failure) {
    using FnPtr = decltype(&square);
    Proxy<FnPtr> proxy(static_cast<FnPtr>(nullptr));
    auto result = proxy(3);
    ASSERT_FALSE(result.has_value());
    EXPECT_STREQ(result.error().what(), "Proxy: callable target is expired or uninitialized");
}

// Test Functor
TEST(ProxyTest, FunctorReference_Success) {
    Multiplier m(5);
    Proxy<Multiplier> proxy(m);
    auto result = proxy(2);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 10);
}

// Test Functor Weak Pointer
TEST(ProxyTest, FunctorWeakPtr_Success) {
    Multiplier m(3);
    Proxy<Multiplier> proxy(m);
    auto result = proxy(4);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 12);
}

TEST(ProxyTest, FunctorWeakPtr_Failure) {
    std::weak_ptr<Multiplier> weak;
    {
        auto sp = std::make_shared<Multiplier>(3);
        weak = sp;
    }
    Proxy<Multiplier> proxy(weak);
    auto result = proxy(2);
    ASSERT_FALSE(result.has_value());
    EXPECT_STREQ(result.error().what(), "Proxy: callable target is expired or uninitialized");
}

// Test std::function
TEST(ProxyTest, StdFunction_Success) {
    std::function<std::string(const std::string&)> fn = greet;
    Proxy<decltype(fn)> proxy(fn);
    auto result = proxy("Bob");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "Hello, Bob");
}

TEST(ProxyTest, StdFunction_Failure) {
    std::function<std::string(const std::string&)> fn;
    Proxy<decltype(fn)> proxy(fn);
    auto result = proxy("Bob");
    ASSERT_FALSE(result.has_value());
    EXPECT_STREQ(result.error().what(), "std::function call to empty target");
}

// Test MemberFunctionPtr
TEST(ProxyTest, MemberFunction_Success) {
    Person p("Charlie");
    auto fn = &Person::say_hello;
    Proxy<decltype(fn)> proxy(fn, &p);
    auto result = proxy("Alice");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "Hi Alice, I'm Charlie");
}

TEST(ProxyTest, MemberFunction_Failure) {
    auto fn = &Person::add;
    using ProxyType = Proxy<decltype(fn)>;
    ProxyType proxy(fn, static_cast<typename ProxyType::ObjectType*>(nullptr));
    auto result = proxy(5);
    ASSERT_FALSE(result.has_value());
    EXPECT_STREQ(result.error().what(), "Member function proxy object is null");
}

TEST(ProxyTest, Lambda_Success) {
    auto lambda = [](int x) { return x + 42; };
    Proxy<decltype(lambda)> proxy(lambda);
    auto result = proxy(8);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 50);
}

TEST(ProxyTest, Lambda_WeakPtrExpired) {
    auto original_lambda = [](int x) { return x * 2; };
    using LambdaType = decltype(original_lambda);

    std::weak_ptr<LambdaType> weak;

    {
        auto sp = std::make_shared<LambdaType>(original_lambda);
        weak = sp;
    }

    Proxy<LambdaType> proxy(weak);
    auto result = proxy(5);
    ASSERT_FALSE(result.has_value());
    EXPECT_STREQ(result.error().what(), "Proxy: callable target is expired or uninitialized");
}

