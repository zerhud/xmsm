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

struct sc_queue {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<101>>(f)
      , mk_qtrans<state<1>, state<2>, event<101>>(f)
    );
  }
};

struct sc_mover {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<state<0>, state<1>, event<102>>(f, move_to<sc_queue, state<2>, state<100>>(f))
    );
  }
};

static_assert( [] {
  xmsm::machine<factory, sc_queue, sc_mover> m{factory{}};
  m.on(event<101>{}); m.on(event<101>{});
  m.on(event<102>{});
  return m.in_state<sc_queue, state<2>>() + 2*m.in_state<sc_mover, state<1>>();
}() == 3, "move_to works normally if required state already achieved" );

struct st_not_run{};
struct st_ready {};
struct st_visible{};
struct st_crashed{};
struct off {};
struct on {};
struct controller {
  struct switch_on{};
  struct switch_off{};
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f, mk_qtrans<off, on, switch_on>(f), mk_qtrans<on, off, switch_off>(f) );
  }
};
struct app {
  struct ev_crash{};
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<st_not_run, st_ready>(f, allow_move(f), when(f, in<controller, on>(f)))
      , mk_qtrans<st_ready, st_visible>(f, allow_move(f))
      , mk_trans<st_ready, st_crashed, ev_crash>(f)
      , mk_trans<st_visible, st_crashed, ev_crash>(f)
      , mk_qtrans<st_crashed, st_not_run>(f, allow_move(f))
    );
  }
};

template<typename app> struct reruner {
  struct once {};
  struct final {};
  struct normal {};
  struct move_to_fail{};
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<normal, once, typename app::ev_crash>(f, move_to<app, st_not_run, move_to_fail>(f))
      , mk_trans<once, normal>(f, when(f, in<app, st_visible>(f) && affected<app>(f)))
      , mk_trans<once, final, typename app::ev_crash>(f)
    );
  }
};

template<typename factory> using machine = xmsm::machine<factory, controller, app, reruner<app>>;

static_assert( [] {
  machine<factory> m{factory{}};
  m.on(controller::switch_on{});
  m.on(app::ev_crash{});
  return m.in_state<app, st_ready>() + 2*m.in_state<reruner<app>, reruner<app>::once>();
}() == 3 );

int main(int,char**) {
}
