#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "xmsm.hpp"

#include <list>
#include <atomic>
#include <vector>
#include <variant>
#include <string_view>

namespace tests {

struct factory {
  using string_view = std::string_view;
};

template<typename type> constexpr auto make(struct xmsm::tags::vector, const factory&, xmsm::_type_c<type>) {
  return std::vector<type>{};
}
template<typename type> constexpr auto make(struct xmsm::tags::list, const factory&, xmsm::_type_c<type>) {
  auto ret = std::vector<type>{};
  ret.reserve(1000);
  return ret;
}
template<typename... types> constexpr auto make(struct xmsm::tags::variant, const factory&, xmsm::type_list<types...>) {
  return std::variant<types...>{};
}

template<typename type> constexpr auto make(struct xmsm::tags::atomic, const factory&, xmsm::_type_c<type>) {
  return type{};
}
constexpr void erase(const factory&, auto& cnt, auto ind) {
  cnt.erase(cnt.begin() + ind);
}

template<auto v> struct state { constexpr static auto val = v; int rt_val{0}; };
template<auto v> struct event { constexpr static auto val = v; int rt_val{0}; };

struct rt_factory : factory {};
template<typename type> constexpr auto make(struct xmsm::tags::list, const rt_factory&, xmsm::_type_c<type>) {
  return std::list<type>{};
}

}
