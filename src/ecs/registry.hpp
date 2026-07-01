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