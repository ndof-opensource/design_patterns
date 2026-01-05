#include <print>
#include <memory_resource>
#include <memory>
#include <type_traits>
#include <concepts>

template<typename T, typename Alloc = std::pmr::polymorphic_allocator<T>>
struct ICloneable {
    friend T;

    // Allocator-aware deleter using allocator_traits.
    // Templated on allocator type (A) to support rebinding between related types.
    template<typename A>
    struct AllocDeleter {
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
    [[no_unique_address]] Alloc allocator_{};
    
    // private, default constructor
    ICloneable() = default;
    // Constructor to set the allocator
    explicit ICloneable(Alloc a) : allocator_(std::move(a)) {}
    // Copy constructor: copies allocator
    ICloneable(const ICloneable&) = default;
    // Move constructor: moves allocator
    ICloneable(ICloneable&&) = default;
    // Copy and move assignment deleted
    ICloneable& operator=(const ICloneable&) = delete;
    ICloneable& operator=(ICloneable&&) = delete;

public:

    // Clone using an allocator.
    // Uses select_on_container_copy_construction for proper allocator propagation.
    // Delegates to virtual do_clone() for polymorphic construction.
    // Contract: the allocator must outlive the clone.
    template<typename U>
    requires std::is_base_of_v<T, U>
    std::unique_ptr<U, AllocDeleter<typename std::allocator_traits<Alloc>::template rebind_alloc<U>>> 
    clone(this U const& self, Alloc alloc = Alloc{}) {
        using alloc_traits = std::allocator_traits<Alloc>;
        using ReboundAlloc = typename alloc_traits::template rebind_alloc<U>;
        using ReboundDeleter = AllocDeleter<ReboundAlloc>;

        // If passed a default-constructed allocator, try to use the object's stored allocator.
        // This check is specific to polymorphic_allocator which has a resource() method.
        if constexpr (requires { new_alloc.resource(); }) {
            if (new_alloc.resource() == std::pmr::get_default_resource()) {
                new_alloc = self.allocator_;
            }
        }

        // Delegate to virtual do_clone for polymorphic construction
        std::unique_ptr<T, Deleter> r = self.do_clone(new_alloc);
        
        // Transfer ownership with rebound deleter
        Deleter old_deleter = r.get_deleter();
        T* obj = r.release();
        
        // Rebind the allocator from T to U
        ReboundAlloc rebound_alloc(old_deleter.alloc);
        
        return std::unique_ptr<U, ReboundDeleter>(
            static_cast<U*>(obj), 
            ReboundDeleter(rebound_alloc)
        );
    }

    // TODO: make a new clone() method with signature clone(this U const& self)
    // can chain these methods together to avoid code duplication

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
