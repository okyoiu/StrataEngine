#pragma once
#include "src/core/types.hpp"
#include "src/core/assert.hpp"
#include "src/core/platform.hpp"
#include "src/ecs/sparse_set.hpp"

#include <memory>
#include <span>
#include <type_traits>

namespace tessera::ecs {

// =============================================================================
// IComponentPoolBase - Type-erased interface for Registry storage
// =============================================================================

class IComponentPoolBase {
public:
    virtual ~IComponentPoolBase() = default;

    // Non-copyable, movable only through unique_ptr
    IComponentPoolBase(const IComponentPoolBase&) = delete;
    IComponentPoolBase& operator=(const IComponentPoolBase&) = delete;

    // Entity lifecycle operations (type-erased)
    virtual void erase(EntityID entity) = 0;
    virtual auto has(EntityID entity) const -> bool = 0;
    virtual auto size() const -> usize = 0;
    virtual auto capacity() const -> usize = 0;

    // Type metadata
    [[nodiscard]] auto type_id() const noexcept -> u32 { return m_type_id; }
    [[nodiscard]] auto type_size() const noexcept -> usize { return m_type_size; }
    [[nodiscard]] auto type_align() const noexcept -> usize { return m_type_align; }

protected:
    explicit IComponentPoolBase(u32 type_id, usize type_size, usize type_align) noexcept
        : m_type_id{type_id}
        , m_type_size{type_size}
        , m_type_align{type_align}
    {}

    // Allow move construction for derived classes
    IComponentPoolBase(IComponentPoolBase&&) noexcept = default;
    IComponentPoolBase& operator=(IComponentPoolBase&&) noexcept = default;

private:
    u32   m_type_id;
    usize m_type_size;
    usize m_type_align;
};

// =============================================================================
// ComponentPool<T> - Typed owner wrapper around SparseSet<T>
// =============================================================================

template <typename T>
    requires std::is_move_constructible_v<T>
class ComponentPool final : public IComponentPoolBase {
public:
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;

    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    explicit ComponentPool(u32 type_id, usize initial_capacity = 1024)
        : IComponentPoolBase{type_id, sizeof(T), alignof(T)}
        , m_storage{initial_capacity}
    {}

    ~ComponentPool() override = default;

    // Move-only semantics (SparseSet is move-only)
    ComponentPool(ComponentPool&&) noexcept = default;
    ComponentPool& operator=(ComponentPool&&) noexcept = default;

    // -------------------------------------------------------------------------
    // Typed Component Operations
    // -------------------------------------------------------------------------

    /// Construct component in-place for entity. Returns reference to new component.
    /// Asserts if entity already has this component.
    template <typename... Args>
        requires std::is_constructible_v<T, Args...>
    auto emplace(EntityID entity, Args&&... args) -> reference {
        TESSERA_ASSERT(!m_storage.has(entity), 
            "Entity already has component of this type");
        return m_storage.emplace(entity, std::forward<Args>(args)...);
    }

    /// Assign or replace component for entity. Returns reference.
    /// If entity already has component, destructs old and constructs new.
    template <typename... Args>
        requires std::is_constructible_v<T, Args...>
    auto emplace_or_replace(EntityID entity, Args&&... args) -> reference {
        if (m_storage.has(entity)) {
            // In-place replacement: destruct old, construct new
            T& existing = m_storage.get(entity);
            std::destroy_at(&existing);
            std::construct_at(&existing, std::forward<Args>(args)...);
            return existing;
        }
        return m_storage.emplace(entity, std::forward<Args>(args)...);
    }

    /// Get mutable reference to component. Asserts if entity doesn't have it.
    [[nodiscard]] auto get(EntityID entity) -> reference {
        TESSERA_ASSERT(m_storage.has(entity), 
            "Entity does not have component of this type");
        return m_storage.get(entity);
    }

    /// Get const reference to component. Asserts if entity doesn't have it.
    [[nodiscard]] auto get(EntityID entity) const -> const_reference {
        TESSERA_ASSERT(m_storage.has(entity), 
            "Entity does not have component of this type");
        return m_storage.get(entity);
    }

    /// Try to get component. Returns nullptr if entity doesn't have it.
    [[nodiscard]] auto try_get(EntityID entity) -> pointer {
        return m_storage.has(entity) ? &m_storage.get(entity) : nullptr;
    }

    [[nodiscard]] auto try_get(EntityID entity) const -> const_pointer {
        return m_storage.has(entity) ? &m_storage.get(entity) : nullptr;
    }

    // -------------------------------------------------------------------------
    // IComponentPoolBase Interface Implementation
    // -------------------------------------------------------------------------

    void erase(EntityID entity) override {
        TESSERA_ASSERT(m_storage.has(entity), 
            "Cannot erase: entity does not have component");
        m_storage.remove(entity);
    }

    [[nodiscard]] auto has(EntityID entity) const -> bool override {
        return m_storage.has(entity);
    }

    [[nodiscard]] auto size() const -> usize override {
        return m_storage.size();
    }

    [[nodiscard]] auto capacity() const -> usize override {
        return m_storage.capacity();
    }

    // -------------------------------------------------------------------------
    // Dense Array Access (for cache-friendly iteration)
    // -------------------------------------------------------------------------

    /// Returns contiguous span of all components (dense array).
    /// Order corresponds to view_entities() order.
    [[nodiscard]] auto view_data() noexcept -> std::span<T> {
        return m_storage.data();
    }

    [[nodiscard]] auto view_data() const noexcept -> std::span<const T> {
        return m_storage.data();
    }

    /// Returns contiguous span of entity IDs that own components.
    /// Order corresponds to view_data() order.
    [[nodiscard]] auto view_entities() const noexcept -> std::span<const EntityID> {
        return m_storage.entities();
    }

    /// Direct access to underlying sparse set (for advanced iteration patterns)
    [[nodiscard]] auto sparse_set() noexcept -> SparseSet<T>& {
        return m_storage;
    }

    [[nodiscard]] auto sparse_set() const noexcept -> const SparseSet<T>& {
        return m_storage;
    }

private:
    SparseSet<T> m_storage;
};

// =============================================================================
// Factory function for type-erased pool creation
// =============================================================================

/// Creates a type-erased component pool. Used by Registry during component registration.
template <typename T>
[[nodiscard]] auto make_component_pool(u32 type_id, usize initial_capacity = 1024)
    -> std::unique_ptr<IComponentPoolBase>
{
    return std::make_unique<ComponentPool<T>>(type_id, initial_capacity);
}