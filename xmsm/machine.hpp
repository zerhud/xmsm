#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "hana.hpp"
#include "scenario.hpp"

namespace xmsm {

template<typename factory, typename... scenarios_t>
struct machine {
  constexpr static auto mk_scenarios(const auto& f) {
    auto mk = [&](auto t){
      auto type = basic_scenario<factory,decltype(+t)>{f}.ch_type(t);
      if constexpr(basic_scenario<factory, decltype(+t)>::is_multi())
          return scenario<factory, decltype(+t), decltype(+type)>{f};
      else return scenario<factory, decltype(+t), decltype(+type)>{f, create_object<decltype(+type)>(f)};
    };
    return unpack(type_list<scenarios_t...>{}, [&](auto... t){ return mk_tuple(mk(t)...); });
  }

  constexpr explicit machine(factory f) : f(std::move(f)), scenarios(mk_scenarios(this->f)) {}

  factory f;
  decltype(mk_scenarios(std::declval<factory>())) scenarios;

  constexpr void try_to_repair(auto&& event) {
    foreach(scenarios, [&](auto& s) {
      unpack(scenarios, [&](auto&&...others) {
        (void)(others.on_other_scenarios_changed(event, others...),...);
      });
      return false;
    });
    foreach(scenarios, [](auto&s){s.reset_own_state();return false;});
  }
  constexpr auto on(auto&& event) {
    foreach(scenarios, [](auto&s){s.reset_own_state();return false;});
    foreach(scenarios, [&](auto& s) {
      unpack(scenarios, [&](auto&&...others) {
        s.on(event, others...);
        (void)(others.on_other_scenarios_changed(event, others...),...);
      });
      return false;
    });
  }
  template<typename sc> constexpr bool is_broken() const {return unpack(scenarios, [](auto&&... list){return utils::search_scenario(type_c<sc>, list...).own_state()==scenario_state::broken;});}
  template<typename scenario, typename state> constexpr bool in_state() const {
    return unpack(scenarios, [](auto&&... s){return (0+...+s.template in_state_by_scenario<scenario,state>());});
  }
  template<typename scenario> constexpr friend auto& get(machine& m) {
    constexpr auto ind = unpack(m.scenarios, [](auto&&... s){return utils::index_of_scenario(type_c<scenario>, s...);});
    return get<ind>(m.scenarios).obj;
  }
  template<auto ind> constexpr friend auto& get(machine& m) {
    return get<ind>(m.scenarios).obj;
  }
  template<typename scenario> constexpr friend const auto& get(const machine& m) {return get<scenario>(const_cast<machine&>(m));}
  template<auto ind> constexpr friend const auto& get(const machine& m) {return get<ind>(const_cast<machine&>(m));}
};

}