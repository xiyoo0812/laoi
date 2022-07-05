#ifndef __AOI_H_——
#define __AOI_H_——
#include "lua_kit.h"

#include <set>
#include <map>
#include <vector>
#include <algorithm>

using namespace std;

namespace laoi {

enum class aoi_type : int
{
    watcher     = 0,    //观察者
    marker      = 1,    //被观察者
};

class aoi_obj
{
public:
    uint64_t eid;
    size_t grid_x;
    size_t grid_y;
    aoi_type type;

    void set_grid(size_t x, size_t y) {
        grid_x = x;
        grid_y = y;
    }

    void enter(aoi_obj* obj){

    }

    void leave(aoi_obj* obj){
        
    }

    void add_watcher(aoi_obj* obj){
        
    }
    void remove_watcher(aoi_obj* obj){
        
    }

    aoi_obj(uint64_t id, aoi_type typ) : eid(id), type(typ) {}
};

typedef set<aoi_obj*> object_set;
typedef vector<object_set> object_axis;
typedef vector<object_axis> object_grids;

class aoi
{ 
public:
    aoi(lua_State* L, size_t w, size_t h, size_t glen, size_t aoi_len, bool offset) {	
        mL = L;
        grid_len = glen;
        aoi_num = aoi_len;
        xgrid_num = w / glen;
        ygrid_num = h / glen;
        grids.resize(ygrid_num + 1);
        for (unsigned int i = 0; i <= ygrid_num; i++) {
            grids[i].resize(xgrid_num + 1);
        }
        if (offset) {
            offset_w = w / 2;
            offset_h = h / 2;
        }
    }
    ~aoi(){};

    void copy(object_set& o1, object_set& o2) {
        for(auto o : o2) {
            o1.insert(o);
        }
    }
    size_t convert_x(size_t inoout_x){
        return (inoout_x + offset_w) / grid_len;
    }

    size_t convert_y(size_t inoout_y){
        return (inoout_y + offset_h) / grid_len;
    }

    void get_rect_objects(object_set& objs, size_t minX, size_t maxX, size_t minY, size_t maxY) {
        for(int y = minY; y <= maxY; y++) {
            for(int x = minX; x <= maxX; x++) {
                copy(objs, grids[y][x]);
            }
        }
    }

    void get_objects(object_set& objs, size_t nxgrid, size_t nygrid) {
        size_t minX = max<size_t>(0, nxgrid - aoi_num);
        size_t maxX = min(xgrid_num + 1, nxgrid + aoi_num);
        size_t minY = max<size_t>(0, nygrid - aoi_num);
        size_t maxY = min(ygrid_num + 1, nygrid + aoi_num);
        get_rect_objects(objs, minX, maxX, minY, maxY);
    }

    void get_around_objects(object_set& enters, object_set& leaves, size_t oxgrid, size_t oygrid, size_t nxgrid, size_t nygrid) {
        size_t offsetX = nxgrid - oxgrid;
        size_t offsetY = nygrid - oygrid;
        if (offsetX < 0) {
            get_rect_objects(enters, nxgrid - aoi_num, oxgrid - aoi_num, max<size_t>(0, nygrid - aoi_num), min(ygrid_num + 1, nygrid + aoi_num));
            get_rect_objects(leaves, nxgrid + aoi_num, oxgrid + aoi_num, max<size_t>(0, oygrid - aoi_num), min(ygrid_num + 1, oygrid + aoi_num));
        }
        else if (offsetX > 0){
            get_rect_objects(enters, oxgrid + aoi_num, nxgrid + aoi_num, max<size_t>(0, nygrid - aoi_num), min(ygrid_num + 1, nygrid + aoi_num));
            get_rect_objects(leaves, oxgrid - aoi_num, nxgrid - aoi_num, max<size_t>(0, oygrid - aoi_num), min(ygrid_num + 1, oygrid + aoi_num));
        }
        if (offsetY < 0) {
            get_rect_objects(enters, max<size_t>(0, nxgrid - aoi_num), min(xgrid_num + 1, nxgrid + aoi_num), nygrid - aoi_num, oygrid - aoi_num);
            get_rect_objects(leaves, max<size_t>(0, oxgrid - aoi_num), min(xgrid_num + 1, oxgrid + aoi_num), nygrid + aoi_num, oygrid + aoi_num);
        }
        else if (offsetY > 0){
            get_rect_objects(enters, max<size_t>(0, nxgrid - aoi_num), min(xgrid_num + 1, nxgrid + aoi_num), oygrid + aoi_num, nygrid + aoi_num);
            get_rect_objects(leaves, max<size_t>(0, oxgrid - aoi_num), min(xgrid_num + 1, oxgrid + aoi_num), oygrid - aoi_num, nygrid - aoi_num);
        }
    }

