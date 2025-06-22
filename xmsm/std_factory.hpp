#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include <list>
#include <vector>
#include <atomic>
#include <variant>
#include <string_view>

namespace xmsm {

struct std_factory {
  using string_view = std::string_view;
};

template<typename type> constexpr auto make(struct tags::vector, const std_factory&, _type_c<type>) {
  return std::vector<type>{};
}
template<typename type> constexpr auto make(struct tags::list, const std_factory&, _type_c<type>) {
  return std::list<type>{};
}

template<typename... types> constexpr auto make(struct tags::variant, const std_factory&, type_list<types...>) {
  return std::variant<types...>{};
}
constexpr void erase(const std_factory&, auto& cnt, auto ind) {
  cnt.erase(cnt.begin() + ind);
}

template<typename type> constexpr auto make(struct tags::atomic, const std_factory&, type_list<type>) {
  return std::atomic<type>{};
}

}