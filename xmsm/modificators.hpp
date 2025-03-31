#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "hana.hpp"

namespace xmsm::modificators {

template<typename e, typename... tail> struct stack_by_event {
  constexpr static bool is_stack_by_event = true;
  constexpr static auto back_events = (type_list<e>{} << ... << type_c<tail>) ;
};

template<typename e> struct stack_by_expression {
  constexpr static bool is_stack_by_expression = true;
  constexpr static auto expression = type_c<e>;
};

template<typename type> struct def_state {
  constexpr static bool is_def_state = true;
  constexpr static auto st = type_c<type>;
};

template<typename e> struct when { constexpr static bool is_when = true; constexpr static e expression{}; };
template<typename e> struct only_if { constexpr static bool is_only_if = true; constexpr static auto expression = type_c<e>; };

template<typename sc, typename st> struct move_to {
  constexpr static auto scenario = type_c<sc>;
  constexpr static auto state = type_c<st>;
};
template<typename sc, typename st> struct try_move_to {
  constexpr static auto scenario = type_c<sc>;
  constexpr static auto state = type_c<st>;
};

}