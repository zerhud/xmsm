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
#include "distribution.hpp"

namespace xmsm {

template<typename factory, typename... scenarios_t>
struct machine {
  using hash_type = decltype(hash(type_c<factory>));
  using connector_type = decltype(distribution::mk_connector<factory, scenarios_t...>(details::declval<factory>()));

  constexpr static auto mk_scenarios(const auto& f) {
    auto mk = [&](auto t){
      using base_type = basic_scenario<factory,decltype(+t)>;
      auto type = [&](auto t) {
        if constexpr(base_type::is_remote()) return type_c<connector_type>;
        else return base_type{f}.ch_type(t);
      }(t);
      if constexpr(base_type::is_remote() || base_type::is_multi())
        return scenario<factory, decltype(+t), decltype(+type), type_list<scenarios_t...>>{f};
      else return scenario<factory, decltype(+t), decltype(+type), type_list<scenarios_t...>>{f, create_object<decltype(+type)>(f)};
    };
    return mk_tuple(mk(type_c<scenarios_t>)...);
  }

  template<typename ent> constexpr static bool is_on_ent() { return type_c<ent> == utils::factory_entity<factory>();}
  constexpr static auto place() { return hash(utils::factory_entity<factory>()); }
  constexpr static auto entity_list() { return (type_list{} << ... << basic_scenario<factory, scenarios_t>::entity_list()); }

  constexpr explicit machine(factory f) : f(details::move(f)), scenarios(mk_scenarios(this->f)), connector(distribution::mk_connector<factory, scenarios_t...>(this->f)) {
    foreach(scenarios, [this](auto& s){if constexpr(s.is_remote()) s.con = &connector;return false;});
  }
  constexpr machine(const machine& other) : f(other.f), scenarios(other.scenarios), connector(distribution::mk_connector<factory, scenarios_t...>(this->f)) {
    foreach(scenarios, [this](auto& s){if constexpr(s.is_remote()) s.con = &connector;return false;});
  }
  constexpr machine(machine&& other) : f(details::move(other.f)), scenarios(details::move(other.scenarios)), connector(details::move(other.connector)) {
    foreach(scenarios, [this](auto& s){if constexpr(s.is_remote()) s.con = &connector;return false;});
  }

  factory f;
  decltype(mk_scenarios(details::declval<factory>())) scenarios;
  [[no_unique_address]] connector_type connector;

