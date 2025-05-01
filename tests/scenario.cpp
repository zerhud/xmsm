/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include <map>
#include <string>
#include <stdexcept>
#include "factory.hpp"

template<auto v> using state = tests::state<v>;
template<auto v> using event = tests::event<v>;

struct factory : tests::factory {};
struct ts1 {
  int val{0};
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f)
      , mk_trans<state<1>, state<0>, event<1>>(f)
      , nothing(f)
    );
  }
};
struct ts1_def1 {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f)
      , mk_trans<state<1>, state<0>, event<1>>(f)
      , pick_def_state<state<1>>(f)
    );
  }
};

constexpr void on_exit(ts1& s, state<0>&, const event<0>& e) { if (s.val++!=0) throw s.val; }
constexpr void on_enter(ts1& s, state<1>&) { if (s.val++!=1) throw s.val; }

static_assert( [] {
  xmsm::scenario<factory, ts1> s{factory{}};
  return (s.states_count()==2) + 2*(s.events_count()==2) + 4*(s.index()==0)
  + 8*s.in_state<state<0>>() + 16*!s.is_stack_with_event_required()
  + 32*(s.cur_state_hash()==hash(xmsm::type_c<state<0>>));
}() == 63);
static_assert( [] {
  xmsm::scenario<factory, ts1> s{factory{}};
  s.on(event<0>{});
  return (s.states_count()==2) + 2*(s.events_count()==2) + 4*(s.index()==1) + 8*s.in_state<state<1>>() + 16*(s.obj.val==2);
}() == 31 );
static_assert( [] {
  xmsm::scenario<factory, ts1_def1> s{factory{}};
  const auto after_init = s.in_state<state<1>>();
  s.on(event<1>{});
  return after_init + 2*s.in_state<state<0>>() + 4*(s.states_count()==2);
}() == 7 );
static_assert( [] {
  xmsm::scenario<factory, ts1> s{factory{}};
  if (s.own_state()!=xmsm::scenario_state::ready) throw __LINE__;
  s.on(event<0>{});
  if (s.own_state()!=xmsm::scenario_state::fired) throw __LINE__;
  s.reset_own_state();
  return s.own_state() == xmsm::scenario_state::ready;
}() );

struct ts_with_stack {
  int val{0};
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f, stack_by_event<event<10>>(f))
      , mk_trans<state<1>, state<0>, event<1>>(f, stack_by_event<event<11>, event<12>>(f))
    );
  }
};
constexpr void on_exit(ts_with_stack& s, state<0>&) { ++s.val; }
constexpr void on_enter(ts_with_stack&, state<0>& s) { ++s.rt_val; }
static_assert( xmsm::scenario<factory, ts_with_stack>{factory{}}.is_stack_with_event_required() );
static_assert( [] {
  xmsm::scenario<factory, ts_with_stack> s{factory{}};
  get<state<0>>(s.cur_state()).rt_val = 100;
  s.on(event<0>{});
  const auto s1 = s.stack_size();
  if (!s.in_state<state<1>>()) throw __LINE__;
  s.on(event<1>{});
  const auto s2 = s.stack_size();
  if (!s.in_state<state<0>>()) throw __LINE__;
  if (get<state<0>>(s.cur_state()).rt_val != 1) throw __LINE__;
  s.on(event<10>{});
  const auto s3 = s.stack_size();
  if (s.obj.val != 1) throw __LINE__;
  s.on(event<12>{});
  if (!s.in_state<state<0>>()) throw __LINE__;
  if (get<state<0>>(s.cur_state()).rt_val != 101) throw __LINE__;
  return (s1==1) + 2*(s2==2) + 4*(s3==1) + 8*(s.stack_size()==0) + 16*(s.events_count()==5) + 32*(s.obj.val==2);
}() == 63 );

