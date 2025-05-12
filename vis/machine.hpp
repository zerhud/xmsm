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

template<typename factory> using machine = xmsm::machine<factory, ts1, ts_with_move_to, ts_with_stack>;

}