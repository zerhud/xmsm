#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "declarations.hpp"
#include "hana.hpp"

namespace xmsm::utils {

template<typename type, typename fnc_type> struct pointer_selector {
  fnc_type fnc;
  template<typename factory, typename user_type> constexpr pointer_selector& operator+(scenario<factory, type, user_type>& s) { fnc(s); return *this; }
  template<typename factory, typename fail, typename user_type> constexpr pointer_selector& operator+(scenario<factory, fail, user_type>&) { return *this; }
};
template<typename factory, typename type> constexpr auto cur_state_hash_from_set(auto&... scenarios){
  decltype(hash<factory>(type_c<type>)) ret{};
  auto fnc = [&](auto& s){ret=s.cur_state_hash();};
  (void)(pointer_selector<type, decltype(fnc)>{fnc} + ... + scenarios);
  return ret;
}

}