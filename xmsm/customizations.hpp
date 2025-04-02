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

template<typename type> constexpr auto& variant_emplace(const auto& f, auto& v) {
  if constexpr(requires{emplace<type>(f, v);}) return emplace<type>(f, v);
  else if constexpr(requires{emplace<type>(v);}) return emplace<type>(v);
  else return v.template emplace<type>();
}
template<typename type> constexpr auto& variant_emplace(const auto& f, auto& v, type& next) {
  if constexpr(requires{emplace<type>(f, v, static_cast<type&&>(next));}) return emplace<type>(f, v, static_cast<type&&>(next));
  else if constexpr(requires{emplace<type>(v, static_cast<type&&>(next));}) return emplace<type>(v, static_cast<type&&>(next));
  else return v.template emplace<type>(static_cast<type&&>(next));
}

constexpr auto& xmsm_emplace_back(auto& obj, auto&&... v) {
  if constexpr(requires{emplace_back(obj, std::forward<decltype(v)>(v)...);}) return emplace_back(obj, std::forward<decltype(v)>(v)...);
  else if constexpr(requires{obj.emplace_back(std::forward<decltype(v)>(v)...);}) return obj.emplace_back(std::forward<decltype(v)>(v)...);
  else return obj.push_back(std::forward<decltype(v)>(v)...);
}
constexpr void pop_back(auto& obj) {
  obj.pop_back();
}

constexpr auto& xmsm_insert_or_emplace(auto& obj, auto&& key, auto&& val) {
  if constexpr(requires{obj.insert_or_assign(key, std::forward<decltype(val)>(val));}) {
    auto [iter, _] = obj.insert_or_assign(std::forward<decltype(key)>(key), std::forward<decltype(val)>(val));
    return *iter;
  }
  else {
    return xmsm_emplace_back(obj, std::forward<decltype(key)>(key), std::forward<decltype(val)>(val));
  }
}

template<typename type> constexpr auto create_state(const auto& f) {
  if constexpr(requires{create<type>(f);}) return create<type>(f);
  else if constexpr(requires{Create<type>(f);}) return Create<type>(f);
  else if constexpr(requires{type{f};}) return type{f};
  else return type{};
}
template<typename type> constexpr auto create_state(const auto& f, const auto& event) {
  if constexpr(requires{create<type>(f, event);}) return create<type>(f, event);
  else if constexpr(requires{Create<type>(f, event);}) return Create<type>(f, event);
  else if constexpr(requires{type{f, event};}) return type{f, event};
  else return create_state<type>(f);
}

template<typename type> constexpr bool test_var_in_state(const auto& v) {
  if constexpr(requires{holds_alternative<type>(v);}) return holds_alternative<type>(v);
  else return visit([]<typename inner>(const inner&){ return type_c<inner> == type_c<type>; }, v);
}

constexpr void call_on_exit(auto& scenario, auto& state, const auto& event) {
  //TODO: try to call member function first
  if constexpr(requires{on_exit(scenario, state, event);}) on_exit(scenario, state, event);
  else if constexpr(requires{on_exit(scenario, state);}) on_exit(scenario, state);
  else if constexpr(requires{onExit(scenario, state, event);}) onExit(scenario, state, event);
  else if constexpr(requires{OnExit(scenario, state);}) OnExit(scenario, state);
}
constexpr void call_on_enter(auto& scenario, auto& state, const auto& event) {
  //TODO: try to call member function first
  if constexpr(requires{on_enter(scenario, state, event);}) on_enter(scenario, state, event);
  else if constexpr(requires{on_enter(scenario, state);}) on_enter(scenario, state);
  else if constexpr(requires{onEnter(scenario, state, event);}) onEnter(scenario, state, event);
  else if constexpr(requires{OnEnter(scenario, state);}) OnEnter(scenario, state);
}
constexpr bool xmsm_contains(const auto& con, const auto& item) {
  if constexpr(requires{contains(con, item);}) return contains(con, item);
  else if constexpr(requires{con.contains(item);}) return con.contains(item);
  else {
    for (auto i=0;i<con.size();++i) if (con[i]==item) return true;
    return false;
  }
}

constexpr auto xmsm_find_pointer(const auto& f, auto& con, const auto& key) {
  if constexpr(requires{find(f, con, key);}) return find(f, con, key);
  else {
    using ret_type = decltype([](auto& i) { auto& [_,s] = i; return &s; }(*begin(con)));
    for (auto& i:con) {
      auto& [cur_key, item] = i;
      if (cur_key==key) return &item;
    }
    return (ret_type)nullptr;
  }
}
constexpr void xmsm_erase_if(const auto& f, auto& con, auto&& fnc) {
  if constexpr(requires{erase_if(f, con, std::forward<decltype(fnc)>(fnc));}) erase_if(f, con, std::forward<decltype(fnc)>(fnc));
  else {
    static_assert( requires{con[0];}, "try to provide erase_if method for this container");
    for (auto i=0;i<con.size();++i) if (fnc(con[i])) erase(f, con, i--);
  }
}

}