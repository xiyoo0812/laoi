
#define LUA_LIB

#include "aoi.h"

namespace laoi {
    static aoi_obj* create_object(uint64_t id, size_t typ) {
        return new aoi_obj(id, (aoi_type)typ);
    }

    static aoi* create_aoi(lua_State* L, size_t w, size_t h, size_t grid, size_t aoi_len, bool offset) {
        return new aoi(L, w, h, grid, aoi_len, offset);
    }

    luakit::lua_table open_laoi(lua_State* L) {
        luakit::kit_state kit_state(L);
        auto llaoi = kit_state.new_table();
        llaoi.set_function("create_aoi", create_aoi);
        llaoi.set_function("create_object", create_object);
        kit_state.new_class<aoi_obj>();
        kit_state.new_class<aoi>(
            "move", &aoi::move,
            "attach", &aoi::attach,
            "detach", &aoi::detach
            );
        return llaoi;
    }
}

extern "C" {
    LUALIB_API int luaopen_laoi(lua_State* L) {
        auto llaoi = laoi::open_laoi(L);
        return llaoi.push_stack();
    }
}
