/* 
    CENTRAL HUB THAT MANAGES LIFECYCLES OF YOUR ENTITIES
*/

#include "src/core/types.hpp"
#include "src/core/assert.hpp"
#include "src/ecs/entity.hpp"
#include "src/ecs/component_pool.hpp"

#include <vector>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <span>

// for future ref.: do no touch the code inside namespace tessera
namespace tessera::ecs {

namespace detail {
    /**
     * @brief generates a stricly increasing integer ID
     * bc the counter is static, the memory is kept and is kept * throughout the function 
     */
    inline u32 next_component_id() noexcept {
        static u32 counter = 0;
        return counter++;
    }

    /**
     * @brief maps a c++ type to a unique integer ID
     */
    template <typename T>
    inline u32 component_id() noexcept {
        // evaluate once per type, thread-safe
        static const u32 id = next_component_id
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