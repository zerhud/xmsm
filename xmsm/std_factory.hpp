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

template<typename... types> constexpr auto mk_variant(const std_factory&) {
  return std::variant<types...>{};
}
template<typename type> constexpr auto mk_vec(const std_factory&) { return std::vector<type>{}; }
template<typename type> constexpr auto mk_list(const std_factory&) { return std::list<type>{}; }
constexpr void erase(const std_factory&, auto& cnt, auto ind) {
  cnt.erase(cnt.begin() + ind);
}

template<typename type> constexpr auto mk_atomic(const std_factory&) { return std::atomic<type>{}; }

}