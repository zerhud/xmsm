/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "factory.hpp"

using namespace std::literals;
template<auto v> using state = tests::state<v>;
template<auto v> using event = tests::event<v>;

struct event0 : event<0>{int val{};}; struct event1 : event<1>{}; struct event2 : event<2>{};
struct state0 : state<0>{int val{};}; struct state1 : state<1>{}; struct state2 : state<2>{}; struct state100 : state<100>{int val100{100};};

struct ts_with_queue {
  int base_val{};
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<0>>(f, allow_move(f))
      , mk_qtrans<state<1>, state<2>, event<1>>(f)
      , mk_qtrans<state<3>, state<2>, event<2>>(f)
      , mk_trans<state<0>, state<100>, event<101>>(f)
    );
  }
};
struct ts_with_move_to {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<10>>(f, move_to<ts_with_queue, state<2>, state<100>>(f))
    );
  }
};
struct ts_with_queue_user : ts_with_queue {int val{};};
struct ts_multi {
  static auto describe_sm(const auto& f) {
    return mk_multi_sm_description(f
      , mk_trans<state<0>, state<1>>(f, when(f, now_in<ts_with_queue, state<2>>(f)))
      , mk_qtrans<state<0>, state<2>, event<2>>(f)
      , mk_qtrans<state<2>, state<3>, event<3>>(f)
      , mk_trans<state<2>, state<1000>, event<10>>(f)
      , finish_state<state<1000>>(f), start_event<event<102>>(f)
    );
  }
};
struct ts_with_cond {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>>(f, when(f, now_in<ts_with_queue, state<2>>(f)))
      , mk_trans<state<0>, state<2>>(f, when(f, cnt_in<ts_multi, 1, state<3>, state<105>>(f)))
      , mk_trans<state<2>, state<3>>(f, when(f, cnt_in<ts_multi, 1, state<3>>(f) && !affected<ts_multi>(f)))
      , mk_trans<state<1>, state<0>, event<100>>(f)
    );
  }
};

struct factory : tests::factory {};
template<typename type> constexpr auto change_type(const factory&, const auto& adl) {
  if constexpr(std::is_same_v<type,state<1>>) return mk_change<state1>(adl);
  if constexpr(std::is_same_v<type,state<2>>) return mk_change<state2>(adl);
  if constexpr(std::is_same_v<type,state<100>>) return mk_change<state100>(adl);
  if constexpr(std::is_same_v<type,ts_with_queue>) return mk_change<ts_with_queue_user>(adl);
}

using machine = xmsm::machine<factory, ts_with_queue, ts_with_move_to, ts_with_cond, ts_multi>;

static_assert( [] {
  machine m{factory{}};
  m.on(event<102>{}); m.on(event<2>{});
  m.on(event<102>{});
  m.on(event<3>{});
  auto ret = m.in_state<ts_with_cond, state<2>>() + 2*(get<3>(m.scenarios).count_in<state<3>>()==1) + 4*(get<3>(m.scenarios).count()==2);
  m.on(event<102>{});
  ret += 8*m.in_state<ts_with_cond, state<3>>();
  return ret;
}() == 15 );

int main(int,char**) {
}