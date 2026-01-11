#include <print>
#include <memory_resource>
#include <memory>
#include <type_traits>
#include <concepts>

// TODO: we use std::convertible_to in constructor requires clause. Can we delete this concept?
template<typename Alloc1, typename Alloc2>
concept is_allocator_compatible = requires (Alloc2 a2) {
    Alloc1(a2);
};

template<typename T, typename InputAlloc = std::pmr::polymorphic_allocator<void>>
struct ICloneable {
    friend T;
    
    // TODO: confirm with Bob this is what he wants (rebind the allocator we were given to T)
    using Alloc = typename std::allocator_traits<InputAlloc>::template rebind_alloc<T>;
    // Allocator-aware deleter using allocator_traits.
    // Templated on allocator type (A) to support rebinding between related types.
    template<typename A>
    struct AllocDeleter { // TODO: clean up deleter after changes from talk 250108
        using allocator_type = A;
        using alloc_traits = std::allocator_traits<A>;

        allocator_type alloc{};

        AllocDeleter() = default;
        explicit AllocDeleter(allocator_type a) : alloc(std::move(a)) {}

        // Enable conversion from AllocDeleter<rebind> for base/derived pointer conversions
        template<typename OtherAlloc>
        requires std::same_as<
            typename std::allocator_traits<OtherAlloc>::template rebind_alloc<typename alloc_traits::value_type>,
            A
        >
        AllocDeleter(const AllocDeleter<OtherAlloc>& other) 
            : alloc(other.alloc) {}

        void operator()(ICloneable* p) const noexcept {
            if (!p) return;
            auto* derived = static_cast<T*>(p);
            allocator_type a = alloc;
            alloc_traits::destroy(a, derived);
            alloc_traits::deallocate(a, derived, 1);
        }
    };

    // Convenient alias for the primary allocator's deleter
    using Deleter = AllocDeleter<Alloc>;

    virtual ~ICloneable() = default;

private:
    // Store the allocator for default cloning. [[no_unique_address]] allows
    // stateless allocators (like std::allocator) to take zero space.
    [[no_unique_address]] Alloc allocator_{}; // TODO: store templated on T
    
    // private, default constructor
    ICloneable() = default;
    // Constructor to set the allocator. Accepts any allocator convertible to Alloc.
    template<typename PassedAlloc>
    requires std::convertible_to<PassedAlloc, Alloc>
    explicit ICloneable(PassedAlloc a) : allocator_(Alloc(std::move(a))) {}
    // Copy constructor: copies allocator
    ICloneable(const ICloneable&) = default;
    // Move constructor: moves allocator
    ICloneable(ICloneable&&) = default;
    // TODO: Discuss - extended copy/move constructors may be redundant with clone().
    // They allow direct construction with a different allocator, but clone() already
    // provides allocator specification. Keep if derived classes need direct construction.
    // Extended copy: copy the object but use a different allocator
    template<typename PassedAlloc>
    requires std::convertible_to<PassedAlloc, Alloc>
    explicit ICloneable(const ICloneable&, PassedAlloc a) : allocator_(Alloc(std::move(a))) {}
    // Extended move: move the object but use a different allocator
    template<typename PassedAlloc>
    requires std::convertible_to<PassedAlloc, Alloc>
    explicit ICloneable(ICloneable&&, PassedAlloc a) : allocator_(Alloc(std::move(a))) {}
    // Copy and move assignment deleted
    ICloneable& operator=(const ICloneable&) = delete;
    ICloneable& operator=(ICloneable&&) = delete;

public:

