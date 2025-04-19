/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "factory.hpp"

#include <iostream>

using namespace std::literals;
template<auto v> using state = tests::state<v>;
template<auto v> using event = tests::event<v>;

struct entity_1 {};
struct entity_2 {};

template<auto max_cmd_data> struct connector;
struct basic_factory : tests::factory {
  using default_entity = entity_1;
  template<auto max_cmd_data> using connector = struct connector<max_cmd_data>;
};
struct factory_default : basic_factory { };
struct factory_e2 : basic_factory { using entity = entity_2; };

struct ts_queue_e2 {
  using entity = entity_2;
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<0>>(f, allow_move(f))
      , mk_qtrans<state<0>, state<2>, event<1>>(f, allow_move(f))
    );
  }
};
struct ts_when_e2 {
  using entity = entity_2;
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>>(f, when(f, in<ts_queue_e2, state<1>>(f) && affected<ts_queue_e2>(f)))
    );
  }
};
struct ts_no_entity {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f, move_to<ts_queue_e2, state<1>, state<100>>(f))
      , mk_trans<state<0>, state<2>, event<1>>(f, try_move_to<ts_queue_e2, state<2>>(f))
      , mk_trans<state<2>, state<3>>(f, when(f, in<ts_queue_e2, state<2>>(f) && affected<ts_queue_e2>(f)))
    );
  }
};
struct ts_multi_e1 {
  using entity = entity_1;
  static auto describe_sm(const auto& f) {
    return mk_multi_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f)
      , mk_trans<state<1>, state<100>>(f, allow_move(f))
      , start_event<event<100>>(f), finish_state<state<100>>(f)
    );
  }
};
struct ts_multi_e2 {
  using entity = entity_2;
  static auto describe_sm(const auto& f) {
    return mk_multi_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f)
      , mk_trans<state<1>, state<100>>(f, allow_move(f))
      , start_event<event<100>>(f), finish_state<state<100>>(f)
    );
  }
};

template<typename factory> using machine = xmsm::machine<factory, ts_no_entity, ts_queue_e2, ts_when_e2, ts_multi_e1, ts_multi_e2>;
using machine1 = machine<factory_default>;
using machine2 = machine<factory_e2>;

template<auto max_cmd_size> struct connector {
  machine1* m1;
  machine2* m2;
  uint32_t buf[max_cmd_size]{};

  constexpr static void on_start() {}
  constexpr static void on_finish() {}
  constexpr uint32_t* allocate() { return &buf[0]; }
  template<typename ent, auto cmd> constexpr void send(auto* buf, auto sz) const {
    if !consteval {
      std::cout << "send to: " << name<factory_e2>(xmsm::type_c<ent>) << " command: " << (int)cmd << " size: " << sz << std::endl;
      for (auto i=0;i<sz;++i) std::cout << "\t" << i << " = " << buf[i] << std::endl;
    }
    if constexpr(machine1::is_on_ent<ent>()) m1->from_remote<cmd>(buf, sz);
    else if constexpr(machine2::is_on_ent<ent>()) m2->from_remote<cmd>(buf, sz);
  }
};

static_assert( [] {
  xmsm::remote_scenario<factory_e2, ts_no_entity, connector<30>> r{factory_e2{}};
  r.state(30);
  r.reset_own_state();
  if (r.own_state()!=xmsm::scenario_state::ready) throw __LINE__;
  r.state(30);
  if (r.own_state()!=xmsm::scenario_state::ready) throw __LINE__;
  r.state(31);
  return r.own_state();
}() == xmsm::scenario_state::fired );


static_assert( xmsm::scenario<factory_default, ts_no_entity>::entity() == xmsm::type_c<entity_1> );
static_assert( xmsm::scenario<factory_default, ts_queue_e2>::entity() == xmsm::type_c<entity_2> );
static_assert( xmsm::scenario<factory_e2, ts_no_entity>::entity() == xmsm::type_c<entity_1> );
static_assert( machine1{factory_default{}}.is_remote<ts_queue_e2>() );
static_assert( !machine1{factory_default{}}.is_remote<ts_no_entity>() );
static_assert( !machine2{factory_e2{}}.is_remote<ts_queue_e2>() );
static_assert( machine2{factory_e2{}}.is_remote<ts_no_entity>() );

static_assert( [] {
  machine1 m1{factory_default{}};
  machine2 m2{factory_e2{}};
  m1.connector.m2 = &m2;
  m2.connector.m1 = &m1;
  m2.on(event<0>{});
  return m1.in_state<ts_queue_e2, state<1>>() + 2*m2.in_state<ts_queue_e2, state<1>>()
  + 4*m1.in_state<ts_no_entity, state<0>>() + 8*m2.in_state<ts_no_entity, state<0>>()
  ;
}() == 15 );
static_assert( [] {
  machine1 m1{factory_default{}};
  machine2 m2{factory_e2{}};
  m1.connector.m2 = &m2;
  m2.connector.m1 = &m1;
  m1.on(event<0>{});
  return m1.in_state<ts_no_entity, state<1>>() + 2*m1.in_state<ts_queue_e2, state<1>>()
    + 4*m2.in_state<ts_no_entity, state<1>>() + 8*m2.in_state<ts_queue_e2, state<1>>()
    + 16*!get<0>(m1.scenarios).move_to_tracker.is_active()
    + 32*m2.in_state<ts_when_e2, state<1>>() + 64*(get<2>(m2.scenarios).own_state()==xmsm::scenario_state::ready)
  ;
}() == 127 );
static_assert( [] {
  machine1 m1{factory_default{}};
  machine2 m2{factory_e2{}};
  m1.connector.m2 = &m2;
  m2.connector.m1 = &m1;
  m1.on(event<1>{});
  return m2.in_state<ts_queue_e2, state<2>>() + 2*m1.in_state<ts_no_entity, state<3>>();
}() == 3 );

int main(int,char**) {
  std::cout << "1: " << hash<factory_e2>(xmsm::type_c<state<1>>) << " 2: " << hash<factory_e2>(xmsm::type_c<state<2>>) << "\n" << std::endl;
  machine1 m1{factory_default{}};
  machine2 m2{factory_e2{}};
  m1.connector.m2 = &m2;
  m2.connector.m1 = &m1;
  m1.on(event<0>{});
  std::cout << get<0>(m1.scenarios).cur_state_hash() << std::endl;
}