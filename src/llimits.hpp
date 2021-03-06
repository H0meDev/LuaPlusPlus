#pragma once
/*
** $Id: llimits.h,v 1.141 2015/11/19 19:16:22 roberto Exp $
** Limits, basic types, and some other 'installation-dependent' definitions
** See Copyright Notice in lua.h
*/

#include <climits>
#include <cstddef>
#include <lua.hpp>
#include <limits>

/*
** 'lu_mem' and 'l_mem' are unsigned/signed integers big enough to count
** the total memory used by Lua (in bytes). Usually, 'size_t' and
** 'ptrdiff_t' should work, but we use 'long' for 16-bit machines.
*/
#if defined(LUAI_MEM)           /* external definitions? */
using lu_mem = LUAI_UMEM;
using l_mem = LUAI_MEM;
#else
using lu_mem = size_t;
using l_mem = ptrdiff_t;
#endif

/* maximum value for size_t */
static constexpr size_t MAX_SIZET = std::numeric_limits<size_t>::max();

// maximum size visible for Lua (must be representable in a lua_Integer
static constexpr size_t MAX_SIZE = sizeof(size_t) < sizeof(lua_Integer) ? MAX_SIZET : static_cast<size_t>(LUA_MAXINTEGER);

static constexpr lu_mem MAX_LUMEM = std::numeric_limits<lu_mem>::max();
static constexpr l_mem MAX_LMEM = std::numeric_limits<l_mem>::max();

// maximum value of an int32_t
static constexpr int32_t MAX_INT = std::numeric_limits<int32_t>::max();

/*
** conversion of pointer to uint32_t:
** this is for hashing only; there is no problem if the integer
** cannot hold the whole pointer value
*/
#define point2uint(p)   ((uint32_t)((size_t)(p) & std::numeric_limits<uint32_t>::max()))

/* type to ensure maximum alignment */
#if defined(LUAI_USER_ALIGNMENT_T)
using L_Umaxalign = LUAI_USER_ALIGNMENT_T;
#else
union L_Umaxalign
{
  lua_Number n;
  double u;
  void *s;
  lua_Integer i;
  long l;
};
#endif

#if !defined(lua_assert) && defined(DEBUG)
#define lua_assert assert
#endif

/* internal assertions for in-house debugging */
#if defined(lua_assert)
#define check_exp(c, e)          (lua_assert(c), (e))
/* to avoid problems with conditions too long */
#define lua_longassert(c)       ((c) ? (void)0 : lua_assert(0))
#else
#define lua_assert(c)           ((void)0)
#define check_exp(c, e)          (e)
#define lua_longassert(c)       ((void)0)
#endif

/*
** assertion for checking API calls
*/
#if !defined(luai_apicheck)
#define luai_apicheck(l, e)      lua_assert(e)
#endif

#define api_check(l, e, msg)      luai_apicheck(l, (e) && msg)

/* macro to avoid warnings about unused variables */
#if !defined(UNUSED)
#define UNUSED(x)       ((void)(x))
#endif

/* type casts (a macro highlights casts in the code) */
#define cast(t, exp)    ((t)(exp))

#define cast_void(i)    cast(void, (i))
#define cast_byte(i)    cast(uint8_t, (i))
#define cast_num(i)     cast(lua_Number, (i))
#define cast_int(i)     cast(int, (i))
#define cast_uchar(i)   cast(uint8_t, (i))

/* cast a signed lua_Integer to lua_Unsigned */
#if !defined(l_castS2U)
#define l_castS2U(i)    ((lua_Unsigned)(i))
#endif

/*
** cast a lua_Unsigned to a signed lua_Integer; this cast is
** not strict ISO C, but two-complement architectures should
** work fine.
*/
#if !defined(l_castU2S)
#define l_castU2S(i)    ((lua_Integer)(i))
#endif

/*
** maximum depth for nested C calls and syntactical nested non-terminals
** in a program. (Value must fit in an unsigned short int.)
*/
#if !defined(LUAI_MAXCCALLS)
#define LUAI_MAXCCALLS 200
#endif

/*
** type for virtual-machine instructions;
** must be an unsigned with (at least) 4 bytes (see details in lopcodes.h)
*/
using Instruction = uint32_t;

/*
** Maximum length for short strings, that is, strings that are
** internalized. (Cannot be smaller than reserved words or tags for
** metamethods, as these strings must be internalized;
** #("function") = 8, #("__newindex") = 10.)
*/
#if !defined(LUAI_MAXSHORTLEN)
#define LUAI_MAXSHORTLEN        40
#endif

