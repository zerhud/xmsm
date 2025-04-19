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

struct fake_connector {
  constexpr static void on_start() {}
  constexpr static void on_finish() {}
  constexpr static int* allocate() { return nullptr; }
  template<typename> constexpr static void send(auto&&...){}
};

template<typename factory, typename... scenarios_t>
struct machine {
  constexpr static auto max_command_data_size() {
    constexpr auto sync_cmd = 2 + unpack(type_list<scenarios_t...>{}, [](auto... s) { return (0+...+!basic_scenario<factory,decltype(+s)>::is_remote()); });
    constexpr auto move_to_cmd = unpack(type_list<scenarios_t...>{}, [](auto... s) { return (2+...+[](auto s) {
      return unpack(basic_scenario<factory, decltype(+s)>::all_trans_info(), [](auto...t) { return (0+...+t().is_queue_allowed); });
    }(s));});
    return sync_cmd*(move_to_cmd<sync_cmd) + move_to_cmd*(sync_cmd<=move_to_cmd);
  }

  using hash_type = decltype(hash<factory>(type_c<factory>));
  constexpr static auto mk_connector(const auto& f) {
    if constexpr(!remote_exists()) return fake_connector{};
    else {
      return typename factory::template connector<max_command_data_size()>{};
    }
  }
  constexpr static auto mk_scenarios(const auto& f) {
    auto mk = [&](auto t){
      using base_type = basic_scenario<factory,decltype(+t)>;
      auto type = [&](auto t) {
        if constexpr(base_type::is_remote()) return type_c<decltype(mk_connector(f))>;
        else return base_type{f}.ch_type(t);
      }(t);
      if constexpr(base_type::is_remote() || base_type::is_multi())
        return scenario<factory, decltype(+t), decltype(+type)>{f};
      else return scenario<factory, decltype(+t), decltype(+type)>{f, create_object<decltype(+type)>(f)};
    };
    return mk_tuple(mk(type_c<scenarios_t>)...);
  }

  template<typename ent> constexpr static bool is_on_ent() { return type_c<ent> == utils::factory_entity<factory>();}
  constexpr static auto place() { return hash<factory>(utils::factory_entity<factory>()); }
  constexpr static auto entity_list() {
    return (type_list{} << ... << basic_scenario<factory, scenarios_t>::entity());
  }

  constexpr explicit machine(factory f) : f(std::move(f)), scenarios(mk_scenarios(this->f)), connector(mk_connector(this->f)) {
    foreach(scenarios, [this](auto& s){if constexpr(s.is_remote()) s.con = &connector;return false;});
  }
  constexpr static bool remote_exists() { return (0+...+basic_scenario<factory, scenarios_t>::is_remote()); }

  factory f;
  decltype(mk_scenarios(std::declval<factory>())) scenarios;
  [[no_unique_address]] decltype(mk_connector(std::declval<factory>())) connector;

