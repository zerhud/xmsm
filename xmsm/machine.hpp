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

  constexpr auto on(auto&& event) {
    foreach(scenarios, [&](auto& s) {
      unpack(scenarios, [&](auto&&...others) {
        s.on(event, others...);
        s.on_other_scenarios_changed(event, others...);
      });
      return false;
    });
  }
};

}