#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "hana.hpp"
#include "basic_scenario.hpp"

namespace xmsm::distribution {

template<typename factory, typename... scenarios_t> constexpr auto max_command_data_size() {
  constexpr auto sync_cmd = 2 + unpack(type_list<scenarios_t...>{}, [](auto... s) { return (0+...+!basic_scenario<factory,decltype(+s)>::is_remote()); });
  constexpr auto move_to_cmd = unpack(type_list<scenarios_t...>{}, [](auto... s) { return (2+...+[](auto s) {
    return unpack(basic_scenario<factory, decltype(+s)>::all_trans_info(), [](auto...t) { return (0+...+t().is_queue_allowed); });
  }(s));});
  return sync_cmd*(move_to_cmd<sync_cmd) + move_to_cmd*(sync_cmd<=move_to_cmd);
}

template<typename factory, typename... sc> constexpr bool remote_exists() { return (0+...+basic_scenario<factory, sc>::is_remote()); }

struct fake_connector {
  constexpr static void on_start() {}
  constexpr static void on_finish() {}
};

template<typename factory, typename... scenarios_t> constexpr static auto mk_connector(const auto& f) {
  if constexpr(!remote_exists<factory, scenarios_t...>()) return fake_connector{};
  else {
    return typename factory::template connector<distribution::max_command_data_size<factory, scenarios_t...>()>{};
  }
}

}