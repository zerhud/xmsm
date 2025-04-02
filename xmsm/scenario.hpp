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

namespace xmsm {

template<typename factory, typename object, typename user_type> constexpr auto mk_scenario_base_type() {
  if constexpr (basic_scenario<factory, object, user_type>::is_multi()) return type_c<multi_scenario<factory, object, user_type>>;
  else return type_c<single_scenario<factory, object, user_type>>;
}

template<typename factory, typename object, typename user_type=object>
struct scenario : decltype(+mk_scenario_base_type<factory, object, user_type>()) {
  using base = decltype(+mk_scenario_base_type<factory, object, user_type>());
  constexpr explicit scenario(factory f) : base(std::move(f)) {}
};

}