  constexpr void try_to_repair(auto&& event) {
    coordinate_scenarios(event);
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
    sync<sync_command::sync, sync_command::sync_multi>(event);
    connector.on_finish();
  }
  constexpr auto on_event_by_hash(auto ehash) {
    return foreach(all_events(), [&](auto cur_event) {
      if (hash(cur_event)==ehash) {
        on(create_object<decltype(+cur_event)>(this->f));
        return true;
      }
      return false;
    });
  }
  template<auto cmd> constexpr void from_remote(const auto* buf, auto sz) {
    connector.on_start();
    if constexpr(cmd==sync_command::move_to) from_remote_move_to(buf, sz);
    else if constexpr(cmd==sync_command::move_to_response) from_remote_sync(buf, sz);
    else if constexpr(cmd==sync_command::sync) {
      from_remote_sync(buf, sz);
      if (!is_synced()) sync_with_event<sync_command::sync, sync_command::sync_multi, decltype(all_events())>(buf[1], [](const auto&){});
    }
    else if constexpr(cmd==sync_command::sync_multi) {
      from_remote_sync_multi(buf, sz);
      sync_with_event<sync_command::sync, sync_command::sync_multi, decltype(all_events())>(buf[1], [](const auto&){});
    }
    else if constexpr(cmd==sync_command::move_to_response_multi) {
      from_remote_sync_multi(buf, sz);
    }
    else static_assert( false, "not all command handled here" );
    connector.on_finish();
  }
  template<auto clang20_compile_issue> constexpr void from_remote_rt(auto cmd, const auto* buf, auto sz) {
    //NOTE: CLANG21: the template parameter is unused, clang can't compile without it if call the from_remote_rt right inside the connector (connector::end_sync_multi_scenario)
    if (cmd==(int)sync_command::move_to) return from_remote<sync_command::move_to>(buf, sz);
    if (cmd==(int)sync_command::move_to_response) return from_remote<sync_command::move_to_response>(buf, sz);
    if (cmd==(int)sync_command::sync) return from_remote<sync_command::sync>(buf, sz);
    if (cmd==(int)sync_command::sync_multi) return from_remote<sync_command::sync_multi>(buf, sz);
    if (cmd==(int)sync_command::move_to_response_multi) return from_remote<sync_command::move_to_response_multi>(buf, sz);
    if constexpr(requires{wrong_sync_command(this->f, cmd, buf, sz);}) wrong_sync_command(this->f, cmd, buf, sz);
  }
  template<typename sc> constexpr bool is_remote() const {return unpack(scenarios, [](auto&&... list){return utils::search_scenario(type_c<sc>, list...).is_remote();});}
  template<typename sc> constexpr bool is_broken() const {return unpack(scenarios, [](auto&&... list){return utils::search_scenario(type_c<sc>, list...).own_state()==scenario_state::broken;});}
  template<typename sc> constexpr auto scenarios_count() const { return unpack(scenarios, [](auto&&...list) {
    auto& s = utils::search_scenario(type_c<sc>, list...);
    if constexpr(!s.is_multi()) return 0;
    else return s.count();
  });}
  template<typename sc, typename st> constexpr unsigned int in_state_count(this auto& m) {
    constexpr auto count = [](const auto& s) -> int {
      if constexpr(!(type_c<sc> <= s.origin())) return 0;
      else if constexpr(!s.is_multi()) return s.template in_state<st>();
      else return s.template count_in<st>();
    };
    return unpack(m.scenarios, [&](auto&&...s) { return (0+...+count(s)); });
  }
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
  template<typename e1, typename e2> constexpr static bool need_sync() {
    constexpr bool e1_to_e2 = (false||...||(contains(basic_scenario<factory, scenarios_t>::entity_list(), type_c<e1>) && basic_scenario<factory, scenarios_t>::template need_sync_with_ent<e2>()));
    constexpr bool e2_to_e1 = (false||...||(contains(basic_scenario<factory, scenarios_t>::entity_list(), type_c<e2>) && basic_scenario<factory, scenarios_t>::template need_sync_with_ent<e1>()));
    return e1_to_e2 || e2_to_e1;
  }

