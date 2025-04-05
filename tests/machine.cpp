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
struct ts_with_queue {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<0>>(f)
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
      , mk_trans<state<0>, state<1>, event<0>>(f, move_to<ts_with_queue, state<100>, state<101>>(f))
      , mk_qtrans<state<0>, state<2>, event<1>>(f, move_to<ts_with_queue, state<2>, state<100>>(f))
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
using xm_ts_t = xmsm::scenario<factory,  ts_with_try_move_to>;

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
  auto tracker = xmsm::mk_tracker<factory,ts_with_queue>(xmsm::basic_scenario<factory, ts_with_move_to>::all_trans_info());
  if (tracker.is_active()) throw __LINE__;
  auto [s_q, s, s_t] = mk_s_queue();
  tracker.update(&s, event<0>{}, s_q, s_t);
  if (tracker.is_active()) throw __LINE__;
  tracker.activate(get<0>(get<1>(s.all_trans_info())().mod_move_to), s_q, s, s_t);
  if (!tracker.is_active()) throw __LINE__;
  if (tracker.search<ts_with_queue, state<2>>()->indexes[0]!=0) throw __LINE__;
  if (tracker.search<ts_with_queue, state<2>>()->indexes[1]!=-1) throw __LINE__;
  tracker.update(&s, event<0>{}, s_q, s_t);
  if (tracker.search<ts_with_queue, state<2>>()->indexes[0]!=0) throw __LINE__;
  if (tracker.search<ts_with_queue, state<2>>()->indexes[1]!=-1) throw __LINE__;
  s_q.on(event<0>{});
  tracker.update(&s, event<0>{}, s_q, s_t);
  if (!tracker.is_active()) throw __LINE__;
  if (tracker.search<ts_with_queue, state<2>>()->indexes[0]!=1) throw __LINE__;
  if (tracker.search<ts_with_queue, state<2>>()->indexes[1]!=-1) throw __LINE__;
  s_q.on(event<1>{});
  tracker.update(&s, event<1>{}, s_q, s_t);
  if (tracker.search<ts_with_queue, state<2>>()->indexes[0]!=-1) throw __LINE__;
  if (tracker.search<ts_with_queue, state<2>>()->indexes[1]!=-1) throw __LINE__;
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

int main(int,char**){
}