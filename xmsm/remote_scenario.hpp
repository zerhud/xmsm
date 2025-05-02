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

struct fake_multi_container {
  constexpr static auto count() { return 0; }
};
template<typename factory> struct multi_scenario_info {
  using sz_type = decltype(sizeof(factory));
  decltype(mk_vec<sz_type>(details::declval<factory>())) states;
  constexpr auto count() const { return states.size(); }
};

template<typename factory, typename object, typename connector> struct remote_scenario : basic_scenario<factory, object> {
  using base = basic_scenario<factory, object>;
  using hash_type = decltype(hash(type_c<factory>));

  using base::all_events;

  constexpr static auto mk_multi_container(const auto& f) {
    if constexpr(!base::is_multi()) return fake_multi_container{};
    else return multi_scenario_info<decltype(+type_dc<decltype(f)>)>{ mk_vec<decltype(sizeof(f))>(f) };
  }

  connector* con=nullptr;

  constexpr explicit remote_scenario(factory f) : base(details::move(f)), cur_state(hash(base::initial_state())), multi_container(mk_multi_container(this->f)) {}
  constexpr void reset_own_state() { _own_state = scenario_state::ready; }
  constexpr static void on_other_scenarios_changed(auto&&...) {}
  constexpr static void on(auto&&...) {}
  template<typename... targets> constexpr bool move_to(const auto& e, auto&&...) {
    return move_to_or_wait<targets...>(e);
  }
  template<typename... targets> constexpr bool move_to_or_wait(const auto& event, auto&&...) {
    if constexpr(!base::is_multi()) return move_to_or_wait_for_single<targets...>(cur_state_hash(), event);
    else {
      bool ret = true;
      for (auto& hash:multi_container.states) ret &= move_to_or_wait_for_single<targets...>(hash, event);
      return ret;
    }
  }
  constexpr static bool is_synced() {return true;}
  constexpr scenario_state own_state() const {return _own_state;}
  constexpr void state(auto hash) requires (!base::is_multi()) {
    _own_state = (scenario_state)((int)_own_state*(cur_state==hash)+(int)scenario_state::fired*(cur_state!=hash));
    cur_state = hash;
    cur_state_index = index_of_by_hash(this->all_states(), cur_state_hash());
  }
  constexpr auto cur_state_hash() const { return cur_state; }
  template<typename state> constexpr bool in_state() const requires (!base::is_multi()) {
    return index_of(this->all_states(), type_c<state>) == cur_state_index;
  }
  constexpr auto count() const requires (base::is_multi()) { return multi_container.count(); }
  template<typename _st> constexpr auto count_in() const requires (base::is_multi()) {
    constexpr auto st_hash = hash(find(base::all_states(), type_c<_st>));
    auto ret = 0;
    for (auto& st:multi_container.states) ret += st==st_hash;
    return ret;
  }
  constexpr void sync_multi(const auto* data, auto sz) requires (base::is_multi()) {
    multi_container.states.resize(sz);
    for (auto i=0;i<sz;++i) multi_container.states[i] = data[i];
    cur_state = 0;
    if (sz != 0) {
      cur_state = multi_container.states[0];
      for (auto s:multi_container.states) cur_state *= s==cur_state;
    }
  }
  constexpr void foreach_scenario_state(auto&& fnc) const requires (base::is_multi()) { for (auto& s:multi_container.states) fnc(s); }
private:
  template<typename... targets> constexpr bool move_to_or_wait_for_single(auto cur_hash, const auto& event) {
    constexpr auto event_hash = unpack(all_events(), [](auto&&... e){return (0+...+(hash(e)*(e<=type_dc<decltype(event)>)));});
    return base::transactions_for_move_to(cur_hash, [](auto to)->bool{return (0+...+(type_c<targets> == to));}, [&](auto t) {
      auto* buf = con->allocate();
      buf[0] = this->own_hash();
      buf[1] = event_hash;
      int i=1; (void)((buf[++i]=hash(type_c<targets>)),...);
      send<sync_command::move_to, decltype(+base::entity())>(*con, buf, sizeof...(targets)+2);
      return true;
    });
  }

  hash_type cur_state{};
  unsigned cur_state_index{};
  scenario_state _own_state {scenario_state::ready};
  [[no_unique_address]] decltype(mk_multi_container(details::declval<factory>())) multi_container;
};

}
