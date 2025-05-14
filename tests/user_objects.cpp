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

struct event0 : event<0>{int val{};}; struct event1 : event<1>{}; struct event2 : event<2>{}; struct event3 : event<3>{}; struct event4 : event<4>{};
struct state0 : state<0>{int val{};}; struct state1 : state<1>{}; struct state2 : state<2>{}; struct state3 : state<3>{}; struct state4 : state<4>{}; struct state100 : state<100>{int val100{100};};
struct factory : tests::factory {};
template<typename type> constexpr auto change_type(const factory&, const auto& adl) {
  if constexpr(std::is_same_v<type,state<1>>) return mk_change<state1>(adl);
  if constexpr(std::is_same_v<type,state<2>>) return mk_change<state2>(adl);
  if constexpr(std::is_same_v<type,state<100>>) return mk_change<state100>(adl);
}

struct ts_with_queue {
  int base_val{};
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<0>>(f, allow_move(f))
      , mk_qtrans<state<1>, state<2>, event<1>>(f)
      , mk_qtrans<state<3>, state<2>, event<2>>(f)
      , mk_qtrans<state<4>, state<3>, event<3>>(f)
      , mk_qtrans<state<5>, state<3>, event<4>>(f)
      , mk_qtrans<state<6>, state<5>, event<5>>(f)
      , mk_trans<state<0>, state<100>, event<100>>(f)
      , mk_trans<state<1>, state<100>, event<101>>(f)
    );
  }
};
struct ts_with_move_to {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<0>>(f, move_to<ts_with_queue, state<2>, state<100>>(f))
    );
  }
};
struct ts_with_queue_user : ts_with_queue {int val{};};
struct ts_multi {
  static auto describe_sm(const auto& f) {
    return mk_multi_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f, when(f, now_in<ts_with_queue, state<2>>(f)))
      , mk_qtrans<state<1>, state<2>>(f)
      , mk_trans<state<2>, state<1000>, event<10>>(f)
      , finish_state<state<1000>>(f), start_event<event<100>>(f)
    );
  }
};
struct ts_multi_user : ts_multi { constexpr explicit ts_multi_user(int,int){} int val1{1}, val2{2}; };
template<typename type> constexpr auto create(const factory& f) requires std::is_same_v<type,ts_multi_user> { return ts_multi_user(0,0); }
constexpr auto mk_sc() {
  struct {
    xmsm::scenario<factory, ts_with_queue, ts_with_queue_user> q{factory{}};
    xmsm::scenario<factory, ts_with_move_to> m{factory{}};
  } ret;
  return ret;
}
constexpr void on_enter(ts_with_queue_user& s, state<1>&, const event0& e) { s.val = e.val; }
constexpr void on_enter(ts_with_queue_user& s, state2&, const event<1>&) { s.base_val = 11; }
static_assert( [] {
  auto [q,m] = mk_sc();
  m.on(event0{},q); return m.in_state<state<1>>() + 2*m.in_state<state1>() + 4*q.in_state<state1>();
}() == 7, "event and state can to be replaced and used together" );
static_assert( [] {
  auto [q,m] = mk_sc();
  q.on(event0{{}, 7});
  if (!q.in_state<state1>()) throw __LINE__;
  q.on(event1{});
  if (!q.in_state<state2>()) throw __LINE__;
  if (!q.in_state<state<2>>()) throw __LINE__;
  return (q.obj.val==7) + 2*(q.obj.base_val==11);
}() == 3, "the on_enter is called" );
static_assert( [] {
  auto [q,m] = mk_sc();
  m.on(event0{}, q);
  m.on_other_scenarios_changed(q);
  if (!q.in_state<state1>()) throw __LINE__;
  if (!q.in_state<state<1>>()) throw __LINE__;
  q.on(event<101>{});
  q.on_other_scenarios_changed(m);
  if (!q.in_state<state100>()) throw __LINE__;
  if (!q.in_state<state<100>>()) throw __LINE__;
  return visit([](auto&v){if constexpr(requires{v.val100;})return v.val100;else return 0;}, q.cur_state());
}() == 100, "the replaced type used as fail state" );

static_assert( [] {
  xmsm::scenario<factory, ts_multi, ts_multi_user> s{factory{}};
  s.on(event<100>{});
  return (s.find(1)->val1==1) + 2*(s.find(1)->val2==2);
}() == 3 );

struct ts_tag {
  constexpr static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>>(f, allow_move(f))
      , mk_trans<state<0>, state<2>, event<2>>(f)
      , mk_trans<state<0>, state<2>, event<2>>(f)
      , mk_trans<state<0>, state<3>, event<3>>(f)
    );
  }
};

struct tag1 {}; struct tag2 {};
struct ts_tag1_1 : ts_tag, tag1 {};
struct ts_tag1_2 : ts_tag, tag1 {};
struct ts_tag1_3 : ts_tag, tag1 {};
struct ts_tag2_1 : ts_tag, tag2 {};
struct ts_tag2_2 : ts_tag, tag2 {};
struct ts_tag2_m : tag2 {
  constexpr static auto describe_sm(const auto& f) {
    return mk_multi_sm_description(f
      , mk_trans<state<0>, state<1>, event<701>>(f)
      , mk_trans<state<1>, state<2>, event<702>>(f)
      , finish_state<state<2>>(f), start_event<event<700>>(f)
    );
  }
};
struct ts_tag_move {
  constexpr static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>, event<1>>(f, move_to<tag1, state<1>, state<100>>(f))
      , mk_trans<state<0>, state<3>>(f, when(f, (in<tag2, state<3>>(f) && affected<tag2>(f)) || cnt_in<tag2, 1, state<1>>(f)))
    );
  }
};

using mtag = xmsm::machine<factory, ts_tag1_1, ts_tag1_2, ts_tag1_3, ts_tag2_1, ts_tag2_2, ts_tag_move, ts_tag2_m>;
static_assert( [] {
  mtag m{factory{}};
  m.on(event<1>{});
  constexpr bool tracking_match = decltype(get<ts_tag_move>(m.scenarios).move_to_tracker)::debug_tracking_scenarios() == xmsm::type_list<ts_tag1_1, ts_tag1_2, ts_tag1_3>{};
  return m.in_state<ts_tag1_1, state<1>>() + 2*tracking_match + 4*m.in_state<ts_tag1_2, state<1>>() + 8*m.in_state<ts_tag2_1, state<0>>();
}() == 15, "can move few scenarios using tag (base class)" );
static_assert( [] {
  mtag m{factory{}};
  m.on(event<2>{});
  m.on(event<1>{});
  return m.in_state<ts_tag1_1, state<2>>() + 2*m.in_state<ts_tag_move, state<100>>() + 4*m.in_state<ts_tag2_1, state<2>>();
}() == 7, "if fail moving one of scenarios by tag the fail state is active" );
static_assert( [] {
  mtag m{factory{}}; m.on(event<3>{});
  return m.in_state<ts_tag_move, state<3>>();
}(), "condition for single scenarios works with tags" );
static_assert( [] {
  mtag m{factory{}}; m.on(event<700>{}); m.on(event<701>{});
  return m.in_state<ts_tag_move, state<3>>();
}(), "condition for multi scenarios works with tags" );
static_assert( [] {
  mtag m{factory{}}; return m.in_state_count<tag1, state<0>>();
}() == 3 );

int main(int,char**){
}