  constexpr static auto all_scenarios() { return (type_list{} << ... << type_c<basic_scenario<factory, scenarios_t>>); }
  constexpr static auto all_states() { return (type_list{} << ... << basic_scenario<factory, scenarios_t>::all_states()); }
  constexpr static auto all_trans() { return (type_list{} + ... + basic_scenario<factory, scenarios_t>::all_trans_info()); }
  constexpr static auto all_events() {
    constexpr auto ret = (type_list{} << ... << basic_scenario<factory, scenarios_t>::all_events());
    constexpr auto check = [](auto e){ return unpack(decltype(ret){}, [&](auto...list){return (0+...+(list<=e));}); };
    static_assert( unpack(ret, [&](auto...e){return ((check(e)==1) + ...);}) == size(ret), "an event cannot to be derived from other event" );
    return ret;
  }
private:
  template<auto cmd_s, auto cmd_m> constexpr void sync(const auto& event) {
    if constexpr(utils::factory_entity<factory>() != type_c<>) {
      sync_single<cmd_s>(event);
      sync_multi<cmd_m>(event);
    }
  }
  template<auto cmd = sync_command::sync> constexpr void sync_single(const auto& event) {
    unpack_local_to_remote<false>([&]<typename...ent>(auto&&...s){
      auto* buf = connector.allocate();
      buf[0] = place();
      buf[1] = event_hash(event);
      int ind=1;(void)((s.synced=true,buf[++ind]=s.cur_state_hash()),...);
      (send<cmd, ent>(connector, buf, 2+sizeof...(s)),...);
    });
  }
  template<auto cmd = sync_command::sync_multi> constexpr void sync_multi(const auto& event) {
    unpack_local_to_remote<true>([&]<typename... ent>(auto&&...ms) {
      begin_sync_multi_scenario<cmd>(connector, hash(utils::factory_entity<factory>()), event_hash(event));
      (void)([&](auto&s) {
        connector.multi_scenario_count(s.count());
        s.foreach_scenario([&](auto&s) { s.synced = true; connector.multi_scenario_state(s.cur_state_hash()); });
      }(ms),...);
      (sync_multi_scenario<ent, cmd>(connector),...);
    });
  }
  constexpr void from_remote_sync_multi(const auto* buf, auto sz) {
    auto source_ent = buf[0];
    auto source_event = buf[1];
    buf += 2; sz -= 2;
    unpack_remote_to_local<true>([&](auto ent, auto&... s) {
      if (hash(ent)!=source_ent) return;
      const auto* ibuf = buf;
      (void)([&](auto&s) {
        if constexpr(s.is_multi()) {
          auto cur_sz = ibuf[0];
          s.sync_multi(++ibuf, cur_sz);
          ibuf += cur_sz;
        }
      }(s),...);
    });
  }
  constexpr void from_remote_sync(const auto* buf, auto sz) {
    if (call_if_need_not_enough_data<3, sync_command::sync>(f, buf, sz)) return;
    unpack_remote_to_local<false>([&](auto ent, auto&&... s) {
      if (hash(ent) == buf[0] && !call_if_need_not_enough_data<sizeof...(s)+2, sync_command::sync>(f, buf, sz)) {
        int i=1;
        (void)((s.state(buf[++i])),...);
      }
    });
  }
  constexpr void from_remote_move_to(const auto* buf, auto sz) {
    if (call_if_need_not_enough_data<3, sync_command::move_to>(f, buf, sz)) return;
    foreach(scenarios, [&](auto&&s) {
      if constexpr(!s.is_remote()) if (s.own_hash()==buf[0]) return sync_with_event<sync_command::move_to_response, sync_command::move_to_response_multi, decltype(s.all_events())>(buf[1], [&](const auto& event) {
        unpack(scenarios, [&](auto&...others) {
          s.move_to_or_wait_cond(event, [&](auto to)->bool {
            for (auto i=2;i<sz;++i) if (buf[i]==hash(to)) return true;
            return false;
          }, others...);
        });
        return true;
      });
      return false;
    });
  }
  template<auto cmd_s, auto cmd_m, typename event_list> constexpr auto sync_with_event(auto ehash, auto&& payload) {
    return foreach(event_list{}, [&](auto cur_event) {
      if (hash(cur_event)==ehash) {
        auto event = create_object<decltype(+cur_event)>(this->f);
        payload(event);
        coordinate_scenarios(event);
        if (!is_synced()) sync<cmd_s, cmd_m>(event);
        return true;
      }
      return false;
    });
  }
  constexpr bool is_synced() const {return unpack(scenarios, [](const auto& s){ return !s.is_remote();}, [](auto&&... list){return (0+...+list.is_synced())==sizeof...(list);});}
  constexpr bool is_fired() const {return unpack(scenarios, [](auto&&... list){return (0+...+(list.own_state()==scenario_state::fired));});}
  template<bool multi=false> constexpr void unpack_local_to_remote(auto&& fnc) {
    unpack(entity_list(), [](auto e){return e!=utils::factory_entity<factory>() && need_sync<decltype(+utils::factory_entity<factory>()), decltype(+e)>();}, [&](auto... ent) {
      unpack(scenarios, [](const auto& s){return !s.is_remote() && s.is_multi()==multi;}, [&](auto&&...ms) {
        fnc.template operator()<decltype(+ent)...>(ms...);
      });
    });
  }
  template<bool multi=false> constexpr void unpack_remote_to_local(auto&& fnc) {
    foreach(entity_list(), [&](auto ent) { if constexpr (ent!=utils::factory_entity<factory>() && need_sync<decltype(+utils::factory_entity<factory>()), decltype(+ent)>()) {
      unpack(scenarios, [](const auto& s){ return s.is_remote() && s.is_multi()==multi && contains(s.entity_list(),decltype(ent){}); }, [&](auto&&...s) { fnc(ent, s...); });
    } return false; });
  }
  constexpr void coordinate_scenarios(auto& event) {
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
  template<typename type> constexpr static auto event_hash(const type&) {
    return unpack(all_events(), [](auto...list){return (0+...+(hash(list)*(list<=type_c<type>)));});
  }
};

}
