#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "xmsm.hpp"

#include <variant>

namespace tests {

struct factory {
  template<typename... types> using variant_t = std::variant<types...>;
};

template<auto v> struct state { constexpr static auto val = v; };
template<auto v> struct event { constexpr static auto val = v; };

}
