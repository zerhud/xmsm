/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include <source_location>
#include <stdexcept>
#include <iostream>
#include "factory.hpp"

struct test_exception : std::runtime_error {
  std::string orig_what;
  constexpr explicit test_exception(std::string what, std::string fn, int line) : std::runtime_error(what + ":\n\t" + fn + ':' + std::to_string(line)), orig_what(std::move(what)) {}
};
void error(std::string what="", std::source_location l = std::source_location::current()) {
  throw test_exception(what, l.file_name(), l.line());
}

using namespace std::literals;
template<auto v> using state = tests::state<v>;
template<auto v> using event = tests::event<v>;

struct factory : tests::rt_factory {};
struct state5_with_ex : state<5> {
  state5_with_ex() { error("ex in ctor state5"); }
};
template<typename type> constexpr auto change_type(const factory&, const auto& adl) {
  if constexpr(std::is_same_v<type,state<5>>) return mk_change<state5_with_ex>(adl);
}
struct ts_simple {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>, event<1>>(f)
      , mk_trans<state<1>, state<2>, event<1>>(f)
      , mk_trans<state<1>, state<3>>(f, when(f, broken<ts_simple>(f)))
      , mk_trans<state<0>, state<5>, event<5>>(f)
    );
  }
};
struct ts_queue {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<5>, event<5>>(f, allow_move(f))
    );
  }
};
struct ts_move {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<state<0>, state<1>, event<1>>(f, move_to<ts_queue, state<5>, state<100>>(f))
    );
  }
};

struct event_error_to_1_in_simple : event<1> {};
constexpr auto on_enter(const ts_simple&, const state<1>&, const event_error_to_1_in_simple&) { error("error on enter to 1 by event_error_to_1_in_simple"); }
constexpr auto on_exit(const ts_simple&, const state<1>&, const event_error_to_1_in_simple&) { error("error on exit from 1 by event_error_to_1_in_simple"); }

int main(int,char**) {
  {
    xmsm::machine<factory, ts_simple> m{factory{}};
    if (m.is_broken<ts_simple>()) error("before exception the state is not broken");
    try {m.on(event_error_to_1_in_simple{}); }
    catch (const test_exception& ex) { if (ex.orig_what!="error on enter to 1 by event_error_to_1_in_simple"sv) throw; }
    if (!m.in_state<ts_simple, state<1>>()) error("on_enter cannot cancel transition");
    if (!m.is_broken<ts_simple>()) error("after exception the state is broken");
    m.try_to_repair(event<1>{});
    if (!m.in_state<ts_simple, state<3>>()) error("after continue the \"broken\" condition is active");
    if (m.is_broken<ts_simple>()) error("after continue state is not broken");
  }
  {
    xmsm::machine<factory, ts_simple> m{factory{}};
    try { m.on(event<1>{}); m.on(event_error_to_1_in_simple{}); }
    catch (const test_exception& ex) { if (ex.orig_what!="error on exit from 1 by event_error_to_1_in_simple"sv) throw; }
    if (m.in_state<ts_simple, state<2>>()) error("on_exit cancels transition");
    if (!m.is_broken<ts_simple>()) error("after exception the state is broken");
  }
  {
    xmsm::machine<factory, ts_simple> m{factory{}};
    try{ m.on(event<5>{}); }
    catch (const test_exception& ex) { if (ex.orig_what!="ex in ctor state5"sv) throw; }
    if (!m.in_state<ts_simple, state<0>>()) error("state don't changed if ex in state ctor");
    if (!m.is_broken<ts_simple>()) error("after exception the state is broken");
    m.try_to_repair(event<5>{});
    if (!m.in_state<ts_simple, state<0>>()) error("no out from 0 state");
    if (m.is_broken<ts_simple>()) error("after continue state is not broken");
  }
  {
    xmsm::machine<factory, ts_queue, ts_move> m{factory{}};
    bool ex_was = false;
    try { m.on(event<1>{}); }
    catch (const test_exception& ex) { if (ex.orig_what!="ex in ctor state5"sv) throw; ex_was = true; }
    if (!ex_was) error("exception wasn't thrown as expected");
    if (!m.in_state<ts_queue, state<0>>()) error("the state wasn't changed");
    if (!m.in_state<ts_move, state<0>>()) error("the event sequence was terminated");
    if (!m.is_broken<ts_queue>()) error("the ts_simple in a \"bad\" state - there was exception");
    if (m.is_broken<ts_move>()) error("the move_to work correctly");
    if (!get<1>(m.scenarios).move_to_tracker.is_active()) error("the move_to_tracker is active");
    m.try_to_repair(event<1>{});
    if (get<1>(m.scenarios).move_to_tracker.is_active()) error("the move_to_tracker is active");
    if (!m.in_state<ts_move, state<100>>()) error("cannot move and the scenario in \"wrong\" state");
  }
}