  constexpr void try_to_repair(auto&& event) {
    sync_scenarios(event);
    reset_states();
  }
  constexpr auto on(auto&& event) {
    reset_states();
    connector.on_start();
    foreach(scenarios, [&](auto& s) {
      unpack(scenarios, [&](auto&&...others) {
        s.on(event, others...);
        (void)(others.on_other_scenarios_changed(event, others...),...);
      });
      return false;
    });
    sync(event);
    connector.on_finish();
  }
  template<auto cmd> constexpr auto from_remote(const auto* buf, auto sz) {
    connector.on_start();
    if constexpr(cmd==sync_command::sync) from_remote_sync(buf, sz);
    else if constexpr(cmd==sync_command::move_to) from_remote_move_to(buf, sz);
    connector.on_finish();
  }
  template<typename sc> constexpr bool is_remote() const {return unpack(scenarios, [](auto&&... list){return utils::search_scenario(type_c<sc>, list...).is_remote();});}
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
private:
  constexpr void sync(const auto& event) {
    foreach_local([&](auto ent, auto&&... s) {
      auto* buf = connector.allocate();
      buf[0] = hash<factory>(utils::factory_entity<factory>());
      buf[1] = event_hash(event);
      int ind=1;(void)((buf[++ind]=s.cur_state_hash()),...);
      connector.template send<decltype(+ent), sync_command::sync>(buf, 2+sizeof...(s));
    });
  }
  constexpr void from_remote_sync(const auto* buf, auto sz) {
    if (sz<3) {
      if constexpr(requires{on_not_enough_syn_data<sync_command::sync>(f, buf, sz);}) on_not_enough_syn_data<sync_command::sync>(f, buf, sz);
      return;
    }
    foreach_remote([&](auto ent, auto&&... s) {
      if (hash<factory>(ent) == buf[0]) {
        if constexpr(requires{on_not_enough_syn_data<sync_command::sync>(f, buf, sz);}) if (sz < sizeof...(s)+2) on_not_enough_syn_data<sync_command::sync>(f, buf, sz);
        if (sz >= sizeof...(s)+2) {
          int i=1;
          (void)((s.state(buf[++i])),...);
        }
      }
    });
    if (is_fired()) sync_with_event<decltype(all_events())>(buf[1], [](const auto&){});
  }
  constexpr void from_remote_move_to(const auto* buf, auto sz) {
    if (sz<3) {
      if constexpr(requires{on_not_enough_syn_data<sync_command::move_to>(f, buf, sz);}) on_not_enough_syn_data<sync_command::move_to>(f, buf, sz);
      return;
    }
    foreach(scenarios, [&](auto&&s) {
      if constexpr(!s.is_remote()) if (s.own_hash()==buf[0]) return sync_with_event<decltype(s.all_events())>(buf[1], [&](const auto& event) {
        unpack(scenarios, [&](auto&...others) {
          s.move_to_or_wait_cond(event, [&](auto to)->bool {
            for (auto i=2;i<sz;++i) if (buf[i]==hash<factory>(to)) return true;
            return false;
          }, others...);
        });
        return true;
      });
      return false;
    });
  }
  template<typename event_list> constexpr auto sync_with_event(auto ehash, auto&& payload) {
    return foreach(event_list{}, [&](auto cur_event) {
      if (hash<factory>(cur_event)==ehash) {
        auto event = create_object<decltype(+cur_event)>(this->f);
        payload(event);
        sync_scenarios(event);
        if (is_fired()) {
          reset_states();
          sync(event);
        }
        return true;
      }
      return false;
    });
  }
  constexpr bool is_fired() const {return unpack(scenarios, [](auto&&... list){return (0+...+(list.own_state()==scenario_state::fired));});}
  template<bool multi=false> constexpr void foreach_local(auto&& fnc) {
    foreach(entity_list(), [&](auto ent) { if constexpr (ent!=utils::factory_entity<factory>()) {
      unpack(scenarios, [](const auto& s){ return !s.is_remote() && s.is_multi()==multi; }, [&](auto&&...s) { fnc(ent, s...); });
    } return false; });
  }
  template<bool multi=false> constexpr void foreach_remote(auto&& fnc) {
    foreach(entity_list(), [&](auto ent) { if constexpr (ent!=utils::factory_entity<factory>()) {
      unpack(scenarios, [](const auto& s){ return s.is_remote() && s.is_multi()==multi; }, [&](auto&&...s) { fnc(ent, s...); });
    } return false; });
  }
  constexpr void sync_scenarios(auto& event) {
    foreach(scenarios, [&](auto& s) {
      unpack(scenarios, [&](auto&&...others) {
        (void)(others.on_other_scenarios_changed(event, others...),...);
      });
      return false;
    });
  }
  constexpr void reset_states() {
    foreach(scenarios, [](auto&s){s.reset_own_state();return false;});
  }
  constexpr static auto all_events() {
    constexpr auto ret = (type_list{} << ... << basic_scenario<factory, scenarios_t>::all_events());
    constexpr auto check = [](auto e){ return unpack(decltype(ret){}, [&](auto...list){return (0+...+(list<=e));}); };
    static_assert( unpack(ret, [&](auto...e){return ((check(e)==1) + ...);}) == size(ret), "an event cannot to be derived from other event" );
    return ret;
  }
  template<typename type> constexpr static auto event_hash(const type&) {
    return unpack(all_events(), [](auto...list){return (0+...+(hash<factory>(list)*(list<=type_c<type>)));});
  }
};

}
