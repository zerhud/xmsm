/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "factory.hpp"

template<auto v> using state = tests::state<v>;
template<auto v> using event = tests::event<v>;

struct factory : tests::factory {};

struct ts_multi {
  static auto describe_sm(const auto& f) {
    return mk_multi_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<0>>(f)
      , mk_qtrans<state<1>, state<10>, event<11>>(f)
      , mk_qtrans<state<10>, state<11>, event<11>>(f)
      , mk_qtrans<state<11>, state<12>, event<11>>(f)
      , mk_qtrans<state<12>, state<13>, event<11>>(f)
      , mk_qtrans<state<13>, state<14>, event<11>>(f)
      , mk_qtrans<state<14>, state<15>, event<11>>(f)
      , mk_qtrans<state<15>, state<2>, event<1>>(f)
      , mk_trans<state<1>, state<3>, event<2>>(f)
      , mk_trans<state<11>, state<3>, event<2>>(f)
      , mk_trans<state<2>, state<101>, event<10>>(f)
      , mk_trans<state<13>, state<101>, event<10>>(f)
      , finish_state<state<7>>(f), finish_state<state<101>>(f), start_event<event<100>>(f)
      , mk_qtrans<state<0>, state<5>, event<5>>(f)
      , mk_qtrans<state<5>, state<6>, event<5>>(f)
      , mk_qtrans<state<6>, state<7>, event<5>>(f)
    );
  }
};
struct ts_move_multi {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f, move_to<ts_multi, state<2>, state<100>>(f))
      , mk_trans<state<0>, state<2>, event<1>>(f, move_to<ts_multi, state<7>, state<100>>(f))
      , mk_trans<state<100>, state<0>, event<0>>(f)
      , mk_trans<state<1>, state<0>, event<0>>(f)
      , mk_trans<state<2>, state<0>, event<0>>(f)
    );
  }
};
constexpr auto mk_s_multi() { struct { xmsm::scenario<factory, ts_multi> m{factory{}}; xmsm::scenario<factory, ts_move_multi> s{factory{}}; } ret; return ret; }
static_assert( [] {
  auto [s_m,s] = mk_s_multi();
  s.on(event<0>{}, s_m); return s.in_state<state<1>>(); }() == true, "we can move empty scenario" );
static_assert( [] {
  auto [s_m,s] = mk_s_multi();
  s_m.on(event<100>{});
  if (!s_m.find_scenario(1)->in_state<state<0>>()) throw __LINE__;
  s.on(event<0>{}, s_m);
  auto ret = s.in_state<state<1>>() + 2*s_m.find_scenario(1)->in_state<state<1>>();
  s_m.on(event<11>{}, s);
  s.on_other_scenarios_changed(event<11>{}, s_m);
  ret += 4*s.in_state<state<1>>() + 8*s_m.find_scenario(1)->in_state<state<10>>();
  return ret;
}() == 15, "move_to with multi scenario: correct way" );
static_assert( [] {
  auto [s_m,s] = mk_s_multi();
  s_m.on(event<100>{});
  s.on(event<0>{}, s_m);
  s_m.on(event<2>{}, s);
  s.on_other_scenarios_changed(event<2>{}, s_m);
  return s.in_state<state<100>>() + 2*s_m.find_scenario(1)->in_state<state<3>>();
}() == 3, "move_to with multi scenario: wrong way" );
static_assert( [] {
  auto [s_m,s] = mk_s_multi();
  s_m.on(event<100>{}); s_m.on(event<0>{}, s_m); s_m.on(event<11>{}, s_m);
  s_m.on(event<100>{}, s);
  s.on(event<0>{}, s_m);
  s_m.on(event<11>{}); s.on_other_scenarios_changed(event<11>{}, s_m);
  auto ret = s.in_state<state<1>>() + 2*s_m.find_scenario(2)->in_state<state<10>>() + 4*s_m.find_scenario(1)->in_state<state<12>>();
  s_m.on(event<11>{}); s.on_other_scenarios_changed(event<11>{}, s_m);
  ret += 8*s.in_state<state<1>>() + 16*s_m.find_scenario(2)->in_state<state<11>>() + 32*s_m.find_scenario(1)->in_state<state<13>>();
  s_m.on(event<2>{}); s.on_other_scenarios_changed(event<2>{}, s_m);
  if (!s.in_state<state<100>>()) throw __LINE__;
  return ret;
}() == 63 );
static_assert( [] {
  auto [s_m,s] = mk_s_multi();
  s_m.on(event<100>{}); s_m.on(event<0>{}, s_m); s_m.on(event<11>{}, s_m);
  s_m.on(event<100>{}, s);
  s.on(event<0>{}, s_m);
  s_m.on(event<11>{}); s.on_other_scenarios_changed(event<11>{}, s_m);
  s_m.on(event<11>{}); s.on_other_scenarios_changed(event<11>{}, s_m);
  s_m.on(event<2>{}); s.on_other_scenarios_changed(event<2>{}, s_m);
  s_m.on(event<10>{}); s.on_other_scenarios_changed(event<10>{}, s_m);
  return s.in_state<state<100>>();
}() == true, "fail if some scenario finished" );
static_assert( [] {
  auto [s_m,s] = mk_s_multi();
  s_m.on(event<100>{});
  s.on(event<1>{}, s_m);
  s_m.on(event<5>{}); s.on_other_scenarios_changed(event<5>{}, s_m);
  s_m.on(event<5>{}); s.on_other_scenarios_changed(event<5>{}, s_m);
  auto ret = s.in_state<state<2>>() + 2*(s_m.count()==0);
  s_m.on(event<100>{}); s.on_other_scenarios_changed(event<5>{}, s_m);
  return ret + 4*s.in_state<state<2>>();
}() == 7, "deactivate if target state is finish state" );
static_assert( [] {
  auto [s_m,s] = mk_s_multi();
  s_m.on(event<100>{}); s.on(event<0>{}, s_m);
  s_m.on(event<100>{}); s.on_other_scenarios_changed(event<100>{}, s_m);
  return s.in_state<state<100>>();
}() == true, "fail if scenario added to multi scenario" );
static_assert( [] {
  auto [s_m,s] = mk_s_multi();
  s_m.on(event<100>{}); s.on(event<0>{}, s_m);
  s_m.on(event<100>{}); s.on_other_scenarios_changed(event<100>{}, s_m);
  if (!s.in_state<state<100>>()) throw __LINE__;
  s.on(event<0>{}, s_m);
  s_m.on(event<100>{}); s.on_other_scenarios_changed(event<100>{}, s_m);
  return s.in_state<state<0>>();
}() == true, "deactivate after fail" );
static_assert( [] {
  auto [s_m,s] = mk_s_multi();
  s_m.on(event<100>{});
  s_m.on(event<5>{}); s.on_other_scenarios_changed(event<5>{}, s_m);
  s_m.on(event<100>{});
  s.on(event<1>{}, s_m);
  if (!s.in_state<state<2>>()) throw __LINE__;
  s_m.on(event<5>{}); s.on_other_scenarios_changed(event<5>{}, s_m);
  auto ret = s.in_state<state<2>>() + 2*(s_m.count()==1);
  s_m.on(event<5>{}); s.on_other_scenarios_changed(event<5>{}, s_m);
  return ret + 4*s.in_state<state<2>>() + 8*(s_m.count()==0);
}() == 15, "one of scenario can finish if target state is a finish state" );

int main(int,char**) {
}