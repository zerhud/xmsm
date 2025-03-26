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

template<typename type> struct def_state {
  constexpr static bool is_def_state = true;
  constexpr static auto st = type_c<type>;
};

}