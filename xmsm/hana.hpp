#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include <utility>

#include "hash.hpp"

namespace xmsm {

template<typename t> struct _type_c{ using type = t; t operator+() const ; };
template<typename t=void> constexpr auto type_c = _type_c<t>{};
template<typename t=void> constexpr auto type_dc = _type_c<std::decay_t<t>>{};
template<typename l, typename r> constexpr bool operator==(_type_c<l>, _type_c<r>) { return false; }
template<typename c> constexpr bool operator==(_type_c<c>, _type_c<c>) { return true; }
template<typename l, typename r> constexpr bool operator<=(_type_c<l> ll, _type_c<r> rr) { return __is_base_of(l, r); }
template<typename l, typename r> constexpr bool operator>=(_type_c<l> ll, _type_c<r> rr) { return __is_base_of(r, l); }
template<typename...> struct type_list{};
template<typename... items> constexpr auto size(const type_list<items...>&) { return sizeof...(items); }
template<typename type, typename... items> constexpr bool contains(const type_list<items...>&) {
  return (0 + ... + (type_c<items> <= type_c<type>));
}
template<typename type, typename... items> constexpr bool contains(const type_list<items...>& l, _type_c<type>) {
  return contains<type>(l);
}
template<typename... l, typename r> constexpr auto operator+(const type_list<l...>&, _type_c<r>) { return type_list<l..., r>{}; }
template<typename... l, typename... r> constexpr auto operator+(const type_list<l...>&, const type_list<r...>&) { return type_list<l..., r...>{}; }
template<typename... items, typename type> constexpr auto operator<<(const type_list<items...>& l, _type_c<type> t) {
  if constexpr (contains(l, t) || t == type_c<>) return l;
  else return type_list<items..., type>{};
}
template<typename... left, typename... right> constexpr auto operator<<(const type_list<left...>&, const type_list<right...>&) {
  return type_list<left..., right...>{};
}
template<typename... items> constexpr auto unpack(const type_list<items...>&, auto&& fnc) {
  return [&]<auto... inds>(std::index_sequence<inds...>){
    return fnc(type_c<items>...);
  }(std::make_index_sequence<sizeof...(items)>{});
}
template<auto ind, typename... items> constexpr auto get(const type_list<items...>&) { return type_c<__type_pack_element<ind, items...>>; }
template<typename... items> constexpr auto index_of(const type_list<items...>&, auto item) {
  auto ind=-1;
  (void)( false || ... || (++ind,type_c<items> <= item));
  return ind;
}
constexpr auto find(const auto& list, auto item) {
  return get<index_of(list, item)>(list);
}
template<typename... items> constexpr auto filter(const type_list<items...>&, auto&& p) {
  if constexpr (sizeof...(items)==0) return type_list{};
  else if constexpr (type_dc<decltype(p(type_c<__type_pack_element<0, items...>>))> != type_c<bool>) return (type_list{} << ... << p(type_c<items>));
  else {
    return (type_list{} << ... << [&](auto i){if constexpr(p(i)) return i; else return type_c<>;}(type_c<items>));
  }
}
template<typename... items> constexpr auto first(const type_list<items...>& l) {
  if constexpr(sizeof...(items)==0) return type_c<>;
  else return get<0>(l);
}
consteval auto max_min_size(auto&&... lists) {
  struct { unsigned max{}, min{}; constexpr bool operator=(unsigned v){ max = max*(max>=v) + v*(max<v); min = min*(min<=v) + min*(v<min); return true; } } ret;
  (void)( true && ... && (ret=size(lists)) );
  return ret;
}

template<typename factory, typename type> constexpr auto name(_type_c<type>) {
  using sv = factory::string_view;
  auto ret = sv{__PRETTY_FUNCTION__};
  ret.remove_suffix(1);
  ret.remove_prefix(ret.find(sv{"type = "}) + 7);
  return ret;
}

template<typename factory> constexpr auto hash32(auto type) {
  auto src = name<factory>(type);
  return hash32(src.data(), src.size(), 0);
}
template<typename factory> constexpr auto hash64(auto type) {
  auto src = name<factory>(type);
  return hash64(src.data(), src.size(), 0);
}
template<typename factory> constexpr auto hash(auto type) { return hash32<factory>(type); }

}