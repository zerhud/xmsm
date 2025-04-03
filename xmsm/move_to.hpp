#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "hana.hpp"

namespace xmsm {

struct allow_queue;
namespace move_to_helpers {
template<typename tt> constexpr auto trans_info_to(auto tinfo, _type_c<tt>) { return filter(tinfo, [](auto i){return decltype(+i)::to == type_c<tt>;}); }
template<typename tt> constexpr auto filter_by_target(auto tinfo, _type_c<tt>, auto tail) {
  auto selected = trans_info_to<tt>(tinfo, type_c<tt>);
  if constexpr (size(selected)==0) return type_list<decltype(tail)>{};
  else return unpack(selected, [&](auto... s) {
    return type_list<decltype(type_list<decltype(+decltype(+s)::from)>{} << tail)...>{};
  });
}
constexpr auto filter_by_target(auto tinfo, auto path) {
  auto iter = filter_by_target(tinfo, first(path), path);
  if constexpr(size(iter)==1 && first(iter)()==path) return iter;
  else return unpack(iter, [&](auto... pathes){return (type_list{} << ... << [&](auto cur) {
    return filter_by_target(tinfo, cur);
  }(pathes()));});
}
}

template<typename target_t> constexpr auto mk_allow_queue_st(auto tinfo, _type_c<target_t> target) {
  constexpr auto list = filter(tinfo, [](auto i){return decltype(+i)::is_queue_allowed;});
  constexpr auto to_target = filter(list, [](auto i){return decltype(+i)::to == type_c<target_t>;});
  constexpr auto to_target_st = unpack(to_target, [](auto... t){return type_list<type_list<decltype(+decltype(+t)::to)>...>{};});
  return unpack(to_target_st, [&](auto... pathes){return (type_list{} << ... << move_to_helpers::filter_by_target(list, pathes()));});
}

template<typename target_t>  constexpr auto all_targets_for_move_to(auto tinfo, _type_c<target_t> target) {
  constexpr auto src = mk_allow_queue_st(tinfo, target);
  return unpack(src, [](auto... seq){return (type_list{} << ... << pop_front(seq()));});
}

template<typename hash_type, auto count, auto max_size>
struct state_queue_tracker {
  hash_type fail_state{};
  hash_type hashes[count][max_size]{};
  int indexes[count]{-1};
};

}