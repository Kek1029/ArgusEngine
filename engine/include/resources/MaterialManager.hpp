//
// Created by etders on 05.04.2026.
//

#ifndef ARGUSENGINE_MATERIALMANAGER_HPP
#define ARGUSENGINE_MATERIALMANAGER_HPP

#include "utils/TileFixedPoint.hpp"
#include "memory/Map.hpp"

namespace ArgusEngine {
namespace Resources {
    // TODO: доделать этот ебаный сопромат, хотя, сразу надо доделать менеджер моделей
    struct alignas(16) PhysicsMaterial {

        Math::Fixed32 youngModulus;      // Модуль Юнга
        Math::Fixed32 yieldStrength;     // Предел текучести
        Math::Fixed32 fractureToughness; // Трещиностойкость
        Math::Fixed32 density;           // Плотность

        Math::Fixed32 poissonRatio;      // Коэффициент Пуассона
        Math::Fixed32 friction;          // Трение
        Math::Fixed32 restitution;       // Прыгучесть
        Math::Fixed32 viscosity;         // Вязкость

        // --- Термодинамика ---
        uint32_t thermalCond;
        uint32_t thermalExpansion;

        uint32_t flags;
        uint32_t pad;
    };

    struct alignas(16) VisualMaterial {
        uint32_t albedoID;
        uint32_t normalID;
        uint32_t metalRoughID;
        uint32_t emissionID;

        Math::Fixed32 roughnessBase;
        Math::Fixed32 metallicBase;
        Math::Fixed32 emissionPower;

        uint32_t shaderFlags;
    };

    class MaterialManager {
        Memory::Map<uint16_t, PhysicsMaterial> physicsStorage;
        Memory::Map<uint16_t, VisualMaterial> visualStorage;
        uint16_t nextId = 1;

    public:
        uint32_t create(PhysicsMaterial phys, VisualMaterial vis) {
            uint32_t id = nextId++;
            physicsStorage.insert(id, std::move(phys));
            visualStorage.insert(id, std::move(vis));
            return id;
        }

        PhysicsMaterial* getPhys(uint32_t id) { return physicsStorage.get(id); }
        VisualMaterial* getVis(uint32_t id) { return visualStorage.get(id); }
    };

} // Resources
} // ArgusEngine

#endif //ARGUSENGINE_MATERIALMANAGER_HPP