    // Clone using a user-provided allocator.
    // Accepts any allocator convertible to Alloc (rebound to T).
    // Delegates to virtual do_clone() for polymorphic construction.
    // Contract: the allocator must outlive the clone.
    template<typename U, typename PassedAlloc>
    requires std::is_base_of_v<T, U>
        && std::convertible_to<PassedAlloc, Alloc>
    std::unique_ptr<U, AllocDeleter<typename std::allocator_traits<Alloc>::template rebind_alloc<U>>> 
    clone(this U const& self, PassedAlloc passed_alloc) {
        using alloc_traits = std::allocator_traits<Alloc>;
        using ReboundAlloc = typename alloc_traits::template rebind_alloc<U>;
        using ReboundDeleter = AllocDeleter<ReboundAlloc>;

        // Convert passed allocator to Alloc (rebound to T) for do_clone
        Alloc alloc(std::move(passed_alloc));
        
        // Delegate to virtual do_clone for polymorphic construction
        std::unique_ptr<T, Deleter> r = self.do_clone(alloc);
        
        // Transfer ownership with rebound deleter
        Deleter old_deleter = r.get_deleter();
        T* obj = r.release();
        
        // Rebind the allocator from T to U (no-op if U == T)
        ReboundAlloc rebound_alloc(old_deleter.alloc);
        
        return std::unique_ptr<U, ReboundDeleter>(
            static_cast<U*>(obj), 
            ReboundDeleter(rebound_alloc)
        );
    }

    // Clone using the stored allocator.
    // If U == T, no rebinding needed. Otherwise, rebinds to U.
    template<typename U>
    requires std::is_base_of_v<T, U>
    std::unique_ptr<U, AllocDeleter<typename std::allocator_traits<Alloc>::template rebind_alloc<U>>> 
    clone(this U const& self) {
        using alloc_traits = std::allocator_traits<Alloc>;
        using ReboundAlloc = typename alloc_traits::template rebind_alloc<U>;
        using ReboundDeleter = AllocDeleter<ReboundAlloc>;

        // Delegate to virtual do_clone using the stored allocator
        std::unique_ptr<T, Deleter> r = self.do_clone(self.allocator_);
        
        // Transfer ownership with rebound deleter
        Deleter old_deleter = r.get_deleter();
        T* obj = r.release();
        
        // Rebind the allocator from T to U (no-op if U == T)
        ReboundAlloc rebound_alloc(old_deleter.alloc);
        
        return std::unique_ptr<U, ReboundDeleter>(
            static_cast<U*>(obj), 
            ReboundDeleter(rebound_alloc)
        );
    }

protected:
    /* TODO: Discussion point.
    *  This relies on each derived class properly overriding do_clone() to construct its 
    *  actual type. If a class in the middle of the hierarchy doesn't override it, the 
    *  downcast from T* to U* could be unsafe (e.g., if Poodle inherits from Dog but only 
    *  Dog::do_clone() exists, it would construct a Dog but cast to Poodle*).
    * 
    *  We could enforce this in one of at least two ways:
    *  1. Runtime check in clone() using dynamic_cast.
    *  2. Make do_clone() pure virtual and provide a helper so derived classes must override.
    *
    *  Example override in derived class Dog:
    *    std::unique_ptr<Animal, Deleter> do_clone(Alloc alloc) const override {
    *        using DogAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Dog>;
    *        using dog_traits = std::allocator_traits<DogAlloc>;
    *        DogAlloc dog_alloc(alloc);
    *        Dog* ptr = dog_traits::allocate(dog_alloc, 1);
    *        dog_traits::construct(dog_alloc, ptr, *this);
    *        return std::unique_ptr<Animal, Deleter>(ptr, Deleter(dog_alloc));
    *    }
    */
    virtual std::unique_ptr<T, Deleter> do_clone(Alloc alloc) const {
        using alloc_traits = std::allocator_traits<Alloc>;

        auto const& self = static_cast<T const&>(*this);
        
        T* ptr = alloc_traits::allocate(alloc, 1);
        try {
            alloc_traits::construct(alloc, ptr, self);
            return std::unique_ptr<T, Deleter>(ptr, Deleter(alloc));
        } catch (...) {
            alloc_traits::deallocate(alloc, ptr, 1);
            throw;
        }
    }
};

template<typename T>
concept Cloneable = std::is_base_of_v<ICloneable<T>, T>;