static_assert( [] {
  using s_type = xmsm::scenario<factory, ts_with_stack>;
  s_type s{factory{}};
  xmsm::scenario<factory, ts1> s2{factory{}};
  auto in0 = in<ts_with_stack, state<0>>(s);
  auto in1 = in<ts_with_stack, state<1>>(s);
  return in0(s) + 2*!in1(s) + 4*((in1 || in0)(s)) + 8*!in0(s2);
}() == 15 );

struct ts_with_expr_stack {
  int val{};
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f, stack_by_event<event<10>>(f), stack_by_expr(f, in<ts1, state<0>>(f) && in<ts1_def1, state<0>,state<1>>(f)))
      , mk_trans<state<1>, state<2>, event<1>>(f, stack_by_event<event<11>, event<12>>(f), stack_by_expr(f, in<ts1, state<1>>(f)))
      , mk_trans<state<1>, state<3>, event<2>>(f)
      , mk_trans<state<2>, state<4>, event<3>>(f, stack_by_expr(f, in<ts1, state<1>>(f)))
    );
  }
};
constexpr void on_enter(const auto& f, ts_with_expr_stack& s, state<2>&) { static_assert(std::is_same_v<std::decay_t<decltype(f)>, factory>); s.val = 3; }
constexpr void on_exit(ts_with_expr_stack& s, state<2>&) { s.val = 21 / (s.val / (s.val==3)); }
static_assert( [] {
  xmsm::scenario<factory, ts1> s_ts1{factory{}};
  xmsm::scenario<factory, ts1_def1> s_ts1_def1{factory{}};
  xmsm::scenario<factory, ts_with_expr_stack> s{factory{}};
  s.on(event<0>{});
  if (s.stack_size()!=1) throw __LINE__;
  s.on(event<1>{});
  if (s.stack_size()!=2) throw __LINE__;
  event<1> e;
  s.on_other_scenarios_changed(e, s_ts1);
  if (s.stack_size()!=2) throw __LINE__;
  s.on_other_scenarios_changed(e, s_ts1, s_ts1_def1);
  if (s.stack_size()!=1) throw __LINE__;
  s_ts1.on(event<0>{});
  s.on_other_scenarios_changed(e, s_ts1);
  return s.obj.val;
}() == 7 );
static_assert( [] {
  xmsm::scenario<factory, ts_with_expr_stack> s{factory{}};
  s.on(event<0>{});
  s.on(event<1>{});
  s.on(event<3>{});
  return s.stack_size();
}() == 3 );

struct ts_with_on {
  int val{};
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>>(f, when(f, in<ts1, state<1>>(f)), stack_by_event<event<10>>(f))
      , mk_trans<state<1>, state<2>>(f, when(f, in<ts1_def1, state<1>>(f)))
      , mk_trans<state<2>, state<3>>(f, when(f, now_in<ts1, state<1>>(f)), stack_by_event<event<11>>(f))
    );
  }
};
static_assert( [] {
  xmsm::scenario<factory, ts1> s_ts1{factory{}};
  xmsm::scenario<factory, ts1_def1> s_ts1_def1{factory{}};
  xmsm::scenario<factory, ts_with_on> s{factory{}};
  s.on_other_scenarios_changed(event<0>{}, s_ts1, s_ts1_def1);
  if (!s.in_state<state<0>>()) throw __LINE__;
  s_ts1_def1.on(event<0>{});
  s.on_other_scenarios_changed(event<0>{}, s_ts1, s_ts1_def1);
  if (!s.in_state<state<0>>()) throw __LINE__;
  if (!s_ts1_def1.in_state<state<1>>()) throw __LINE__;
  s_ts1.on(event<0>{});
  s.on_other_scenarios_changed(event<0>{}, s_ts1, s_ts1_def1);
  auto ret = s.in_state<state<3>>() + 2*(s.stack_size()==2);
  s_ts1.reset_own_state();
  s.on(event<11>{});
  s.on_other_scenarios_changed(event<0>{}, s_ts1, s_ts1_def1);
  ret += 4*s.in_state<state<2>>() + 8*(s.stack_size()==1);
  return ret;
}() == 15 );

