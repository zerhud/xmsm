#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "../factory.hpp"

#include <iostream>

using namespace std::literals;
template<auto v> using state = tests::state<v>;
template<auto v> using event = tests::event<v>;

struct entity_1 {};
struct entity_2 {};
struct entity_3 {};
struct entity_4 {};
struct entity_5 {};

template<auto max_cmd_data> struct connector;
struct basic_factory : tests::factory {
  using default_entity = entity_1;
  template<auto max_cmd_data> using connector = struct connector<max_cmd_data>;
};
struct factory_default : basic_factory { };
struct factory_e2 : basic_factory { using entity = entity_2; };
struct factory_e3 : basic_factory { using entity = entity_3; };
struct factory_e4 : basic_factory { using entity = entity_4; };
struct factory_e5 : basic_factory { using entity = entity_5; };

struct ts_queue_e2 {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<0>>(f, allow_move(f))
      , mk_qtrans<state<0>, state<2>, event<1>>(f, allow_move(f))
      , entity<entity_2>(f)
    );
  }
};
struct ts_when_e2 {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>>(f, when(f, in<ts_queue_e2, state<1>>(f) && affected<ts_queue_e2>(f)))
      , entity<entity_2>(f)
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
struct ts_e3 {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<1>>(f, allow_move(f))
      , mk_qtrans<state<1>, state<2>, event<1>>(f, allow_move(f))
      , mk_qtrans<state<2>, state<3>, event<1>>(f, allow_move(f))
      , entity<entity_3>(f)
    );
  }
};
struct ts_multi_e1 {
  static auto describe_sm(const auto& f) {
    return mk_multi_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<0>>(f, allow_move(f))
      , mk_qtrans<state<1>, state<2>>(f, allow_move(f))
      , mk_trans<state<1>, state<3>, event<111>>(f, allow_move(f))
      , mk_trans<state<1>, state<100>, event<110>>(f, allow_move(f))
      , start_event<event<100>>(f), finish_state<state<100>>(f)
      , entity<entity_1>(f)
    );
  }
};
struct ts_multi_e2 {
  static auto describe_sm(const auto& f) {
    return mk_multi_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f, move_to<ts_multi_e1, state<2>, state<100>>(f))
      , mk_trans<state<1>, state<100>>(f, allow_move(f))
      , start_event<event<100>>(f), finish_state<state<100>>(f)
      , entity<entity_2>(f)
    );
  }
};
struct ts_multi_e2_2 {
  static auto describe_sm(const auto& f) {
    return mk_multi_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f, move_to<ts_multi_e1, state<2>, state<200>>(f))
      , mk_trans<state<0>, state<50>, event<5>>(f) , mk_trans<state<50>, state<51>, event<5>>(f) , mk_trans<state<51>, state<52>, event<5>>(f) , mk_trans<state<52>, state<53>, event<5>>(f)
      , mk_trans<state<1>, state<200>>(f, allow_move(f))
      , start_event<event<200>>(f), finish_state<state<200>>(f)
      , entity<entity_2>(f)
    );
  }
};
struct ts_sep_e4 {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<1>>(f, allow_move(f))
      , mk_trans<state<1>, state<0>, event<0>>(f)
      , entity<entity_4>(f)
    );
  }
};
struct ts_sep_e5 {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
    , mk_trans<state<0>, state<1>, event<1>>(f, move_to<ts_sep_e4, state<1>, state<100>>(f))
    , mk_trans<state<1>, state<0>, event<0>>(f)
    , entity<entity_5>(f)
    );
  }
};

using scenarios_list = xmsm::type_list<ts_no_entity, ts_queue_e2, ts_when_e2, ts_multi_e1, ts_multi_e2, ts_multi_e2_2, ts_e3, ts_sep_e4, ts_sep_e5>;
template<typename factory> using machine = xmsm::machine<factory, ts_no_entity, ts_queue_e2, ts_when_e2, ts_multi_e1, ts_multi_e2, ts_multi_e2_2, ts_e3, ts_sep_e4, ts_sep_e5>;
using machine1 = machine<factory_default>;
using machine2 = machine<factory_e2>;
using machine3 = machine<factory_e3>;
using machine4 = machine<factory_e4>;
using machine5 = machine<factory_e5>;

