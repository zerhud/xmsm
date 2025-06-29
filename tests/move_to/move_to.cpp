/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "../factory.hpp"

template<auto v> using state = tests::state<v>;
template<auto v> using event = tests::event<v>;

struct factory : tests::factory {};
struct ts_with_queue {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<0>>(f, allow_move(f))
      , mk_qtrans<state<1>, state<2>, event<1>>(f)
      , mk_qtrans<state<3>, state<2>, event<2>>(f)
      , mk_qtrans<state<4>, state<3>, event<3>>(f)
      , mk_qtrans<state<5>, state<3>, event<4>>(f)
      , mk_qtrans<state<6>, state<5>, event<5>>(f, allow_move(f))
      , mk_trans<state<0>, state<100>, event<100>>(f)
      , mk_trans<state<1>, state<100>, event<101>>(f)
      , mk_trans<state<5>, state<100>, event<100>>(f)
      , mk_trans<state<1>, state<0>, event<0>>(f)
      , mk_trans<state<2>, state<0>, event<0>>(f)
      , mk_trans<state<2>, state<6>, event<6>>(f)
      , mk_trans<state<100>, state<0>, event<0>>(f)
    );
  }
};
struct ts_with_move_to {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f, move_to<ts_with_queue, state<100>, state<101>>(f), allow_move(f))
      , mk_qtrans<state<0>, state<2>, event<1>>(f, move_to<ts_with_queue, state<2>, state<100>>(f), allow_move(f))
      , mk_trans<state<2>, state<0>, event<0>>(f)
      , mk_trans<state<100>, state<0>, event<0>>(f)
    );
  }
};
struct ts_with_try_move_to {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<2>, event<0>>(f, try_move_to<ts_with_move_to, state<2>>(f))
      , mk_trans<state<2>, state<3>, event<1>>(f, try_move_to<ts_with_queue, state<100>>(f))
      , mk_trans<state<0>, state<10>, event<10>>(f, move_to<ts_with_move_to, state<2>, state<100>>(f))
    );
  }
};

using xm_ts_q = xmsm::scenario<factory, ts_with_queue>;
using xm_ts_m = xmsm::scenario<factory, ts_with_move_to>;
using xm_ts_t = xmsm::scenario<factory, ts_with_try_move_to>;