struct ts_with_if {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f, only_if(f, in<ts1, state<1>>(f)))
      , mk_trans<state<0>, state<2>, event<0>>(f)
      , mk_trans<state<2>, state<0>, event<10>>(f)
    );
  }
};
static_assert( [] {
  xmsm::scenario<factory, ts1> s_ts1{factory{}};
  xmsm::scenario<factory, ts_with_if> s{factory{}};
  s.on(event<0>{});
  if (!s.in_state<state<2>>()) throw __LINE__;
  s_ts1.on(event<0>{});
  if (!s_ts1.in_state<state<1>>()) throw __LINE__;
  s.on(event<10>{});
  s.on(event<0>{}, s_ts1);
  return s.in_state<state<1>>();
}(), "the transition won't happen if only_if condition evaluates to false" );

struct ts_with_state_mods {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f)
      , mk_trans<state<1>, state<2>, event<0>>(f)
      , mk_trans<state<2>, state<0>, event<10>>(f)
      , to_state_mods<state<1>>(f, stack_by_event<event<11>>(f))
      , from_state_mods<state<2>>(f, only_if(f, in<ts1, state<1>>(f)))
    );
  }
};
static_assert( [] {
  xmsm::scenario<factory, ts1> s_ts1{factory{}};
  xmsm::scenario<factory, ts_with_state_mods> s{factory{}};
  s.on(event<0>{});
  if (s.stack_size()!=1) throw __LINE__;
  s.on(event<0>{});
  if (!s.in_state<state<2>>()) throw __LINE__;
  s.on(event<10>{});
  if (!s.in_state<state<2>>()) throw __LINE__;
  s_ts1.on(event<0>{});
  s.on(event<10>{}, s_ts1);
  if (!s.in_state<state<0>>()) throw __LINE__;
  return s.stack_size();
}() == 1 );

struct multi_ts {
  static auto describe_sm(const auto& f) {
    return mk_multi_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f)
      , mk_trans<state<1>, state<2>>(f)
      , mk_trans<state<1>, state<1000>, event<101>>(f)
      , mk_trans<state<2>, state<0>, event<10>>(f)
      , to_state_mods<state<2>>(f, when(f, in<ts1, state<1>>(f)))
      , finish_state<state<1000>>(f), start_event<event<100>>(f)
    );
  }
};
static_assert( [] { xmsm::scenario<factory, multi_ts> s{factory{}}; return s.is_multi() + 2*s.empty() + 4*(s.count()==0); }() == 7 );
static_assert( [] {
  xmsm::scenario<factory, multi_ts> s{factory{}};
  xmsm::scenario<factory, ts1> s_ts1{factory{}};
  if (s.own_state() != xmsm::scenario_state::ready) throw __LINE__;
  s.on(event<100>{});
  if (s.own_state() != xmsm::scenario_state::fired) throw __LINE__;
  if (s.count()!=1) throw __LINE__;
  s.on(event<100>{});
  if (s.count()!=2) throw __LINE__;
  if (s.own_state() != xmsm::scenario_state::fired) throw __LINE__;
  s.reset_own_state();
  if (s.own_state() != xmsm::scenario_state::ready) throw __LINE__;
  s.on(event<0>{});
  if (s.count()!=2) throw __LINE__;
  if (s.count_in<state<0>>()!=0) throw __LINE__;
  if (s.count_in<state<1>>()!=2) throw __LINE__;
  s.on_address(event<101>{}, s.scenarios[0].id);
  if (s.count()!=1) throw __LINE__;
  s_ts1.on(event<0>{});
  s.on_other_scenarios_changed(event<200>{}, s_ts1);
  return (s.count()==1) + 2*(s.count_in<state<2>>()==1);
}() == 3 );

