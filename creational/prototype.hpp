#pragma once

#include <memory>
#include <concepts>
#include <type_traits>

template <typename T>
concept HasCloneMethod = requires(const T& obj) {
    { obj.clone() } -> std::convertible_to<std::unique_ptr<T>>;
};

template <typename T>
concept IsCloneable = std::copy_constructible<std::remove_cvref_t<T>> ||
                      HasCloneMethod<std::remove_cvref_t<T>>;

class Prototype {
private:
    
    // Internal, private, abstract interface to define the requirements and 
    // capabilities of the prototype, i.e. the ability to clone()
    class HolderBase {
    public:
        virtual ~HolderBase() = default;
        virtual std::unique_ptr<HolderBase> clone() const = 0;
    };

    // Internal, private, templated concrete implementation of HolderBase 
    // to hold the actual object to be cloned and implement the clone function
    template <typename T>
    class Holder : public HolderBase {
    private:
        T held_object;
    
    public:
        Holder(const T& obj) : held_object(obj) {}

        Holder(T&& obj) : held_object(std::move(obj)) {}

        std::unique_ptr<HolderBase> clone() const override {
            if constexpr (HasCloneMethod<T>) {
                // prefer to use class's clone() method if it exists
                // then move cloned object into a Holder<T>
                std::unique_ptr<T> cloned_user_obj_ptr = held_object.clone();
                return std::make_unique<Holder<T>>(std::move(*cloned_user_obj_ptr));
            } else {
                // otherwise, fall back to copy constructor
                return std::make_unique<Holder<T>>(held_object);
            }
        }
    };

    // pointer to the implementation of the internal Holder object
    std::unique_ptr<HolderBase> pimpl;

public:
    // Constructor is constrained to only accept types that are IsCloneable
    template <typename T>
        requires IsCloneable<T>
    Prototype(const T& obj) : pimpl(std::make_unique<Holder<T>>(obj)) {}

    // Prototype's clone() function simply invokes the Holder implementation's
    // clone() function
    Prototype clone() const {
        return Prototype(pimpl->clone());
    }

private:
    // Internal, helper constructor to create new Prototype wrapper around new 
    // cloned Holder object during cloning process
    Prototype(std::unique_ptr<HolderBase> ptr) : pimpl(std::move(ptr)) {}
};
