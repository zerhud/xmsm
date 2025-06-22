#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "hash.hpp"

namespace xmsm {

namespace details {
template<typename t> constexpr decltype(auto) move(t&v) {return (t&&)v;}
template<typename _Tp, typename _Up = _Tp&&> _Up __declval(int);
template<typename _Tp> _Tp __declval(long);
template<typename _Tp> auto declval() noexcept -> decltype(__declval<_Tp>(0));
template <typename T> struct decay { template <typename U> static U impl(U); using type = decltype(impl(declval<T>())); };
template<typename, auto...> struct shim_for_clang_stupidity{};
}
template<auto val> constexpr decltype(auto) with_inds(auto&& fnc, auto&&...args) {
#if __has_builtin(__integer_pack)
  return fnc.template operator()<__integer_pack(val)...>(static_cast<decltype(args)&&>(args)...);
#else
  return [&]<typename shim, auto...inds>(details::shim_for_clang_stupidity<shim, inds...>){
    return fnc.template operator()<inds...>(static_cast<decltype(args)&&>(args)...);
  }(__make_integer_seq<details::shim_for_clang_stupidity, int, val>{});
#endif
}

template<auto v> struct _value{ constexpr static auto val = v; };
template<auto v> constexpr auto value_c = _value<v>{};
template<auto...> struct value_list{};
template<auto...v> constexpr auto size(value_list<v...>) { return sizeof...(v);}
template<typename t> struct _type_c{ using type = t; t operator+() const ; constexpr t operator()()const{return t{};} constexpr operator bool()const{return __is_same(type, void);}};
template<typename t> constexpr t operator*(const _type_c<t>&){ return t{}; }
template<typename t=void> constexpr auto type_c = _type_c<t>{};
template<typename t=void> constexpr auto type_dc = _type_c<typename details::decay<t>::type>{};
template<typename l, typename r> constexpr bool operator==(_type_c<l>, _type_c<r>) { return false; }
template<typename c> constexpr bool operator==(_type_c<c>, _type_c<c>) { return true; }
template<typename l, typename r> constexpr bool operator<=(_type_c<l> ll, _type_c<r> rr) { return __is_base_of(l, r); }
template<typename l, typename r> constexpr bool operator>=(_type_c<l> ll, _type_c<r> rr) { return __is_base_of(r, l); }
template<typename l> constexpr bool operator<=(_type_c<l> ll, _type_c<l> rr) { return true; }
template<typename l> constexpr bool operator>=(_type_c<l> ll, _type_c<l> rr) { return true; }
template<typename t> concept is_type_c = requires{ []<typename i>(_type_c<i>){}(t{}); };
template<typename...> struct type_list{};
template<typename... l, typename... r> constexpr bool operator==(type_list<l...>, type_list<r...>) { return false; }
template<typename... c> constexpr bool operator==(type_list<c...>, type_list<c...>) { return true; }
template<typename... items> constexpr auto size(const type_list<items...>&) { return sizeof...(items); }
template<typename type, typename... items> constexpr bool contains(const type_list<items...>&) {
  return (0 + ... + (type_c<items> <= type_c<type>));
}
template<typename type, typename... items> constexpr bool contains(const type_list<items...>& l, _type_c<type>) { return contains<type>(l); }
template<typename... items, typename... checks> constexpr bool contains(const type_list<items...>& list, const type_list<checks...>&) {
  return (0+...+contains<checks>(list));
}
template<typename... l, typename r> constexpr auto operator+(const type_list<l...>&, _type_c<r>) { return type_list<l..., r>{}; }
template<typename... l, typename... r> constexpr auto operator+(const type_list<l...>&, const type_list<r...>&) { return type_list<l..., r...>{}; }
template<typename... l, typename r> constexpr auto operator%(const type_list<l...>&, _type_c<r>) { return type_list<r, l...>{}; }
template<typename... items, typename type> constexpr auto operator<<(const type_list<items...>& l, _type_c<type> t) {
  if constexpr (t == type_c<> || contains(l, t)) return l;
  else return type_list<items..., type>{};
}
template<typename... left, typename... right> constexpr auto operator<<(const type_list<left...>& l, const type_list<right...>&) {
  return (l << ... << type_c<right>);
}
template<typename... items> constexpr auto revert(const type_list<items...>&) {return (type_list{} % ... % type_c<items>);}
template<typename... items> constexpr auto unpack(const type_list<items...>&, auto&& fnc) { return fnc(type_c<items>...); }
template<typename... items> constexpr auto unpack_with_inds(const type_list<items...>&, auto&& fnc) {
  return with_inds<sizeof...(items)>(static_cast<decltype(fnc)&&>(fnc), type_c<items>...);
}
template<typename... items> constexpr bool foreach(const type_list<items...>&, auto&& fnc) {
  return (false || ... || fnc(type_c<items>));
}
template<auto ind, typename... items> constexpr auto get(const type_list<items...>&) { return type_c<__type_pack_element<ind, items...>>; }
template<typename... items> constexpr auto index_of(const type_list<items...>&, auto item) {
  auto ind=-1;
  const bool found = ( false || ... || (++ind,type_c<items> <= item) );
  return (-1*!found) + ind*found;
}
template<typename... items> constexpr auto index_of_by_hash(const type_list<items...>&, auto target_hash) {
  auto ind=-1;
  const bool found = (false || ... || (++ind,hash(type_c<items>)==target_hash));
  return (-1*!found) + ind*found;
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
template<typename... items> constexpr auto last(const type_list<items...>& l) {
  if constexpr(sizeof...(items)==0) return type_c<>;
  else return get<sizeof...(items)-1>(l);
}
consteval auto max_min_size(auto&&... lists) {
  struct { unsigned max{}, min{}; constexpr bool operator=(unsigned v){ max = max*(max>=v) + v*(max<v); min = min*(min<=v) + min*(v<min); return true; } } ret;
  (void)( true && ... && (ret=size(lists)) );
  return ret;
}
consteval auto max_size(auto&&... lists) {
  auto [max,min] = max_min_size(static_cast<decltype(lists)&&>(lists)...);
  return max;
}

template<typename type> constexpr auto _name(_type_c<type>) {
  auto len = []<auto N>(const char(&str)[N]){return (unsigned)(N-1);};
  constexpr auto pref_len =
  //TODO: add msvc??
#ifdef __clang__
    len("auto xmsm::_name(_type_c<type>) [type = ")
#else
    len("constexpr auto xmsm::_name(_type_c<r>) [with type = ")
#endif
    ;
  struct {
    const char* base;
    unsigned size;
  } ret {__PRETTY_FUNCTION__, len(__PRETTY_FUNCTION__)-1};
  ret.base += pref_len;
  ret.size -= pref_len;
  return ret;
}
template<typename factory, typename type> constexpr auto name(_type_c<type>) {
  using sv = factory::string_view;
  constexpr auto vec = _name(type_c<type>);
  return sv{vec.base, vec.size};
}

constexpr auto hash32(auto type) {
  constexpr auto src = _name(type);
  return hash32(src.base, src.size, 0);
}
constexpr auto hash64(auto type) {
  constexpr auto src = _name(type);
  return hash64(src.base, src.size, 0);
}
constexpr auto hash(auto type) { return hash32(type); }

template<typename type, auto ind> struct tuple_value {
  type value;
  template<auto i> constexpr friend type& get(tuple_value& t) requires(i==ind){ return t.value; }
  template<auto i> constexpr friend const type& get(const tuple_value& t) requires(i==ind){ return t.value; }
  template<typename object, template<typename...>class holder, typename factory, typename... tail, auto _ind>
  constexpr friend type& get(tuple_value<holder<factory, object, tail...>, _ind>& t) requires (type_c<type> == type_c<holder<factory,object,tail...>>) { return t.value; }

  template<typename base> constexpr friend type& by_base(tuple_value& t) requires(type_c<base> <= type_c<type>) { return t.value; }
  template<typename base> constexpr friend const type& by_base(const tuple_value& t) requires(type_c<base> <= type_c<type>) { return t.value; }
};

template<typename... bases> struct tuple_storage : bases... {
  constexpr decltype(sizeof...(bases)) size() const { return sizeof...(bases); }
#ifndef __clang__ //TODO: GCC15: bug in gcc: the tuple_value is ambiguous base class for some reason, and the friend get method is not available for typename template parameter
  template<typename object> constexpr friend auto& get(tuple_storage& t) { return obj_get<object, 0>(t); }
  template<typename object> constexpr static bool obj_check(const auto&) { return false; }
  template<typename object, template<typename...>class holder, typename factory, typename... tail> constexpr static bool obj_check(const holder<factory, object, tail...>&) { return true; }
  template<typename object, auto ind> constexpr static auto& obj_get(tuple_storage& obj) {
    if constexpr(obj_check<object>(get<ind>(obj))) return get<ind>(obj);
    else return obj_get<object, ind+1>(obj);
  }
#endif
};
constexpr auto mk_tuple(auto&&... items) {
  return with_inds<sizeof...(items)>([&]<auto... inds>{
    return tuple_storage<tuple_value<decltype(+type_dc<decltype(items)>), inds>...>{static_cast<decltype(items)&&>(items)...};
  });
}

constexpr auto size(const auto& obj) requires requires{obj.size();} { return obj.size(); }
constexpr auto foreach(auto&& obj, auto&& fnc) requires requires{obj.size(); get<0>(obj);} {
  return with_inds<size(obj)>([&]<auto...inds>{ return (false||...||fnc(get<inds>(obj))); });
}
constexpr auto unpack(auto&& obj, auto&& fnc) requires requires{obj.size(); get<0>(obj);} {
  return with_inds<size(obj)>([&]<auto...inds>{ return fnc(get<inds>(obj)...); });
}
constexpr auto unpack(auto&& obj, auto&& filter, auto&& fnc) {
  constexpr auto inds = with_inds<size(obj)>([&]<auto...inds>{
    return (type_list{} << ... << [&]<auto cur>(_value<cur>) {
      if constexpr(filter(get<cur>(obj))) return type_c<_value<cur>>;
      else return type_c<>;
    }(value_c<inds>));
  });
  return unpack(inds, [&](auto... inds) {
    return fnc(get<decltype(+inds)::val>(obj)...);
  });
}

constexpr auto has_duplicates(auto&&... items) {
  return (0 + ... + [&](auto cur){ return (-1 + ... + (cur==items)); }(items)) / 2;
}
template<typename first, typename... items> constexpr auto pop_front(const type_list<first, items...>&) { return type_list<items...>{}; }
template<typename... items> constexpr auto pop_front(const type_list<items...>&, auto&& fnc) {
  return (type_list{} << ... << [&](auto item) {
    if constexpr(fnc(item)) return item;
    else return type_c<>;
  }(type_c<items>));
}

}