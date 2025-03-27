#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "hana.hpp"

namespace xmsm { template<typename, typename, typename> struct scenario; }

namespace xmsm::scenario_checker {

template<typename sc, typename... st> struct in {
  constexpr static bool is_checker = true;
  constexpr static auto scenario = type_c<sc>;
  constexpr static auto states = (type_list{} << ... << type_c<st>);

  constexpr bool operator()(auto&&... s) const { return (false || ... || check(s)); }
  template<typename factory, typename obj, typename... tail> constexpr static bool check(const xmsm::scenario<factory, obj, tail...>& s) {
    auto cur = s.cur_state_hash();
    return scenario == type_dc<obj> && unpack(states, [&](auto... i){ return (false || ... || (hash<factory>(i)==cur)); });
  }
};

template<typename l, typename r> struct and_checker {
  constexpr static bool is_checker = true;
  constexpr static auto left = type_c<l>;
  constexpr static auto rigth = type_c<r>;
  constexpr bool operator()(auto&&... s) const { return l{}(static_cast<decltype(s)&&>(s)...) && r{}(static_cast<decltype(s)&&>(s)...); }
};
template<typename l, typename r> struct or_checker {
  constexpr static bool is_checker = true;
  constexpr static auto left = type_c<l>;
  constexpr static auto rigth = type_c<r>;
  constexpr bool operator()(auto&&... s) const { return l{}(static_cast<decltype(s)&&>(s)...) || r{}(static_cast<decltype(s)&&>(s)...); }
};
template<typename c> struct not_checker {
  constexpr static bool is_checker = true;
  constexpr static auto checker = type_c<c>;
  constexpr bool operator()(auto&&... s) const { return !c{}(static_cast<decltype(s)&&>(s)...); }
};

constexpr auto operator!(auto&& c){ return not_checker<std::decay_t<decltype(c)>>{}; }
constexpr auto operator||(auto&& l, auto&& r){ return or_checker<std::decay_t<decltype(l)>, std::decay_t<decltype(r)>>{}; }
constexpr auto operator&&(auto&& l, auto&& r){ return and_checker<std::decay_t<decltype(l)>, std::decay_t<decltype(r)>>{}; }

}
