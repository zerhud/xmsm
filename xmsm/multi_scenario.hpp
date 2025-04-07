#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "single_scenario.hpp"

namespace xmsm {

template<typename factory> struct scenario_id_def_generator {
  using id_type = decltype(mk_atomic<uint64_t>(std::declval<factory>()));
  id_type last_id{};
  constexpr auto operator()(const auto&){ return ++last_id; }
};

template<typename factory, typename object, typename user_type>
struct multi_scenario : basic_scenario<factory, object> {
  using base = basic_scenario<factory, object>;
  using single_scenario_type = single_scenario<factory, object, user_type>;
  using info = base::info;

  constexpr static auto finish_state() {
    constexpr auto list = unpack(info{}, [](auto... i) {
      return (type_list{} << ... << [](auto i) {
        if constexpr(requires{decltype(+i)::is_finish_state;}) return decltype(+i)::st;
        else return type_c<>;
      }(i));
    });
    static_assert( size(list)>0, "multi scenario must to have at least one finish state" );
    static_assert( contains(single_scenario_type::all_states(), list), "some of finish states are unreachable" );
    return list;
  }
  constexpr static auto start_events() {
    constexpr auto list = filter(info{}, [](auto i){return requires{decltype(+i)::is_start_event;};});
    static_assert( size(list)>=1, "multi scenario must to have one or more start events" );
    return unpack(list, [](auto... i){return (type_list{} << ... << decltype(+i)::event);});
  }

  constexpr static auto mk_user_id_generator(const auto& f) {
    if constexpr(requires{mk_scenario_id_generator(f);}) return mk_scenario_id_generator(f);
    else return scenario_id_def_generator<factory>{};
  }

  template<typename id_type, typename single_scenario_type> struct scenario_entry { id_type id; single_scenario_type scenario; };

  constexpr static auto mk_scenarios_type(const auto& f) {
    using id_type = decltype(mk_user_id_generator(f))::id_type;
    using entry_type = scenario_entry<id_type, single_scenario_type>;
    if constexpr(requires{mk_map<id_type,entry_type>(f);}) return mk_map<id_type,single_scenario_type>(f);
    else return mk_list<entry_type>(f);
  }

  constexpr explicit multi_scenario(factory f) : base(std::move(f)), user_id_gen(mk_user_id_generator(this->f)), scenarios(mk_scenarios_type(this->f)) {}

  decltype(mk_user_id_generator(std::declval<factory>())) user_id_gen;
  decltype(mk_scenarios_type(std::declval<factory>())) scenarios;

  constexpr auto count() const { return scenarios.size(); }
  template<typename st> constexpr auto count_in() const { auto ret = 0; foreach_scenario([&](const auto& s){ret+=in_state<st>(s);}); return ret; }
  constexpr bool empty() const { return scenarios.empty(); }
  template<typename id_type> constexpr user_type* find(const id_type& id) {
    auto* s = find_scenario(id);
    return s ? &s->obj : nullptr;
  }
  template<typename id_type> constexpr single_scenario_type* find_scenario(const id_type& id) {
    return xmsm_find_pointer(this->f, scenarios, id);
  }
  constexpr auto own_state() const {
    for (auto& _s:scenarios) {
      auto& [_,s] = _s;
      if (s.own_state()!=scenario_state::ready) return s.own_state();
    }
    return scenario_state::ready;
  }
  constexpr void reset_own_state() { foreach_scenario([](auto& s){s.reset_own_state();}); }

  constexpr void on_address(const auto& e, const auto& id, auto&&... scenarios) {
    if (auto* s=find_scenario(id);s) s->on(e, scenarios...);
    clean_scenarios();
  }
  constexpr void on(const auto& e, auto&&... others) {
    if constexpr (!contains(start_events(), type_dc<decltype(e)>)) foreach_scenario([&](auto& s){s.on(e, others...);});
    else {
      auto& [_,s] = xmsm_insert_or_emplace( scenarios, user_id_gen(e), single_scenario_type{this->f, create_object<user_type>(this->f)} );
      s._own_state = scenario_state::fired;
    }
    clean_scenarios();
  }
  constexpr void on_other_scenarios_changed(const auto& e, auto&&... others) {
    foreach_scenario([&](auto& s){s.on_other_scenarios_changed(e, others...);});
    clean_scenarios();
  }
  template<typename... targets> constexpr bool move_to(const auto& e, auto&&... scenarios) {
    bool ret = true;
    foreach_scenario([&](auto& s){ret &= s.template move_to<targets...>(e, scenarios...);});
    clean_scenarios();
    return ret;
  }
  constexpr auto cur_state_hash() const {
    using hash_type = decltype(std::declval<single_scenario_type>().cur_state_hash());
    hash_type ret{};
    if (!empty()) {
      auto&[_,first_scenario] = *begin(scenarios);
      ret = first_scenario.cur_state_hash();
      foreach_scenario([&](auto& s){ret *= ret==s.cur_state_hash();});
    }
    return ret;
  }
private:
  template<typename self_type> constexpr void foreach_scenario(this self_type& self, auto&& fnc) {
    for (auto& entry : self.scenarios) {
      auto& [_,scenario] = entry;
      fnc(scenario);
    }
  }
  constexpr void clean_scenarios() {
    xmsm_erase_if(this->f, scenarios, [](const auto& i) { auto& [_,s] = i; return in_state(s, finish_state()); });
  }
};

}