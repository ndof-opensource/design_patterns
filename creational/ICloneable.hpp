#include <print>
#include <memory_resource>
#include <memory>
#include <type_traits>


template<typename T> 
struct ICloneable{
    friend T;

    struct PmrDeleter {
        using DestroyFn = void(*)(ICloneable*, std::pmr::memory_resource*) noexcept;

        std::pmr::memory_resource* mr = nullptr;
        DestroyFn fn = nullptr;

        void operator()(ICloneable* p) const noexcept {
            if (!p) return;

            if (fn && mr) {
                // destroy + deallocate using the resource
                fn(p, mr);
            } else {
                // fallback: normal delete
                delete p;
            }
        }

    };

    virtual ~ICloneable() = default;

private:

    std::pmr::memory_resource* mr = nullptr; // TODO: Change this name to avoid confusion with the memory resource parameter of the clone method?
    // private, default constructor: no data members, private + friend enforces proper CRTP
    ICloneable() = default;
    // private, default copy constructor: no data members to copy, private + friend prevents 
    // object slicing while allowing derived classes to be copied normally
    ICloneable(const ICloneable&) = default;

    // Static destruction logic specific to T, but with a uniform signature.
    static void destroy_impl(ICloneable* base, std::pmr::memory_resource* mr) noexcept {
        auto* d = static_cast<T*>(base);
        d->~T();
        mr->deallocate(d, sizeof(T), alignof(T));
    }

public:

    // Clone into the given memory resource.
    // Contract: the caller guarantees that `mr` outlives the clone.
    virtual std::unique_ptr<T, PmrDeleter> clone(std::pmr::memory_resource* mr = nullptr) const {
        using D = T;
        auto const& self = static_cast<D const&>(*this);
        // instantiate a polymorphic allocator using the memory resource for the type of T

        if (!mr) {
            // Plain new/delete path
            return std::unique_ptr<T, PmrDeleter>(new D(self), PmrDeleter{}); // default-constructed deleter => delete
        }

        // Allocate object storage from the given memory_resource
        void* mem = mr->allocate(sizeof(D), alignof(D));
        try {
            // NOTE: D must have: D(D const&, std::pmr::memory_resource*)
            D* p = new (mem) D(self, mr);
            return std::unique_ptr<T, PmrDeleter>(p, PmrDeleter{ mr, &destroy_impl });
        } catch (...) {
            mr->deallocate(mem, sizeof(D), alignof(D));
            throw;
        }
    }
};

template<typename T>
concept Cloneable = std::is_base_of_v<ICloneable<T>, T>;
