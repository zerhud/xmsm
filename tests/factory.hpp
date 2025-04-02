#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "xmsm.hpp"

#include <vector>
#include <variant>
#include <string_view>

namespace tests {

struct factory {
  using string_view = std::string_view;
};

template<typename... types> constexpr auto mk_variant(const factory&) {
  return std::variant<types...>{};
}
template<typename type> constexpr auto mk_list(const factory&) {
  auto ret = std::vector<type>{};
  ret.reserve(1000); // we can't use std::list in constexpr context, and the vector shouldn't move elements
  return ret;
}
template<typename type> constexpr auto mk_atomic(const factory&) {
  return type{};
}
constexpr void erase(const factory&, auto& cnt, auto ind) {
  cnt.erase(cnt.begin() + ind);
}

template<auto v> struct state { constexpr static auto val = v; int rt_val{0}; };
template<auto v> struct event { constexpr static auto val = v; int rt_val{0}; };

}