constexpr auto mk_s_queue(){struct{xm_ts_q q{factory{}}; xm_ts_m m{factory{}}; xm_ts_t t{factory{}};}ret; return ret;}
static_assert( [] {
  auto [s_q, s, s_t] = mk_s_queue();
  s_t.on(event<10>{}, s, s_q);
  return s.in_state<state<2>>() + 2*s_q.in_state<state<1>>() + 4*s_t.in_state<state<10>>();
}() == 7, "can handle chain of transitions" );
static_assert( [] {
  auto [s_q, s, s_t] = mk_s_queue();
  s_t.on(event<0>{}, s, s_q);
  return s.in_state<state<2>>() + 2*s_q.in_state<state<1>>() + 4*s_t.in_state<state<2>>();
}() == 7, "can handle chain of transitions" );
static_assert( [] {
  auto tracker = xmsm::mk_tracker<factory>(xmsm::basic_scenario<factory, ts_with_move_to>::all_trans_info(), xmsm::type_list<ts_with_queue>{});
  if (tracker.is_active()) throw __LINE__;
  auto [s_q, s, s_t] = mk_s_queue();
  tracker.update(&s, event<0>{}, s_q, s_t);
  if (tracker.is_active()) throw __LINE__;
  tracker.activate(get<0>(get<1>(s.all_trans_info())().mod_move_to), s_q, s, s_t);
  if (!tracker.is_active()) throw __LINE__;
  if (tracker.search<ts_with_queue, state<2>, state<100>>()->indexes[0]!=0) throw __LINE__;
  if (tracker.search<ts_with_queue, state<2>, state<100>>()->indexes[1]!=-1) throw __LINE__;
  tracker.update(&s, event<0>{}, s_q, s_t);
  if (tracker.search<ts_with_queue, state<2>, state<100>>()->indexes[0]!=0) throw __LINE__;
  if (tracker.search<ts_with_queue, state<2>, state<100>>()->indexes[1]!=-1) throw __LINE__;
  s_q.on(event<0>{});
  tracker.update(&s, event<0>{}, s_q, s_t);
  if (!tracker.is_active()) throw __LINE__;
  if (tracker.search<ts_with_queue, state<2>, state<100>>()->indexes[0]!=1) throw __LINE__;
  if (tracker.search<ts_with_queue, state<2>, state<100>>()->indexes[1]!=-1) throw __LINE__;
  s_q.on(event<1>{});
  tracker.update(&s, event<1>{}, s_q, s_t);
  if (tracker.search<ts_with_queue, state<2>, state<100>>()->indexes[0]!=-1) throw __LINE__;
  if (tracker.search<ts_with_queue, state<2>, state<100>>()->indexes[1]!=-1) throw __LINE__;
  if (tracker.is_active()) throw __LINE__;
  return true;
}() == true );
static_assert( [] {
  auto [s_q, s, s_t] = mk_s_queue();
  s.on(event<1>{}, s_q, s_t);
  if (!s_q.in_state<state<1>>()) throw __LINE__;
  s_q.on(event<101>{}, s, s_t);
  if (!s_q.in_state<state<100>>()) throw __LINE__;
  s.on_other_scenarios_changed(event<100>{}, s_q, s_t);
  return s.in_state<state<100>>();
}() );
static_assert( [] {
  auto [s_q, s, s_t] = mk_s_queue();
  s.move_to<state<100>>(event<100>{}, s_q, s_t);
  return s.in_state<state<0>>();
}() );
static_assert( [] {
  auto [s_q, s, s_t] = mk_s_queue();
  s.on(event<1>{}, s_q, s_t);
  s_q.on(event<101>{}, s, s_t); s.on_other_scenarios_changed(event<101>{}, s_q, s_t);
  s.on(event<0>{}, s_q, s_t);
  if (!s.in_state<state<0>>()) throw __LINE__;
  s_q.on(event<0>{}, s_q, s_t); s.on_other_scenarios_changed(event<0>{}, s_q, s_t);
  if (!s.in_state<state<0>>()) throw __LINE__;
  if (!s_q.in_state<state<0>>()) throw __LINE__;
  s_q.on(event<100>{}, s_q, s_t); s.on_other_scenarios_changed(event<0>{}, s_q, s_t);
  return s.in_state<state<0>>() + 2*s_q.in_state<state<100>>();
}() == 3, "deactivate queue after fail" );
static_assert( [] {
  auto [s_q, s, s_t] = mk_s_queue();
  s.on(event<1>{}, s_q, s_t); s.on_other_scenarios_changed(event<1>{}, s_q, s_t);
  s_q.on(event<0>{}, s, s_t); s.on_other_scenarios_changed(event<0>{}, s_q, s_t);
  return s.in_state<state<100>>();
}() == true, "cannot backward state" );
static_assert( [] {
  auto [s_q, s, s_t] = mk_s_queue();
  s_q.on(event<0>{}, s);
  auto ret = s_q.in_state<state<1>>();
  s.on(event<1>{}, s_q, s_t); s.on_other_scenarios_changed(event<1>{}, s_q, s_t);
  return ret + 2*s_q.in_state<state<1>>() + 4*s.in_state<state<2>>();
}() == 7, "move_to works if no allow_move transaction exists" );


