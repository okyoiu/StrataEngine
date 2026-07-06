#pragma once
#include "src/core/types.hpp"
#include "src/ecs/entity.hpp"
#include "src/ecs/registry.hpp"

namespace tessera::ecs {

// Iterates entities that own every component in <First, Rest...>.
// Iteration walks First's dense array, so list the rarest component first.
template <typename First, typename... Rest>
class View {
public:
    explicit View(Registry& registry) noexcept : registry_{registry} {}

    template <typename Fn>
    void each(Fn&& fn) {
        auto& first_pool = registry_.pool<First>();
        auto entities = first_pool.view_entities();
        auto data = first_pool.view_data();

        for (usize i = 0; i < entities.size(); ++i) {
            const EntityID entity = entities[i];
            if ((registry_.has<Rest>(entity) && ...)) {
                fn(entity, data[i], registry_.get<Rest>(entity)...);
            }
        }
    }

private:
    Registry& registry_;
};

template <typename... Components>
[[nodiscard]] auto make_view(Registry& registry) -> View<Components...> {
    return View<Components...>{registry};
}
}
