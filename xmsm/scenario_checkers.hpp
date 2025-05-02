#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "hana.hpp"
#include "declarations.hpp"

namespace xmsm::scenario_checker {

struct basic {
  constexpr static bool is_checker = true;
  constexpr static auto list_scenarios() { return type_list{}; }
};
struct _true : basic {constexpr bool operator()(auto&&...) const {return true;}};
struct _false : basic {constexpr bool operator()(auto&&...) const {return false;}};
template<typename sc, typename... st> struct in : basic {
  constexpr static auto scenario = type_c<sc>;
  constexpr static auto states = (type_list{} << ... << type_c<st>);

  constexpr bool operator()(auto&&... s) const { return (false || ... || check(s)); }
  template<typename factory, typename obj, typename... tail> constexpr static bool check(const xmsm::scenario<factory, obj, tail...>& s) {
    auto cur = s.cur_state_hash();
    if constexpr(scenario != type_dc<obj>) return false;
    else return unpack(states, [&](auto... i){ return (false || ... || (hash(i)==cur)); });
  }
  constexpr static auto list_scenarios() { return type_list<sc>{}; }
};
template<typename sc, typename... st> using all_in = in<sc, st...>;

template<typename sc, typename... st> struct now_in : basic {
  constexpr static auto scenario = type_c<sc>;
  constexpr static auto states = (type_list{} << ... << type_c<st>);

  constexpr bool operator()(auto&&... s) const { return (false || ... || check(s)); }
  template<typename factory, typename obj, typename... tail> constexpr static bool check(const xmsm::scenario<factory, obj, tail...>& s) {
    return s.own_state() == scenario_state::fired && in<sc,st...>::check(s);
  }
  constexpr static auto list_scenarios() { return type_list<sc>{}; }
};
template<typename sc, auto cnt, typename... st> struct count_in : basic {
  constexpr static auto scenario = type_c<sc>;
  constexpr static auto states = (type_list{} << ... << type_c<st>);

  constexpr bool operator()(auto&&... s) const { return (false || ... || check(s)); }
  template<typename factory, typename obj, typename... tail> constexpr static bool check(const xmsm::scenario<factory, obj, tail...>& s) {
    if constexpr(type_dc<obj> != type_c<sc>) return false;
    else if constexpr(!s.is_multi()) return in<sc, st...>::check(s);
    else return cnt <= s.template count_in<st...>();
  }
  constexpr static auto list_scenarios() { return type_list<sc>{}; }
};
template<typename sc, auto req_st> struct in_own_state : basic {
  constexpr static auto scenario = type_c<sc>;
  constexpr bool operator()(auto&&... s) const { return (false || ... || check(s)); }
  template<typename factory, typename obj, typename... tail> constexpr static bool check(const xmsm::scenario<factory, obj, tail...>& s) {
    if constexpr(type_dc<obj> != type_c<sc>) return false;
    else return s.own_state() == req_st;
  }
  constexpr static auto list_scenarios() { return type_list<sc>{}; }
};

template<typename l, typename r> struct and_checker : basic {
  constexpr static auto left = type_c<l>;
  constexpr static auto rigth = type_c<r>;
  constexpr bool operator()(auto&&... s) const { return l{}(static_cast<decltype(s)&&>(s)...) && r{}(static_cast<decltype(s)&&>(s)...); }
  constexpr static auto list_scenarios() { return type_list{} << l::list_scenarios() << r::list_scenarios(); }
};
template<typename l, typename r> struct or_checker : basic {
  constexpr static auto left = type_c<l>;
  constexpr static auto rigth = type_c<r>;
  constexpr bool operator()(auto&&... s) const { return l{}(static_cast<decltype(s)&&>(s)...) || r{}(static_cast<decltype(s)&&>(s)...); }
  constexpr static auto list_scenarios() { return type_list{} << l::list_scenarios() << r::list_scenarios(); }
};
template<typename c> struct not_checker : basic {
  constexpr static auto checker = type_c<c>;
  constexpr bool operator()(auto&&... s) const { return !c{}(static_cast<decltype(s)&&>(s)...); }
  constexpr static auto list_scenarios() { return type_list{} << c::list_scenarios(); }
};

constexpr auto operator!(auto&& c){ return not_checker<decltype(+type_dc<decltype(c)>)>{}; }
constexpr auto operator||(auto&& l, auto&& r){ return or_checker<decltype(+type_dc<decltype(l)>), decltype(+type_dc<decltype(r)>)>{}; }
constexpr auto operator&&(auto&& l, auto&& r){ return and_checker<decltype(+type_dc<decltype(l)>), decltype(+type_dc<decltype(r)>)>{}; }

}
