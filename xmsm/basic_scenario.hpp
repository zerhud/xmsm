#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "hana.hpp"
#include "customizations.hpp"
#include "modificators.hpp"
#include "scenario_checkers.hpp"
#include "declarations.hpp"

namespace xmsm {

template<typename _from, typename _to, typename _event, typename... _mods>
struct trans_info {
  consteval static auto all_stack_by_event() { return filter(type_list<_mods...>{}, [](auto v){return requires{decltype(+v){}.is_stack_by_event;};}); }
  consteval static auto all_stack_by_expression() { return filter(type_list<_mods...>{}, [](auto v){return requires{decltype(+v){}.is_stack_by_expression;};}); }
  consteval static auto all_mod_when() { return filter(type_list<_mods...>{}, [](auto v){return requires{decltype(+v){}.is_when;};}); }
  consteval static auto all_mod_only_if() { return filter(type_list<_mods...>{}, [](auto v){return requires{decltype(+v){}.is_only_if;};}); }

  constexpr static bool is_trans_info = true;
  constexpr static auto to = type_c<_to>;
  constexpr static auto from = type_c<_from>;
  constexpr static auto event = type_c<_event>;
  constexpr static auto mods = type_list<_mods...>{};
  constexpr static auto mod_stack_by_event = first(all_stack_by_event());
  constexpr static auto mod_stack_by_expression = first(all_stack_by_expression());
  constexpr static auto mod_when = first(all_mod_when());
  constexpr static auto mod_only_if = first(all_mod_only_if());

  static_assert( size(all_stack_by_event()) < 2, "only single stack by event modification is available for transition" );
};

template<typename... adding, typename _from, typename _to, typename _event, typename... _mods>
constexpr auto add_mods(_type_c<trans_info<_from, _to, _event, _mods...>>, const type_list<adding...>&) {
  return type_c<trans_info<_from, _to, _event, _mods..., adding...>>;
}

template<typename factory, typename object> struct basic_scenario {
  struct multi_sm_indicator;
  factory f;

  constexpr explicit basic_scenario(factory f) : f(std::move(f)) {}

  friend constexpr auto mk_sm_description(const basic_scenario&, auto&&... args) {
    return type_list<decltype(+type_dc<decltype(args)>)...>{};
  }
  friend constexpr auto mk_multi_sm_description(const basic_scenario&, auto&&... args) {
    return type_list<multi_sm_indicator, decltype(+type_dc<decltype(args)>)...>{};
  }
  template<typename from, typename to, typename event=void, typename... mods>
  friend constexpr auto mk_trans(const basic_scenario&, auto&&... mods_obj) {
    constexpr auto mods_list = type_list<decltype(+type_dc<decltype(mods_obj)>)...>{};
    return unpack(mods_list, [](auto... s){return trans_info<from, to, event, decltype(+s)..., mods...>{};});
  }
  template<typename st> friend constexpr auto pick_def_state(const basic_scenario&) { return modificators::def_state<st>{}; }
  template<typename... e> friend constexpr auto stack_by_event(const basic_scenario&) { return modificators::stack_by_event<e...>{}; }
  friend constexpr auto _true(const basic_scenario&){ return scenario_checker::_true{}; }
  friend constexpr auto _false(const basic_scenario&){ return scenario_checker::_false{}; }
  template<typename s, typename... st> friend constexpr auto in(const basic_scenario&){ return scenario_checker::in<s, st...>{}; }
  template<typename s, typename... st> friend constexpr auto now_in(const basic_scenario&){ return scenario_checker::now_in<s, st...>{}; }
  template<typename e> friend constexpr auto stack_by_expr(const basic_scenario&, e) { return modificators::stack_by_expression<e>{}; }
  template<typename e> friend constexpr auto when(const basic_scenario&, e) { return modificators::when<e>{}; }
  template<typename e> friend constexpr auto only_if(const basic_scenario&, e) { return modificators::only_if<e>{}; }
  template<typename st, typename... _mods> friend constexpr auto to_state_mods(const basic_scenario&, _mods...) { return modificators::to_state_mods<st, _mods...>{}; }
  template<typename st, typename... _mods> friend constexpr auto from_state_mods(const basic_scenario&, _mods...) { return modificators::from_state_mods<st, _mods...>{}; }
  template<typename st> friend constexpr auto finish_state(const basic_scenario&){ return modificators::finish_state<st>{}; }
  template<typename event> friend constexpr auto start_event(const basic_scenario&){ return modificators::start_event<event>{}; }

  using info = decltype(object::describe_sm(std::declval<basic_scenario>()));

  constexpr static bool is_multi() {
    return first(info{}) == type_c<multi_sm_indicator>;
  }
};

}
