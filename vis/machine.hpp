#pragma once
#include "xmsm/machine.hpp"

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

namespace xmsm_vis {
template<auto i> struct state {};
template<auto i> struct event {};
struct ts1 {
  constexpr static auto describe_sm(const auto& f) {
    return mk_sm_description(f,
        mk_trans<state<0>, state<1>, event<1>>(f)
      , mk_trans<state<1>, state<2>>(f, allow_move(f), allow_queue(f))
      , mk_trans<state<2>, state<0>, event<100>>(f)
    );
  }
};

struct ts_with_move_to {
  constexpr static auto describe_sm(const auto& f) {
    return mk_sm_description(f,
        mk_trans<state<0>, state<1>, event<2>>(f, try_move_to<ts1, state<2>>(f))
      , mk_trans<state<1>, state<0>, event<200>>(f)
    );
  }
};

struct ts_with_stack {
  constexpr static auto describe_sm(const auto& f) {
    return mk_sm_description(f,
        mk_trans<state<0>, state<1>, event<3>>(f, stack_by_expr(f, in<ts1, state<1>>(f)))
      , mk_trans<state<1>, state<0>, event<300>>(f)
    );
  }
};

struct taskbar_app;
struct desktop_app;
struct app_hidden{}; struct app_visible{}; struct app_not_run{}; struct app_final{};

struct desktop_app {
  struct wait_for_start{};
  struct wait_to_show{};
  struct crashed{};
  struct crash{};
  struct started{};
  constexpr static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<app_not_run, wait_for_start>(f, allow_move(f))
      , mk_qtrans<wait_for_start, app_hidden, started>(f, allow_move(f))
      , mk_qtrans<app_hidden, app_visible>(f, allow_move(f))
      , mk_qtrans<app_visible, app_hidden>(f, allow_move(f))

      , mk_trans<wait_for_start, crashed, crash>(f)
      , mk_trans<app_hidden, crashed, crash>(f)
      , mk_trans<app_visible, crashed, crash>(f)

      , mk_qtrans<crashed, app_final>(f, allow_move(f))
      , mk_qtrans<crashed, app_not_run>(f, allow_move(f))
    );
  }
};
struct taskbar_app {
  struct wait_for_start{};
  struct wait_to_show{};
  struct crashed{};

  struct crash{};
  struct started{};
  struct ui_display{};
  constexpr static auto describe_sm(const auto& f) {
    return mk_sm_description(f
    , mk_qtrans<app_not_run, wait_for_start>(f, allow_move(f))
    , mk_qtrans<wait_for_start, app_hidden, started>(f)
    , mk_qtrans<app_hidden, app_visible, ui_display>(f)

    , mk_trans<wait_for_start, crashed, crash>(f)
    , mk_trans<app_hidden, crashed, crash>(f)
    , mk_trans<app_visible, crashed, crash>(f)

    , mk_qtrans<crashed, app_final>(f, allow_move(f))
    , mk_qtrans<crashed, app_not_run>(f, allow_move(f))
    );
  }
};
struct taskbar_desktop {
  struct nothing {};
  struct taskbar_pending {};
  struct desktop_pending {};
  struct desktop {};
  struct taskbar_only {};
  struct all_hidden {};

  struct start{};
  constexpr static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_qtrans<nothing, taskbar_pending, start>(f, allow_move(f), move_to<taskbar_app, app_visible, nothing>(f))
      , mk_qtrans<taskbar_pending, desktop_pending>(f, allow_move(f), move_to<desktop_app, app_visible, nothing>(f), when(f, in<taskbar_app, app_visible>(f)))
      , mk_qtrans<desktop_pending, desktop>(f, when(f, in<desktop_app, app_hidden>(f)), move_to<desktop_app, app_visible, nothing>(f))

      , mk_qtrans<desktop, taskbar_only>(f, allow_move(f), move_to<desktop_app, app_hidden, nothing>(f))
      , mk_qtrans<taskbar_only, desktop>(f, allow_move(f), move_to<desktop_app, app_visible, nothing>(f))
      , mk_qtrans<taskbar_only, all_hidden>(f, allow_move(f), move_to<desktop_app, app_not_run, nothing>(f), move_to<taskbar_app, app_not_run, nothing>(f))

      , mk_trans<desktop, desktop_pending>(f, when(f, in<desktop_app, app_not_run>(f)), move_to<desktop_app, app_visible, nothing>(f))
      , mk_trans<desktop, taskbar_pending>(f, when(f, in<taskbar_app, app_not_run>(f)), move_to<taskbar_app, app_visible, nothing>(f))
      , mk_trans<taskbar_only, taskbar_pending>(f, when(f, in<taskbar_app, app_not_run>(f)), move_to<taskbar_app, app_visible, nothing>(f))

      , mk_qtrans<all_hidden, taskbar_only>(f, allow_move(f), move_to<taskbar_app, app_visible, nothing>(f))
      , mk_qtrans<all_hidden, desktop>(f, allow_move(f), move_to<taskbar_app, app_visible, nothing>(f), move_to<desktop_app, app_visible, nothing>(f))
    );
  }
};
template<typename observing>
struct rerun_once {
  struct normal {};
  struct crashed_once{};
  struct wont_rerun{};
  constexpr static auto describe_sm(const auto& f) {
    return mk_sm_description(f //TODO: move_to here moves to app_not_run, right after that taskbar_desktop moves desktop to wait_to_start and move_to fails and move rerun_once to wont_rerun
      , mk_trans<normal, crashed_once>(f, when(f, in<observing, typename observing::crashed>(f) && affected<observing>(f)), move_to<observing, app_not_run, wont_rerun>(f))
      , mk_trans<crashed_once, wont_rerun>(f, when(f, in<observing, typename observing::crashed>(f) && affected<observing>(f)))
      , mk_trans<crashed_once, normal>(f, when(f, in<observing, app_visible>(f) && affected<observing>(f)))
    );
  }
};

template<typename factory> using machine = xmsm::machine<factory, ts1, ts_with_move_to, ts_with_stack, taskbar_app, desktop_app, taskbar_desktop, rerun_once<taskbar_app>, rerun_once<desktop_app>>;

}
