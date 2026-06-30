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

    /**
     * @brief generates a stricly increasing integer ID
     * bc the counter is static, the memory is kept and is kept * throughout the function 
     */
    inline u32 next_component_id() noexcept {
        static u32 counter = 0;
        return counter++;
    }

    /* @brief maps a c++ type to a unique integer ID */
    template <typename T>
    inline u32 component_id() noexcept {
        // evaluate once per type, thread-safe
        static const u32 id = next_component_id
    }
}