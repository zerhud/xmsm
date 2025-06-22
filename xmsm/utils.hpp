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
  template<typename factory, typename user_type, typename olist> constexpr pointer_selector& operator^(scenario<factory, type, user_type, olist>& s) { fnc(s); return *this; }
  template<typename factory, typename fail, typename user_type, typename olist> constexpr pointer_selector& operator^(scenario<factory, fail, user_type, olist>&) { return *this; }
};
template<typename type> constexpr auto cur_state_hash_from_set(auto&... scenarios){
  decltype(hash(type_c<type>)) ret{};
  auto fnc = [&](auto& s){ret=s.cur_state_hash();};
  (void)(pointer_selector<type, decltype(fnc)>{fnc} ^...^ scenarios);
  return ret;
}
template<typename type> constexpr auto count_from_set(auto&... scenarios){
  decltype(hash(type_c<type>)) ret{};
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
template<typename scenario> constexpr void for_scenario(_type_c<scenario>, auto&& fnc, auto&&...list) {
  (void)([&](auto& s, auto type) {
    if constexpr(type_c<tags::user_object<scenario>> <= type) return fnc(s), true;
    else return false;
  }(list, type_dc<decltype(list)>) || ...);
}

}