struct ts_with_queue {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<0>>(f, allow_move(f))
      , mk_qtrans<state<1>, state<2>, event<1>>(f)
      , mk_qtrans<state<3>, state<2>, event<2>>(f)
      , mk_qtrans<state<4>, state<3>, event<3>>(f)
      , mk_qtrans<state<5>, state<3>, event<4>>(f)
      , mk_qtrans<state<6>, state<5>, event<5>>(f)
      , mk_trans<state<0>, state<100>, event<100>>(f)
    );
  }
};
struct ts_with_move_to {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f, move_to<ts_with_queue, state<100>, state<101>>(f))
      , mk_qtrans<state<0>, state<2>, event<1>>(f, move_to<ts_with_queue, state<2>, state<100>>(f), allow_move(f))
    );
  }
};
struct ts_with_move_to_deep {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<2>, event<0>>(f, move_to<ts_with_move_to, state<2>, state<100>>(f))
    );
  }
};
constexpr auto mk_s_queue() {struct{xmsm::scenario<factory, ts_with_queue> q{factory{}}; xmsm::scenario<factory, ts_with_move_to> m{factory{}};}ret; return ret; }
static_assert( mk_allow_queue_st(xmsm::scenario<factory, ts_with_queue>{factory{}}.all_trans_info(), xmsm::type_c<state<2>>) == xmsm::type_list<
  xmsm::type_list<tests::state<0>, tests::state<1>, tests::state<2> >,
  xmsm::type_list<tests::state<4>, tests::state<3>, tests::state<2> >,
  xmsm::type_list<tests::state<6>, tests::state<5>, tests::state<3>, tests::state<2> > >{} );
static_assert( [] { xmsm::scenario<factory, ts_with_move_to> s{factory{}}; s.on(event<1>{}); return s.in_state<state<100>>(); }(), "fail if scenario are not present" );
static_assert( [] {
  auto [s_q, s] = mk_s_queue();
  s.on(event<0>{}, s_q);
  return s.in_state<state<101>>() + 2*s_q.in_state<state<0>>();
}() == 3, "fail if scenario didn't allow such transition");
static_assert( [] {
  auto [s_q, s] = mk_s_queue();
  s_q.on(event<0>{}); s.on(event<1>{}, s_q);
  return s.in_state<state<2>>() + 2*s_q.in_state<state<1>>();
}() == 3, "can handle single transition" );
static_assert( [] {
  auto [s_q, s] = mk_s_queue();
  s.on(event<1>{}, s_q);
  return s.in_state<state<2>>() + 2*s_q.in_state<state<1>>();
}() == 3, "can handle few transitions" );
static_assert( [] {
  auto [s_q, s] = mk_s_queue();
  xmsm::scenario<factory, ts_with_move_to_deep> s_t{factory{}};
  s_t.on(event<0>{}, s, s_q);
  return s.in_state<state<2>>() + 2*s_q.in_state<state<1>>() + 4*s_t.in_state<state<2>>();
}() == 7, "can handle chain of transitions" );

struct rt_factory : factory {};
template<typename key, typename value> auto mk_map(const rt_factory&){ return std::map<key,value>{}; }
void erase_if(const rt_factory&, auto& con, auto&& fnc) {
  for (auto pos=con.begin();pos!=con.end();) {
    if (fnc(*pos)) pos = con.erase(pos);
    else ++pos;
  }
}
int main(int,char**) {
  {
    xmsm::scenario<rt_factory, multi_ts> s{rt_factory{}};
    if (s.own_state() != xmsm::scenario_state::ready) throw std::runtime_error(std::to_string(__LINE__));
    s.on(event<100>{});
    if (s.own_state() != xmsm::scenario_state::fired) throw std::runtime_error(std::to_string(__LINE__));
    if (s.count()!=1) throw std::runtime_error(std::to_string(__LINE__));
    if (!s.find_scenario(1)->in_state<state<0>>()) throw std::runtime_error(std::to_string(__LINE__));
    s.on(event<0>{});
    if (!s.find_scenario(1)->in_state<state<1>>()) throw std::runtime_error(std::to_string(__LINE__));
    s.on(event<101>{});
    if (s.count()!=0) throw std::runtime_error(std::to_string(__LINE__));
  }
}