    bool attach(aoi_obj* obj, size_t x, size_t y){
        int nxgrid_num = convert_x(x);
        int nygrid_num = convert_y(y);
        if ((nxgrid_num < 0) || (nxgrid_num > xgrid_num) || (nygrid_num < 0) || (nygrid_num > ygrid_num)) {
            return false;
        }
        //查询节点
        object_set objs;
        get_objects(objs, nxgrid_num, nygrid_num);
        //消息通知
        luakit::kit_state kit_state(mL);
        for (auto cobj : objs) {
            if (cobj->type == aoi_type::watcher) {
                kit_state.object_call(this, "on_enter", nullptr, std::tie(), cobj->eid, obj->eid);
            }
            if (obj->type == aoi_type::watcher) {
                kit_state.object_call(this, "on_enter", nullptr, std::tie(), obj->eid, cobj->eid);
            }
        }
        //放入格子
        obj->set_grid(nxgrid_num, nygrid_num);
        grids[nygrid_num][nxgrid_num].insert(obj);
        return true;
    }

    bool detach(aoi_obj* obj) {
        grids[obj->grid_y][obj->grid_x].erase(obj);
        return true;
    }

    bool move(aoi_obj* obj, size_t x, size_t y) {
        int nxgrid_num = convert_x(x);
        int nygrid_num = convert_y(y);
        if ((nxgrid_num < 0) || (nxgrid_num > xgrid_num) || (nygrid_num < 0) || (nygrid_num > ygrid_num)) {
            return false;
        }
        if (nxgrid_num == obj->grid_x && nygrid_num == obj->grid_y){
            return false;
        }
        grids[obj->grid_y][obj->grid_x].erase(obj);
        //消息通知
        object_set enters, leaves;
        get_around_objects(enters, leaves, obj->grid_x,  obj->grid_y, nxgrid_num, nygrid_num);
        //进入视野
        luakit::kit_state kit_state(mL);
        for (auto cobj : enters) {
            if (cobj->type == aoi_type::watcher) {
                kit_state.object_call(this, "on_enter", nullptr, std::tie(), cobj->eid, obj->eid);
            }
            if (obj->type == aoi_type::watcher) {
                kit_state.object_call(this, "on_enter", nullptr, std::tie(), obj->eid, cobj->eid);
            }
        }
        //退出事视野
        for (auto cobj : leaves) {
           if (cobj->type == aoi_type::watcher) {
                kit_state.object_call(this, "on_leave", nullptr, std::tie(), cobj->eid, obj->eid);
            }
            if (obj->type == aoi_type::watcher) {
                kit_state.object_call(this, "on_leave", nullptr, std::tie(), obj->eid, cobj->eid);
            } 
        }
        obj->set_grid(nxgrid_num, nygrid_num);
        grids[nygrid_num][nxgrid_num].insert(obj);
        return true;
    }

private:
    lua_State* mL;
    size_t offset_w = 0;    //x轴坐标偏移
    size_t offset_h = 0;    //y轴坐标偏移
    size_t grid_len = 50;   //格子长度
    size_t xgrid_num = 1;   //x轴的格子数
    size_t ygrid_num = 1;   //y轴的格子数
    size_t aoi_num = 1;     //视野格子数
    object_grids grids;     //二维数组保存将地图xy轴切割后的格子
};

}

#endif
