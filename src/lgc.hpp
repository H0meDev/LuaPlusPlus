#pragma once
/*
** $Id: lgc.h,v 2.91 2015/12/21 13:02:14 roberto Exp $
** Garbage Collector
** See Copyright Notice in lua.h
*/

#include <lobject.hpp>
#include <lstate.hpp>
#include <type_traits>
#include <lutils.hpp>

/*
** Collectable objects may have one of three colors: white, which
** means the object is not marked; gray, which means the
** object is marked, but its references may be not marked; and
** black, which means that the object and all its references are marked.
** The main invariant of the garbage collector, while marking objects,
** is that a black object can never point to a white one. Moreover,
** any gray object must be in a "gray list" (gray, grayagain, weak,
** allweak, ephemeron) so that it can be visited again before finishing
** the collection cycle. These lists have no meaning when the invariant
** is not being enforced (e.g., sweep phase).
*/

/* how much to allocate before next GC step */
#if !defined(GCSTEPSIZE)
/* ~100 small strings */
#define GCSTEPSIZE      (cast_int(100 * sizeof(TString)))
#endif

/*
** Possible states of the Garbage Collector
*/
#define GCSpropagate    0
#define GCSatomic       1
#define GCSswpallgc     2
#define GCSswpfinobj    3
#define GCSswptobefnz   4
#define GCSswpend       5
#define GCScallfin      6
#define GCSpause        7

#define issweepphase(g)  \
  (GCSswpallgc <= (g)->gcstate && (g)->gcstate <= GCSswpend)

/*
** macro to tell when main invariant (white objects cannot point to black
** ones) must be kept. During a collection, the sweep
** phase may break the invariant, as objects turned white may point to
** still-black objects. The invariant is restored when sweep ends and
** all objects are white again.
*/

#define keepinvariant(g)        ((g)->gcstate <= GCSatomic)

/*
** some useful bit tricks
*/
#define resetbits(x, m)          ((x) &= cast(uint8_t, ~(m)))
#define setbits(x, m)            ((x) |= (m))
#define testbits(x, m)           ((x) & (m))
#define bitmask(b)              (1<<(b))
#define bit2mask(b1, b2)         (bitmask(b1) | bitmask(b2))
#define l_setbit(x, b)           setbits(x, bitmask(b))
#define resetbit(x, b)           resetbits(x, bitmask(b))
#define testbit(x, b)            testbits(x, bitmask(b))

/* Layout for bit use in 'marked' field: */
#define WHITE0BIT       0  /* object is white (type 0) */
#define WHITE1BIT       1  /* object is white (type 1) */
#define BLACKBIT        2  /* object is black */
#define FINALIZEDBIT    3  /* object has been marked for finalization */
/* bit 7 is currently used by tests (luaL_checkmemory) */

#define WHITEBITS       bit2mask(WHITE0BIT, WHITE1BIT)

#define iswhite(x)      testbits((x)->marked, WHITEBITS)
#define isblack(x)      testbit((x)->marked, BLACKBIT)
#define isgray(x)  /* neither white nor black */  \
  (!testbits((x)->marked, WHITEBITS | bitmask(BLACKBIT)))

#define tofinalize(x)   testbit((x)->marked, FINALIZEDBIT)

#define otherwhite(g)   ((g)->currentwhite ^ WHITEBITS)
#define isdeadm(ow, m)   (!(((m) ^ WHITEBITS) & (ow)))
#define isdead(g, v)     isdeadm(otherwhite(g), (v)->marked)

#define changewhite(x)  ((x)->marked ^= WHITEBITS)
#define gray2black(x)   l_setbit((x)->marked, BLACKBIT)

#define luaC_white(g)   cast(uint8_t, (g)->currentwhite & WHITEBITS)

/*
** Does one step of collection when debt becomes positive. 'pre'/'pos'
** allows some adjustments to be done only when needed. macro
** 'condchangemem' is used only for heavy tests (forcing a full
** GC cycle on every opportunity)
*/
#define luaC_condGC(L, pre, pos) \
  { if (L->globalState->GCdebt > 0) { pre; luaC_step(L); pos;}; \
    condchangemem(L, pre, pos); }

/* more often than not, 'pre'/'pos' are empty */
#define luaC_checkGC(L)         luaC_condGC(L, (void)0, (void)0)

#define luaC_barrier(L, p, v) (  \
    (iscollectable(v) && isblack(p) && iswhite(gcvalue(v))) ?  \
    luaC_barrier_(L, obj2gco(p), gcvalue(v)) : cast_void(0))

#define luaC_barrierback(L, p, v) (  \
    (iscollectable(v) && isblack(p) && iswhite(gcvalue(v))) ? \
    luaC_barrierback_(L, p) : cast_void(0))

#define luaC_objbarrier(L, p, o) (  \
    (isblack(p) && iswhite(o)) ? \
    luaC_barrier_(L, obj2gco(p), obj2gco(o)) : cast_void(0))

#define luaC_upvalbarrier(L, uv) ( \
    (iscollectable((uv)->v) && !upisopen(uv)) ? \
    luaC_upvalbarrier_(L, uv) : cast_void(0))

class LGCFactory
{
public:
  LGCFactory() = delete;

  // Create a new collectable object (with given type and size) and link it to 'allgc' list.
  template<class T>
  static T* luaC_newobj(lua_State* L, LuaType type, size_t sz)
  {
    static_assert(std::is_base_of<GCObject, T>::value);
    static_assert(std::is_same<decltype(LGCFactory::luaC_free(std::declval<lua_State*>(), std::declval<T*>())), void>::value);

    T* object = static_cast<T*>(LMem<void>::luaM_newobject(L, LuaType::DataType(novariant(type)), sz));
    global_State* g = L->globalState;
    object->marked = luaC_white(g);
    object->type = type;
    object->next = g->allgc;
    g->allgc = object;
    return object;
  }

  template<class T>
  static void luaC_freeobj(lua_State* L, T* obj)
  {
    Lua::ScopedValueSetter<lua_State*> stateGuard(LGCFactory::active_state, L);
    LGCFactory::luaC_free(L, obj);
  }

  static lua_State* getActiveState() { return LGCFactory::active_state; }

private:

  static thread_local lua_State* active_state;
  static void luaC_free(lua_State* L, Proto* funcion);
  static void luaC_free(lua_State* L, LClosure* closure);
  static void luaC_free(lua_State* L, CClosure* closure);
  static void luaC_free(lua_State* L, Table* table);
  static void luaC_free(lua_State* L, lua_State* L1);
  static void luaC_free(lua_State* L, Udata* udata);
  static void luaC_free(lua_State* L, TString* string);
};

LUAI_FUNC void luaC_fix(lua_State *L, GCObject *o);
LUAI_FUNC void luaC_freeallobjects(lua_State *L);
LUAI_FUNC void luaC_step(lua_State *L);
LUAI_FUNC void luaC_runtilstate(lua_State *L, int statesmask);
LUAI_FUNC void luaC_fullgc(lua_State *L, int isemergency);
LUAI_FUNC void luaC_barrier_(lua_State *L, GCObject *o, GCObject *v);
LUAI_FUNC void luaC_barrierback_(lua_State *L, Table *o);
LUAI_FUNC void luaC_upvalbarrier_(lua_State *L, UpVal *uv);
LUAI_FUNC void luaC_checkfinalizer(lua_State *L, GCObject *o, Table *mt);
LUAI_FUNC void luaC_upvdeccount(lua_State *L, UpVal *uv);