struct ts_with_queue2 {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<0>>(f, allow_move(f))
      , mk_qtrans<state<1>, state<2>, event<1>>(f)
      , mk_qtrans<state<3>, state<2>, event<2>>(f, only_if(f, in<ts_with_queue2, state<0>>(f)), allow_move(f))
      , mk_trans<state<0>, state<3>, event<3>>(f)
      , mk_qtrans<state<1>, state<100>, event<101>>(f)
    );
  }
};
struct ts_combo {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<0>>(f, when(f, now_in<ts_with_queue, state<1>>(f)), move_to<ts_with_queue, state<2>, state<100>>(f))
      , mk_qtrans<state<0>, state<2>, event<1>>(f, move_to<ts_with_queue, state<100>, state<100>>(f))
      , mk_qtrans<state<0>, state<3>, event<2>>(f, move_to<ts_with_queue, state<2>, state<101>>(f), move_to<ts_with_queue2, state<2>, state<102>>(f))
      , mk_qtrans<state<0>, state<4>, event<3>>(f, move_to<ts_with_queue2, state<2>, state<102>>(f))
      , mk_trans<state<0>, state<5>, event<4>>(f, try_move_to<ts_with_queue2, state<2>>(f))
      , to_state_mods<state<100>>(f, stack_by_event<event<1100>>(f))
      , from_state_mods<state<1>>(f, stack_by_event<event<1101>>(f))
    );
  }
};
static_assert([] {
  auto [s_q, s_m, s_t] = mk_s_queue();
  xmsm::scenario<factory, ts_combo> s{factory{}};
  s_q.on(event<0>{});
  s.on_other_scenarios_changed(event<0>{}, s_q, s_m, s_t);
  if (!s.move_to_tracker.search<ts_with_queue, state<2>, state<100>>()->is_active()) throw __LINE__;
  auto ret = s.in_state<state<1>>() + 2*s_q.in_state<state<1>>();
  s_q.on(event<1>{});
  s.on_other_scenarios_changed(event<1>{}, s_q, s_m, s_t);
  return ret + 4*s_q.in_state<state<2>>() + 8*s.in_state<state<1>>();
}() == 15, "when -> move_to");
static_assert( [] {
  auto [s_q, s_m, s_t] = mk_s_queue();
  xmsm::scenario<factory, ts_combo> s{factory{}};
  s.on(event<1>{}, s_q, s_m, s_t);
  return s.stack_size();
}() == 1, "use to_state_mods for fail_state on move_to fail" );
static_assert( [] {
  auto [s_q, s_m, s_t] = mk_s_queue();
  xmsm::scenario<factory, ts_combo> s{factory{}};
  s.on(event<0>{}, s_q, s_m, s_t);
  if (!s.in_state<state<1>>()) throw __LINE__;
  s_q.on(event<101>{});
  s.on_other_scenarios_changed(event<101>{}, s_q, s_m, s_t);
  if (!s.in_state<state<100>>()) throw __LINE__;
  return s.stack_size();
}() == 1, "go to fail_state on wrong move and use modificators from from_state_mods for fail_state" );
static_assert( [] {
  auto [s_q, s_m, s_t] = mk_s_queue();
  xmsm::scenario<factory, ts_combo> s{factory{}};
  xmsm::scenario<factory, ts_with_queue2> s_q2{factory{}};
  s.on(event<2>{}, s_q, s_q2);
  return s.in_state<state<3>>() + 2*s_q.in_state<state<1>>() + 4*s_q2.in_state<state<1>>();
}() == 7, "few move_to works" );
static_assert( [] {
  auto [s_q, s_m, s_t] = mk_s_queue();
  xmsm::scenario<factory, ts_combo> s{factory{}};
  xmsm::scenario<factory, ts_with_queue2> s_q2{factory{}};
  s.on(event<2>{}, s_q, s_q2);
  s_q.on(event<101>{});
  s.on_other_scenarios_changed(s_q, s_q2);
  return s.in_state<state<101>>() + 2*s_q.in_state<state<100>>() + 4*s_q2.in_state<state<1>>();
}() == 7, "correct choice fail state by scenario" );
static_assert( [] {
  auto [s_q, s_m, s_t] = mk_s_queue();
  xmsm::scenario<factory, ts_combo> s{factory{}};
  xmsm::scenario<factory, ts_with_queue2> s_q2{factory{}};
  s.on(event<2>{}, s_q, s_q2);
  s_q2.on(event<101>{});
  s.on_other_scenarios_changed(s_q, s_q2);
  return s.in_state<state<102>>() + 2*s_q.in_state<state<1>>() + 4*s_q2.in_state<state<100>>();
}() == 7, "correct choice fail state by scenario" );
static_assert( [] {
  xmsm::scenario<factory, ts_combo> s{factory{}};
  xmsm::scenario<factory, ts_with_queue2> s_q2{factory{}};
  s_q2.on(event<3>{}, s);
  s.on(event<3>{}, s_q2);
  return s.in_state<state<102>>();
}() == true, "try move if allowed and fail if cannot" );
static_assert( [] {
  xmsm::scenario<factory, ts_with_queue2> s_q2{factory{}};
  xmsm::scenario<factory, ts_combo> s(factory{});
  s_q2.on(event<3>{});
  s.on(event<3>{}, s_q2);
  return !s.move_to_tracker.is_active() + 2*s.in_state<state<102>>();
}() == 3, "tracker is deactivated on fail" );
static_assert( [] {
  xmsm::scenario<factory, ts_with_queue2> s_q2{factory{}};
  xmsm::scenario<factory, ts_combo> s(factory{});
  s_q2.on(event<0>{}, s);
  if (!s_q2.in_state<state<1>>()) throw __LINE__;
  s.on(event<4>{}, s_q2);
  return s.in_state<state<5>>() + 2*s_q2.in_state<state<1>>();
}() == 3, "try_move_to works only if move allowed" );

int main(int,char**){
}
