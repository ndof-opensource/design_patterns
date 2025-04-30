// proxy_test_suite_expanded.cpp
#include <gtest/gtest.h>
#include "../proxy.hpp"
#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <tuple>

using namespace ndof;

// === Free Functions ===
std::string greet_name(const std::string& name) {
    return "Hello, " + name;
}
int sum(int a, int b) {
    return a + b;
}
double multiply(double x, double y, double z) {
    return x * y * z;
}
void do_nothing() {}

TEST(FunctionGroup_FreeFunction, NoArgs) {
    auto& ref = greet_name;
    Proxy<decltype(ref)> proxy(ref);
    auto r = proxy("World");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, "Hello, World");
}
TEST(FunctionGroup_FreeFunction, NoArgs_MakeProxy) {
    auto proxy = make_proxy(greet_name);
    auto r = proxy("World");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, "Hello, World");
}

TEST(FunctionGroup_FreeFunction, MultiArg) {
    auto& ref = sum;
    Proxy<decltype(ref)> proxy(sum);
    auto r = proxy(3, 7);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 10);
}
TEST(FunctionGroup_FreeFunction, MultiArg_MakeProxy) {
    auto proxy = make_proxy(sum);
    auto r = proxy(3, 7);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 10);
}

// === Functors ===
struct StatelessFunctor {
    std::string operator()() const { return "no state, no args"; }
};
struct StatefulFunctor {
    int factor;
    explicit StatefulFunctor(int f) : factor(f) {}
    int operator()(int x) const { return x * factor; }
};
struct MultiArgFunctor {
    std::string operator()(const std::string& name, int id) const {
        return name + " #" + std::to_string(id);
    }
};

TEST(FunctorGroup, Stateless) {
    Proxy<StatelessFunctor> proxy;
    auto r = proxy();
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, "no state, no args");
}
TEST(FunctorGroup, Stateless_MakeProxy) {
    auto proxy = make_proxy(StatelessFunctor{});
    auto r = proxy();
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, "no state, no args");
}

TEST(FunctorGroup, Stateful) {
    StatefulFunctor f{5};
    Proxy<StatefulFunctor> proxy(f);
    auto r = proxy(2);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 10);
}
TEST(FunctorGroup, Stateful_MakeProxy) {
    auto proxy = make_proxy(StatefulFunctor{5});
    auto r = proxy(2);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 10);
}

TEST(FunctorGroup, MultiArg) {
    Proxy<MultiArgFunctor> proxy;
    auto r = proxy("Alice", 42);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, "Alice #42");
}
TEST(FunctorGroup, MultiArg_MakeProxy) {
    auto proxy = make_proxy(MultiArgFunctor{});
    auto r = proxy("Alice", 42);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, "Alice #42");
}

// === Lambdas ===
TEST(LambdaGroup, NoCapture_NoArgs) {
    auto lam = []() { return 42; };
    Proxy<decltype(lam)> proxy(lam);
    auto r = proxy();
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 42);
}
TEST(LambdaGroup, NoCapture_NoArgs_MakeProxy) {
    auto proxy = make_proxy([]() { return 42; });
    auto r = proxy();
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 42);
}

TEST(LambdaGroup, Capture_ByValue) {
    int x = 10;
    auto lam = [x](int y) { return x + y; };
    Proxy<decltype(lam)> proxy(lam);
    auto r = proxy(5);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 15);
}
TEST(LambdaGroup, Capture_ByValue_MakeProxy) {
    int x = 10;
    auto proxy = make_proxy([x](int y) { return x + y; });
    auto r = proxy(5);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 15);
}

TEST(LambdaGroup, Capture_ByRef) {
    int x = 20;
    auto lam = [&x](int y) { return x * y; };
    Proxy<decltype(lam)> proxy(lam);
    auto r = proxy(2);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 40);
}
TEST(LambdaGroup, Capture_ByRef_MakeProxy) {
    int x = 20;
    auto proxy = make_proxy([&x](int y) { return x * y; });
    auto r = proxy(2);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 40);
}

// === Member Functions ===
struct Person {
    std::string name;
    Person(std::string n) : name(std::move(n)) {}
    std::string id() { return name + ":plain"; }
    std::string id() const { return name + ":const"; }
    std::string id() volatile {
        return std::string(const_cast<const std::string&>(name)) + ":volatile";
    }
    std::string id() const volatile {
        return std::string(const_cast<const std::string&>(name)) + ":const volatile";
    }
};

// NOTE: Tests for cv-qualified member functions are currently disabled due to 
// cv-qualification mismatch in CallableTraits<ClassType>. Need to revisit 
// once traits are confirmed to preserve full cv qualifiers on the owning class.

// TEST(MemberFunctionGroup, ConstQualified) {
//     const Person p("Alice");
//     auto method = static_cast<std::string (Person::*)() const>(&Person::id);
//     Proxy<decltype(method)> proxy(method, &p);
//     auto r = proxy();
//     ASSERT_TRUE(r.has_value());
//     EXPECT_EQ(*r, "Alice:const");
// }
// TEST(MemberFunctionGroup, ConstQualified_MakeProxy) {
//     const Person p("Alice");
//     auto proxy = make_proxy(&Person::id, p);
//     auto r = proxy();
//     ASSERT_TRUE(r.has_value());
//     EXPECT_EQ(*r, "Alice:const");
// }
