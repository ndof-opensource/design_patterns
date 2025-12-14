#include <print>
#include <memory_resource>
#include <memory>
#include <type_traits>


template<typename T> 
struct ICloneable {
    friend T;

    // Allocator-aware deleter using allocator_traits
    template<typename Alloc>
    struct AllocDeleter {
        using allocator_type = Alloc;
        using alloc_traits = std::allocator_traits<Alloc>;

        allocator_type alloc{};

        AllocDeleter() = default;
        explicit AllocDeleter(allocator_type a) : alloc(std::move(a)) {}

        // Enable conversion from AllocDeleter<rebind> for base/derived pointer conversions
        template<typename OtherAlloc>
        requires std::is_same_v<
            typename std::allocator_traits<OtherAlloc>::template rebind_alloc<T>,
            Alloc
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

    // Default allocator and deleter types for convenience
    using DefaultAlloc = std::pmr::polymorphic_allocator<T>;
    using PmrDeleter = AllocDeleter<DefaultAlloc>;

    virtual ~ICloneable() = default;

private:
    std::pmr::memory_resource* mem_res = nullptr; 
    // private, default constructor: no data members, private + friend enforces proper CRTP
    ICloneable() = default;
    // private, default copy constructor: no data members to copy, private + friend prevents 
    // object slicing while allowing derived classes to be copied normally
    ICloneable(const ICloneable&) = default;

public:

    // Clone using a polymorphic_allocator.
    // Uses select_on_container_copy_construction for proper allocator propagation.
    // Contract: the allocator's memory_resource must outlive the clone.
    template<typename U>
    requires std::is_base_of_v<T, U>
    std::unique_ptr<std::remove_reference_t<U>, AllocDeleter<std::pmr::polymorphic_allocator<U>>> 
    clone(this U const& self, std::pmr::polymorphic_allocator<U> alloc = {}) {
        using Alloc = std::pmr::polymorphic_allocator<U>;
        using alloc_traits = std::allocator_traits<Alloc>;
        using Deleter = AllocDeleter<Alloc>;

        // Use select_on_container_copy_construction for proper propagation semantics.
        Alloc new_alloc = alloc_traits::select_on_container_copy_construction(alloc);

        // If using default resource and the object has a stored resource, use it
        if (new_alloc.resource() == std::pmr::get_default_resource() && self.mem_res) {
            new_alloc = Alloc{self.mem_res};
        }

        // Delegate to virtual do_clone
        std::unique_ptr<T, PmrDeleter> r = self.do_clone(new_alloc.resource());
        T* obj = r.release();
        
        return std::unique_ptr<std::remove_reference_t<U>, Deleter>(
            static_cast<U*>(obj), 
            Deleter(new_alloc)
        );
    }

protected:
    /* TODO: Discussion point.
    *  this relies on each derived class properly overriding do_clone() to construct its 
    * actual type. If a class in the middle of the hierarchy doesn't override it, the 
    * downcast from T* to U* could be unsafe (e.g., if Poodle inherits from Dog but only 
    * Dog::do_clone() exists, it would construct a Dog but cast to Poodle*).
    * 
    * We could enforce this in one of at least two ways:
    * 1. Runtime check in clone() using dynamic_cast.
    * 2. Made do_clone() pure virtual and provide a helper so derived classes must override do_clone().
    * 
    */
    virtual std::unique_ptr<T, PmrDeleter> do_clone(std::pmr::memory_resource* mr = nullptr) const {
        using D = T;
        using Alloc = std::pmr::polymorphic_allocator<D>;
        using alloc_traits = std::allocator_traits<Alloc>;

        auto const& self = static_cast<D const&>(*this);
        
        if (!mr) {
            // Plain new/delete path - use default allocator
            Alloc alloc{};
            D* ptr = alloc_traits::allocate(alloc, 1);
            try {
                alloc_traits::construct(alloc, ptr, self);
                return std::unique_ptr<T, PmrDeleter>(ptr, PmrDeleter(alloc));
            } catch (...) {
                alloc_traits::deallocate(alloc, ptr, 1);
                throw;
            }
        }

        // PMR path - use provided memory resource
        Alloc alloc{mr};
        D* ptr = alloc_traits::allocate(alloc, 1);
        try {
            // Use allocator_traits::construct which may use allocator-aware construction
            alloc_traits::construct(alloc, ptr, self);
            return std::unique_ptr<T, PmrDeleter>(ptr, PmrDeleter(alloc));
        } catch (...) {
            alloc_traits::deallocate(alloc, ptr, 1);
            throw;
        }
    }
};

template<typename T>
concept Cloneable = std::is_base_of_v<ICloneable<T>, T>;
