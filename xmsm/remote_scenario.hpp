#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "basic_scenario.hpp"

namespace xmsm {

template<typename factory, typename object, typename connector> struct remote_scenario : basic_scenario<factory, object> {
  using base = basic_scenario<factory, object>;
  using hash_type = decltype(hash<factory>(type_c<factory>));

  using base::all_events;

  hash_type cur_state{};
  connector* con=nullptr;
  scenario_state _own_state{scenario_state::ready};

  constexpr explicit remote_scenario(factory f) : base(std::move(f)) {}
  constexpr void reset_own_state() { _own_state = scenario_state::ready; }
  constexpr static void on_other_scenarios_changed(auto&&...) {}
  constexpr static void on(auto&&...) {}
  template<typename... targets> constexpr bool move_to(const auto& e, auto&&...) {
    return move_to_or_wait<targets...>(e);
  }
  template<typename... targets> constexpr bool move_to_or_wait(const auto& event, auto&&...) {
    auto event_hash = unpack(all_events(), [](auto&&... e){return (0+...+(hash<factory>(e)*(e<=type_dc<decltype(event)>)));});
    //con->template send_command<decltype(+base::entity()), sync_command::move_to>(this->own_hash(), event_hash, hash<factory>(type_c<targets>)...);
    auto* buf = con->allocate();
    buf[0] = this->own_hash();
    buf[1] = event_hash;
    int i=1; (void)((buf[++i]=hash<factory>(type_c<targets>)),...);
    con->template send<decltype(+base::entity()), sync_command::move_to>(buf, sizeof...(targets)+2);
    return true;
  }
  constexpr scenario_state own_state() const {return _own_state;}
  constexpr void state(auto hash) {
    _own_state = (scenario_state)((int)_own_state*(cur_state==hash)+(int)scenario_state::fired*(cur_state!=hash));
    cur_state = hash;
  }
  constexpr auto cur_state_hash() const {return cur_state;}
  template<typename state> constexpr bool in_state() const {
    //TODO: store the index_of_by_hash result in class field (changed only in state method)?
    return index_of(this->all_states(), type_c<state>) == index_of_by_hash<factory>(this->all_states(), cur_state);
  }
};

}