#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include <utility>

namespace xmsm {

template<typename t> struct _type_c{ using type = t; t operator+() const ; };
template<typename t=void> constexpr auto type_c = _type_c<t>{};
template<typename t=void> constexpr auto type_dc = _type_c<std::decay_t<t>>{};
template<typename l, typename r> constexpr bool operator==(_type_c<l>, _type_c<r>) { return false; }
template<typename c> constexpr bool operator==(_type_c<c>, _type_c<c>) { return true; }
template<typename...> struct type_list{};
template<typename... items> constexpr auto size(const type_list<items...>&) { return sizeof...(items); }
template<typename type, typename... items> constexpr bool contains(const type_list<items...>&) {
  return (0 + ... + (type_c<items> == type_c<type>));
}
template<typename type, typename... items> constexpr bool contains(const type_list<items...>& l, _type_c<type>) {
  return contains<type>(l);
}
template<typename... items, typename type> constexpr auto operator<<(type_list<items...> l, _type_c<type> t) {
  if constexpr (contains(l, t) || t == type_c<void>) return l;
  else return type_list<items..., type>{};
}
template<typename... items> constexpr auto unpack(type_list<items...>, auto&& fnc) {
  return [&]<auto... inds>(std::index_sequence<inds...>){
    return fnc(type_c<items>...);
  }(std::make_index_sequence<sizeof...(items)>{});
}
template<auto ind, typename... items> constexpr auto get(type_list<items...>) { return type_c<__type_pack_element<ind, items...>>; }
template<typename... items> constexpr auto filter(type_list<items...>, auto&& p) {
  return (type_list<>{} << ... << p(type_c<items>));
}
template<typename... items> constexpr auto first(type_list<items...> l) {
  if constexpr(sizeof...(items)==0) return type_c<void>;
  else return get<0>(l);
}

}