/*
** Initial size for the string table (must be power of 2).
** The Lua core alone registers ~50 strings (reserved words +
** metaevent keys + a few others). Libraries would typically add
** a few dozens more.
*/
#if !defined(MINSTRTABSIZE)
#define MINSTRTABSIZE   128
#endif

/*
** Size of cache for strings in the API. 'N' is the number of
** sets (better be a prime) and "M" is the size of each set (M == 1
** makes a direct cache.)
*/
#if !defined(STRCACHE_N)
#define STRCACHE_N              53
#define STRCACHE_M              2
#endif

/* minimum size for string buffer */
#if !defined(LUA_MINBUFFER)
#define LUA_MINBUFFER   32
#endif

/*
** macros that are executed whenever program enters the Lua core
** ('lua_lock') and leaves the core ('lua_unlock')
*/
#if !defined(lua_lock)
#define lua_lock(L)     ((void) 0)
#define lua_unlock(L)   ((void) 0)
#endif

/*
** macro executed during Lua functions at points where the
** function can yield.
*/
#if !defined(luai_threadyield)
#define luai_threadyield(L)     {lua_unlock(L); lua_lock(L);}
#endif

/*
** these macros allow user-specific actions on threads when you defined
** LUAI_EXTRASPACE and need to do something extra when a thread is
** created/deleted/resumed/yielded.
*/
#if !defined(luai_userstateopen)
#define luai_userstateopen(L)           ((void)L)
#endif

#if !defined(luai_userstateclose)
#define luai_userstateclose(L)          ((void)L)
#endif

#if !defined(luai_userstatethread)
#define luai_userstatethread(L, L1)      ((void)L)
#endif

#if !defined(luai_userstatefree)
#define luai_userstatefree(L, L1)        ((void)L)
#endif

#if !defined(luai_userstateresume)
#define luai_userstateresume(L, n)       ((void)L)
#endif

#if !defined(luai_userstateyield)
#define luai_userstateyield(L, n)        ((void)L)
#endif

/*
** The luai_num* macros define the primitive operations over numbers.
*/

/* floor division (defined as 'floor(a/b)') */
#if !defined(luai_numidiv)
#define luai_numidiv(L, a, b)     ((void)L, l_floor(luai_numdiv(L, a, b)))
#endif

/* float division */
#if !defined(luai_numdiv)
#define luai_numdiv(L, a, b)      ((a)/(b))
#endif

/*
** modulo: defined as 'a - floor(a/b)*b'; this definition gives NaN when
** 'b' is huge, but the result should be 'a'. 'fmod' gives the result of
** 'a - trunc(a/b)*b', and therefore must be corrected when 'trunc(a/b)
** ~= floor(a/b)'. That happens when the division has a non-integer
** negative result, which is equivalent to the test below.
*/
#if !defined(luai_nummod)
#define luai_nummod(L, a, b, m)  \
  { (m) = l_mathop(fmod)(a, b); if ((m)*(b) < 0) (m) += (b); }
#endif

/* exponentiation */
#if !defined(luai_numpow)
#define luai_numpow(L, a, b)      ((void)L, l_mathop(pow)(a, b))
#endif

/* the others are quite standard operations */
#if !defined(luai_numadd)
#define luai_numadd(L, a, b)      ((a)+(b))
#define luai_numsub(L, a, b)      ((a)-(b))
#define luai_nummul(L, a, b)      ((a)*(b))
#define luai_numunm(L, a)        (-(a))
#define luai_numeq(a, b)         ((a) == (b))
#define luai_numlt(a, b)         ((a) < (b))
#define luai_numle(a, b)         ((a) <= (b))
#define luai_numisnan(a)        (!luai_numeq((a), (a)))
#endif

/*
** macro to control inclusion of some hard tests on stack reallocation
*/
#if !defined(HARDSTACKTESTS)
#define condmovestack(L, pre, pos)        ((void)0)
#else
/* realloc stack keeping its size */
#define condmovestack(L, pre, pos)  \
  { int sz_ = (L)->stacksize; pre; luaD_reallocstack((L), sz_); pos; }
#endif

#if !defined(HARDMEMTESTS)
#define condchangemem(L, pre, pos)        ((void)0)
#else
#define condchangemem(L, pre, pos)  \
  { if (G(L)->gcrunning) { pre; luaC_fullgc(L, 0); pos; } }
#endif
