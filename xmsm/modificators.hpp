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

template<typename type> concept check_expression = requires{type::is_checker;};

struct allow_queue {};
struct allow_move {};
template<typename e, typename... tail> struct stack_by_event {
  constexpr static bool is_stack_by_event = true;
  constexpr static auto back_events = (type_list<e>{} << ... << type_c<tail>) ;
};

template<check_expression e> struct stack_by_expression {
  constexpr static bool is_stack_by_expression = true;
  constexpr static auto expression = type_c<e>;
};

template<typename type> struct def_state {
  constexpr static bool is_def_state = true;
  constexpr static auto st = type_c<type>;
};
template<typename type> struct finish_state {
  constexpr static bool is_finish_state = true;
  constexpr static auto st = type_c<type>;
};
template<typename type> struct start_event {
  constexpr static bool is_start_event = true;
  constexpr static auto event = type_c<type>;
};
template<typename type, typename... _mods> struct to_state_mods {
  constexpr static bool is_to_state_mods = true;
  constexpr static auto st = type_c<type>;
  constexpr static auto mods = type_list<_mods...>{};
};
template<typename type, typename... _mods> struct from_state_mods {
  constexpr static bool is_from_state_mods = true;
  constexpr static auto st = type_c<type>;
  constexpr static auto mods = type_list<_mods...>{};
};

template<check_expression e> struct when { constexpr static bool is_when = true; constexpr static e expression{}; };
template<check_expression e> struct only_if { constexpr static bool is_only_if = true; constexpr static e expression{}; };

template<typename sc, typename st, typename fst> struct move_to {
  constexpr static bool is_move_to = true;
  constexpr static auto scenario = type_dc<sc>;
  constexpr static auto state = type_dc<st>;
  constexpr static auto fail_state = type_dc<fst>;
};
template<typename sc, typename st> struct try_move_to {
  constexpr static bool is_try_move_to = true;
  constexpr static auto scenario = type_c<sc>;
  constexpr static auto state = type_c<st>;
};

}