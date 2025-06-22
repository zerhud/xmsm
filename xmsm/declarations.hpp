#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

namespace xmsm {

enum class sync_command{ sync, move_to, move_to_response, sync_multi, move_to_response_multi };
enum class scenario_state { ready, broken, fired };

template<typename factory, typename object> struct basic_scenario ;
template<typename, typename, typename, typename> struct scenario;

namespace tags {

template<typename> struct user_object {};

constexpr struct vector {} vector{};
constexpr struct list : vector {} list{};
constexpr struct stack : vector {} stack{};
constexpr struct state_stack : vector {} state_stack{};
constexpr struct map {} map {};
constexpr struct variant {} variant{};
constexpr struct atomic {} atomic{};

}

}