template<auto max_cmd_size> struct connector {
  machine1* m1;
  machine2* m2;
  machine3* m3;
  machine4* m4;
  machine5* m5;
  uint32_t buf[max_cmd_size]{};

  uint32_t multi_cmd{};
  std::vector<uint64_t> multi;

  constexpr static void on_start() {}
  constexpr static void on_finish() {}
  constexpr uint32_t* allocate() { return &buf[0]; }
  template<auto cmd, typename ent> constexpr friend void send(connector& self, auto* buf, auto sz) {
    if !consteval {
      std::cout << "send to: " << name<factory_e2>(xmsm::type_c<ent>) << " command: " << (int)cmd << " size: " << sz << std::endl;
      for (auto i=0;i<sz;++i) std::cout << "\t" << i << " = " << buf[i] << std::endl;
    }
    if constexpr(machine1::is_on_ent<ent>()) self.m1->from_remote<cmd>(buf, sz);
    else if constexpr(machine2::is_on_ent<ent>()) self.m2->from_remote<cmd>(buf, sz);
    else if constexpr(machine3::is_on_ent<ent>()) self.m3->from_remote<cmd>(buf, sz);
    else if constexpr(machine4::is_on_ent<ent>()) self.m4->from_remote<cmd>(buf, sz);
    else if constexpr(machine5::is_on_ent<ent>()) self.m5->from_remote<cmd>(buf, sz);
    else throw __LINE__;
  }

  template<auto cmd> constexpr friend void begin_sync_multi_scenario(connector& self, auto source, auto event) {
    if !consteval {
      std::cout << "send to multi, command: " << (int)cmd << std::endl;
    }
    self.multi_cmd = (uint32_t)cmd;
    self.multi.clear();
    self.multi.emplace_back(source), self.multi.emplace_back(event);
  }
  constexpr void multi_scenario_count(auto sz) { multi.reserve(multi.size()+sz+1); multi.emplace_back(sz); }
  constexpr void multi_scenario_state(auto st) { multi.emplace_back(st); }
  template<typename target,auto cmd> constexpr friend void sync_multi_scenario(connector& self) {
    if !consteval {
      std::cout << "\tend: " << __LINE__ << " to: " << name<factory_e2>(xmsm::type_c<target>) << ' ' << self.multi.size() << std::endl;
      std::cout << "\t\t";
      for (auto& d:self.multi) std::cout << ' ' << d ;
      std::cout << std::endl;
    }
    if (self.multi.size() < 4) return;
    if (std::is_same_v<target,entity_1>) self.m1->from_remote_rt<cmd>(self.multi_cmd, self.multi.data(), self.multi.size());
    else if (std::is_same_v<target,entity_2>) self.m2->from_remote_rt<cmd>(self.multi_cmd, self.multi.data(), self.multi.size());
    else if (std::is_same_v<target,entity_3>) self.m3->from_remote_rt<cmd>(self.multi_cmd, self.multi.data(), self.multi.size());
    else throw __LINE__;
  }
};

constexpr auto mk_m() {
  struct {
    machine1 m1{factory_default{}};
    machine2 m2{factory_e2{}};
    machine3 m3{factory_e3{}};
    machine4 m4{factory_e4{}};
    machine5 m5{factory_e5{}};
  } ret{};
  return ret;
}
constexpr void connect(auto& m1, auto& m2, auto& m3, auto& m4, auto& m5) {
  m1.connector.m2=&m2; m1.connector.m3=&m3;
  m2.connector.m1=&m1; m2.connector.m3=&m3;
  m3.connector.m1=&m1; m3.connector.m2=&m2;
  m4.connector.m5=&m5; m5.connector.m4=&m4;
}

