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
struct ts1 {
  int val{0};
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>, event<0>>(f)
      , mk_trans<state<1>, state<0>, event<1>>(f)
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
  return (s.states_count()==2) + 2*(s.events_count()==2) + 4*(s.index()==0) + 8*s.in_state<state<0>>() + 16*!s.is_stack_with_event_required();
}() == 31);
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
  if (get<state<0>>(s.cur_state()).rt_val != 0) throw __LINE__;
  s.on(event<10>{});
  const auto s3 = s.stack_size();
  if (s.obj.val != 1) throw __LINE__;
  s.on(event<12>{});
  if (!s.in_state<state<0>>()) throw __LINE__;
  if (get<state<0>>(s.cur_state()).rt_val != 101) throw __LINE__;
  return (s1==1) + 2*(s2==2) + 4*(s3==1) + 8*(s.stack_size()==0) + 16*(s.events_count()==5) + 32*(s.obj.val==2);
}() == 63 );

int main(int,char**) {
}