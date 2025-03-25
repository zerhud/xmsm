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

constexpr void on_exit(ts1& s, state<0>&, const event<0>& e) {
  if (s.val++!=0) throw s.val;
}
constexpr void on_enter(ts1& s, state<1>&) {
  if (s.val++!=1) throw s.val;
}

static_assert( [] {
  xmsm::scenario<factory, ts1> s{factory{}};
  return (s.states_count()==2) + 2*(s.events_count()==2) + 4*(s.index()==0) + 8*s.in_state<state<0>>();
}() == 15);
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

int main(int,char**) {
}