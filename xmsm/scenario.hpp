#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "single_scenario.hpp"
#include "multi_scenario.hpp"
#include "remote_scenario.hpp"

namespace xmsm {

template<typename factory, typename object, typename user_type, typename other_scenarios> constexpr auto mk_scenario_base_type() {
  using basic = basic_scenario<factory, object>;
  if constexpr(basic::is_remote()) return type_c<remote_scenario<factory, object, user_type>>;
  else if constexpr (basic::is_multi()) return type_c<multi_scenario<factory, object, user_type, other_scenarios>>;
  else return type_c<single_scenario<factory, object, user_type, other_scenarios>>;
}

template<typename factory, typename object, typename user_type=object, typename other_scenarios=type_list<>>
struct scenario : decltype(+mk_scenario_base_type<factory, object, user_type, other_scenarios>()) {
  using base = decltype(+mk_scenario_base_type<factory, object, user_type, other_scenarios>());
  constexpr explicit scenario(factory f) : base(details::move(f)) {}
  constexpr explicit scenario(factory f, user_type uo) : base(details::move(f), details::move(uo)) {}
  template<typename scenario, typename state> constexpr bool in_state_by_scenario() const {
    if constexpr(type_c<object> <= type_c<scenario>) {
      if constexpr(base::is_multi()) return base::count() == base::template count_in<state>();
      else return base::template in_state<state>();
    }
    else return false;
  }
  constexpr base& debug_get_base() { return (base&)*this; }
};

}