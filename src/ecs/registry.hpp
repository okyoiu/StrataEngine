#pragma once
/*
    CENTRAL HUB THAT MANAGES LIFECYCLES OF YOUR ENTITIES
*/

#include "src/core/types.hpp"
#include "src/core/assert.hpp"
#include "src/ecs/entity.hpp"
#include "src/ecs/component_pool.hpp"

#include <vector>
#include <memory>
#include <utility>

namespace tessera::ecs {

namespace detail {
    /**
     * @brief generates a strictly increasing integer ID
     * bc the counter is static, the memory is kept throughout the program
     */
    inline u32 next_component_id() noexcept {
        static u32 counter = 0;
        return counter++;
    }

    /**
     * @brief maps a c++ type to a unique integer ID, evaluated once per type
     */
    template <typename T>
    inline u32 component_id() noexcept {
        static const u32 id = next_component_id();
        return id;
    }
}

// REGISTRY
class Registry {
    public:
        // we can have exactly 1,048,576 entities alive at once
        // '1u' means unsigned integer 1. we bitshift it left by 20 positions
        static constexpr usize MAX_ENTITIES = 1u << 20u;

        explicit Registry(usize expected_entities = 4096)
            : m_next_index {0}{
                // this prevents vectors from constantly resizing to save on speed
                m_versions.reserve(expected_entities);
                m_free_indices.reserve(256);
                m_pools.reserve(32); // assuming we dont have more than 32 diff. component types
            }

            ~Registry() = default;

            // non-copyable
            Registry(const Registry&) = delete;
            Registry& operator=(const Registry&) = delete;

            // movable
            Registry(Registry&&) noexcept = default;
            Registry& operator=(Registry&&) noexcept = default;

            // creating a new entity. so we check an index and recycle from list.
            // otherwise we increment
            [[nodiscard]] EntityID create() {
                u32 index;
                if (!m_free_indices.empty()) {
                    index = m_free_indices.back();
                    m_free_indices.pop_back();
                } else {
                    TESSERA_ASSERT(m_next_index < MAX_ENTITIES, "Entity index overflow");
                    index = m_next_index++;
                    m_versions.push_back(0);
                }
                return make_entity_id(index, m_versions[index]);
            }

            // checks whether a given handle still refers to a live entity
            // (its version must match the current version stored for that index)
            [[nodiscard]] auto valid(EntityID entity) const noexcept -> bool {
                const u32 index = entity_index(entity);
                return index < m_versions.size() && m_versions[index] == entity_version(entity);
            }

            // detroying an entity, so remove all components and recycle index
            void destroy(EntityID entity) {
                // if check condition for this
                TESSERA_ASSERT(valid(entity), "Attempt to destroy an invalid entity");

                // removing every pool that holds entity's component
                for (auto& ptr : m_pools) {
                    if (ptr && ptr->has(entity)) {
                        ptr->erase(entity);
                    }
                }
                        // Bump the version so any surviving handles become stale
                const u32 index = entity_index(entity);
                ++m_versions[index];
                m_free_indices.push_back(index);
            }

            // -------------------------------------------------------------------
            // Generic component API
            // -------------------------------------------------------------------

            /// Fetches (creating on first use) the typed pool backing component T.
            template <typename T>
            [[nodiscard]] auto pool() -> ComponentPool<T>& {
                const u32 id = detail::component_id<T>();
                if (id >= m_pools.size()) {
                    m_pools.resize(id + 1);
                }
                if (!m_pools[id]) {
                    m_pools[id] = make_component_pool<T>(id);
                }
                return static_cast<ComponentPool<T>&>(*m_pools[id]);
            }

            template <typename T, typename... Args>
                requires std::is_constructible_v<T, Args...>
            auto add(EntityID entity, Args&&... args) -> T& {
                return pool<T>().emplace(entity, std::forward<Args>(args)...);
            }

            template <typename T>
            [[nodiscard]] auto get(EntityID entity) -> T& {
                return pool<T>().get(entity);
            }

            template <typename T>
            [[nodiscard]] auto get(EntityID entity) const -> const T& {
                return const_cast<Registry*>(this)->pool<T>().get(entity);
            }

            template <typename T>
            [[nodiscard]] auto try_get(EntityID entity) -> T* {
                return pool<T>().try_get(entity);
            }

            template <typename T>
            [[nodiscard]] auto has(EntityID entity) -> bool {
                return pool<T>().has(entity);
            }

            template <typename T>
            void remove(EntityID entity) {
                pool<T>().erase(entity);
            }

    private:
        std::vector<u32> m_versions;

        // A stack of entity indices that were destroyed and are ready to be reused.
        std::vector<u32> m_free_indices;

        // A simple counter. If m_free_indices is empty, we grab the next fresh index from here.
        u32 m_next_index;

        // This is the master list of all Component Pools.
        // We use unique_ptr so the Registry strictly owns the memory.
        // IComponentPoolBase allows us to store different types (Position, Velocity) in one array.
        std::vector<std::unique_ptr<IComponentPoolBase>> m_pools;
};
}
