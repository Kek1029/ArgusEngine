#ifndef ARGUSENGINE_TYPES_HPP
#define ARGUSENGINE_TYPES_HPP

#include <tuple>
#include <glm/vec3.hpp>

#include "utils/Utils.hpp"
#include "memory/Registry.hpp"
#include "utils/TileFixedPoint.hpp"

namespace ArgusEngine {
    #define CHUNK_SIZE 64
    using fixed_coord = Math::Fixed32;
    using tile_coord  = int32_t;

    struct alignas(8) Tile {
        tile_coord x, y;
    };

    MAKE_COMPONENT(Transform,
        Math::QuatFixed,         // 16б (Поворот)
        Math::Vector3Fixed<24>,  // 12б (Позиция)
        uint32_t,                // 4б  (ParentID)

        Math::Vector3Fixed<24>,  // 12б (Масштаб)
        uint32_t,                // 4б  (Flags)

        tile_coord, tile_coord,   // 8б  (tx, ty)
        uint32_t,                // 4б  (LayerMask)
        uint32_t                 // 4б
    )

    MAKE_COMPONENT(PhysicsBody,
        Math::Vector3Fixed<24> , // Линейная скорость
        Math::Vector3Fixed<24> , // Угловая скорость
        Math::Fixed32          , // 1/m (0 для статики)
        uint16_t               , // Ссылка на материал
        uint16_t                 // Состояние
    )

    MAKE_COMPONENT(MeshLink,
        uint32_t ,               // Ссылка на модель
        uint16_t ,               // Текущий LOD
        uint16_t ,               // Флаги
        uint32_t                 // Для инстасинга
    )

    using AllComponents = std::tuple<
        Component_Transform,
        Component_PhysicsBody,
        Component_MeshLink
    >;

    using WorldRegistry = Memory::Registry<
        Component_Transform,
        Component_PhysicsBody,
        Component_MeshLink
    >;

} // ArgusEngine

#endif //ARGUSENGINE_TYPES_HPP