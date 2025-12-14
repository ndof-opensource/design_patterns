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

    std::pmr::memory_resource* mem_res = nullptr; 
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
    template<typename U>
    requires std::is_base_of_v<T, U>
    std::unique_ptr<std::remove_reference_t<T>, PmrDeleter> clone(this T& self, std::pmr::memory_resource* mr = nullptr) {
        if (!mr) mr = self.mem_res;
        std::unique_ptr<U, PmrDeleter> r = self.do_clone(mr);
        // instantiate a polymorphic allocator using the memory resource for the type of T

    }

protected:

    virtual std::unique_ptr<T, PmrDeleter> do_clone(std::pmr::memory_resource* mr = nullptr) {
        using D = T; // TODO: Ask Bob if we still need this?
        auto const& self = static_cast<D const&>(*this);
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
