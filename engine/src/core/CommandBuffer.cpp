#include "core/CommandBuffer.hpp"

namespace ArgusEngine::Engine {

    // TODO: выкинуть std::array к чертям
    template<std::size_t... Is>
    constexpr std::array<CommandBuffer::ApplyFunc, CommandBuffer::TABLE_SIZE>
    init_jump_table(std::index_sequence<Is...>) {
        std::array<CommandBuffer::ApplyFunc, CommandBuffer::TABLE_SIZE> table = {
            &CommandBuffer::apply_thunk<std::tuple_element_t<Is, AllComponents>>...
        };

        table[CommandBuffer::IDX_DESTROY]   = &CommandBuffer::sys_destroy_thunk;
        table[CommandBuffer::IDX_TILE_REG]  = &CommandBuffer::sys_tile_reg_thunk;
        table[CommandBuffer::IDX_TILE_MOVE] = &CommandBuffer::sys_tile_move_thunk;

        return table;
    }

    const std::array<CommandBuffer::ApplyFunc, CommandBuffer::TABLE_SIZE>
    CommandBuffer::jump_table = init_jump_table(std::make_index_sequence<std::tuple_size_v<AllComponents>>{});

    void CommandBuffer::submit() {
        auto& buffer = cmdBuffers[active];
        size_t total_size = buffer.size();
        if (total_size == 0) return;

        uint8_t* ptr = buffer.data();
        uint8_t* end = ptr + total_size;

        while (ptr < end) {
            if (ptr + sizeof(CommandHeader) > end) break;

            __builtin_prefetch(ptr + 128, 0, 3);

            CommandHeader* h = reinterpret_cast<CommandHeader*>(ptr);
            auto func = jump_table[h->p_idx];
            func(reg, tilespares, h->entity, h->op, ptr + sizeof(CommandHeader));

            ptr += (sizeof(CommandHeader) + h->size + 15) & ~15;
        }

        buffer.clear();
        active ^= 1;
    }

} // namespace ArgusEngine::Engine