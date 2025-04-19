#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "declarations.hpp"
#include "hana.hpp"

namespace xmsm::utils {
template<typename factory> constexpr auto factory_entity() {
  if constexpr(requires{ typename factory::entity; }) return type_c<typename factory::entity>;
  else if constexpr(requires{ typename factory::default_entity; }) return type_c<typename factory::default_entity>;
  else return type_c<>;
}

template<typename type, typename fnc_type> struct pointer_selector {
  fnc_type fnc;
  template<typename factory, typename user_type> constexpr pointer_selector& operator^(scenario<factory, type, user_type>& s) { fnc(s); return *this; }
  template<typename factory, typename fail, typename user_type> constexpr pointer_selector& operator^(scenario<factory, fail, user_type>&) { return *this; }
};
template<typename factory, typename type> constexpr auto cur_state_hash_from_set(auto&... scenarios){
  decltype(hash<factory>(type_c<type>)) ret{};
  auto fnc = [&](auto& s){ret=s.cur_state_hash();};
  (void)(pointer_selector<type, decltype(fnc)>{fnc} ^...^ scenarios);
  return ret;
}
template<typename factory, typename type> constexpr auto count_from_set(auto&... scenarios){
  decltype(hash<factory>(type_c<type>)) ret{};
  auto fnc = [&](auto& s){ret=s.count();};
  (void)(pointer_selector<type, decltype(fnc)>{fnc} ^...^ scenarios);
  return ret;
}
template<typename type> constexpr bool is_broken_from_set(auto&... scenarios) {
  bool ret = false;
  auto fnc = [&](auto& s){ret=s.own_state()==scenario_state::broken;};
  (void)(pointer_selector<type, decltype(fnc)>{fnc} ^...^ scenarios);
  return ret;
}
template<typename scenario> consteval auto index_of_scenario(_type_c<scenario>, auto&&... list) {
  auto check = []<typename factory, typename cur_scenario>(const basic_scenario<factory, cur_scenario>&){return type_c<scenario> == type_c<cur_scenario>;};
  auto ind=0;
  (void)(false||...||[&](const auto& cur) {
    auto ups = !check(cur);
    ind += ups;
    return !ups;
  }(list));
  return ind;
}
template<auto ind, auto cur> constexpr auto& _search_scenario(auto& first, auto&... tail) {
  if constexpr(ind==cur) return first;
  else return _search_scenario<ind, cur+1>(tail...);
}
template<typename scenario> constexpr auto& search_scenario(_type_c<scenario>, auto&&... list) {
#if defined(__cpp_pack_indexing)
  return list...[index_of_scenario(type_c<scenario>, list...)];
#else
  return _search_scenario<index_of_scenario(type_c<scenario>, list...), 0>(list...);
#endif
}
template<auto ind, auto cur> constexpr decltype(auto) _nth(auto&& first, auto&&... tail) {
  if constexpr(ind==cur) return std::forward<decltype(first)>(first);
  else return _nth<ind,cur+1>(std::forward<decltype(tail)>(tail)...);
}
template<auto ind> constexpr decltype(auto) nth(auto&&... args) {
  //TODO: GCC15: delete the function
#if defined(__cpp_pack_indexing)
  return std::forward<decltype(args...[ind])>(args...[ind]);
#else
  return _nth<ind, 0>(std::forward<decltype(args)>(args)...);
#endif
}

}