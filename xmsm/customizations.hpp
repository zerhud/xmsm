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
  if constexpr(requires{emplace_back(obj, static_cast<decltype(v)&&>(v)...);}) return emplace_back(obj, static_cast<decltype(v)&&>(v)...);
  else if constexpr(requires{obj.emplace_back(static_cast<decltype(v)&&>(v)...);}) return obj.emplace_back(static_cast<decltype(v)&&>(v)...);
  else return obj.push_back(static_cast<decltype(v)&&>(v)...);
}
constexpr void pop_back(auto& obj) {
  if constexpr(requires{obj.pop_back();}) obj.pop_back();
  else obj.pop();
}

constexpr auto& xmsm_insert_or_emplace(auto& obj, auto&& key, auto&& val) {
  if constexpr(requires{obj.insert_or_assign(key, static_cast<decltype(val)&&>(val));}) {
    auto [iter, _] = obj.insert_or_assign(static_cast<decltype(key)&&>(key), static_cast<decltype(val)&&>(val));
    return *iter;
  }
  else {
    return xmsm_emplace_back(obj, static_cast<decltype(key)&&>(key), static_cast<decltype(val)&&>(val));
  }
}

template<typename type> constexpr auto create_object(const auto& f) {
  if constexpr(requires{create<type>(f);}) return create<type>(f);
  else if constexpr(requires{Create<type>(f);}) return Create<type>(f);
  else if constexpr(requires{type{f};}) return type{f};
  else return type{};
}
template<typename type> constexpr auto create_state(const auto& f, const auto& event) {
  if constexpr(requires{create<type>(f, event);}) return create<type>(f, event);
  else if constexpr(requires{Create<type>(f, event);}) return Create<type>(f, event);
  else if constexpr(requires{type{f, event};}) return type{f, event};
  else return create_object<type>(f);
}

template<typename type> constexpr bool test_var_in_state(const auto& v) {
  if constexpr(requires{holds_alternative<type>(v);}) return holds_alternative<type>(v);
  else return visit([]<typename inner>(const inner&){ return type_c<inner> == type_c<type>; }, v);
}

constexpr void call_on_exit(const auto& factory, auto& scenario, auto& state, const auto& event) {
  if constexpr(requires{scenario.on_exit(factory, state, event);}) scenario.on_exit(factory, state, event);
  else if constexpr(requires{scenario.onExit(factory, state, event);}) scenario.onExit(factory, state, event);
  else if constexpr(requires{on_exit(factory, scenario, state, event);}) on_exit(factory, scenario, state, event);
  else if constexpr(requires{on_exit(factory, scenario, state);}) on_exit(factory, scenario, state);
  else if constexpr(requires{onExit(factory, scenario, state, event);}) onExit(factory, scenario, state, event);
  else if constexpr(requires{OnExit(factory, scenario, state);}) OnExit(factory, scenario, state);
  // and without factory
  else if constexpr(requires{scenario.on_exit(state, event);}) scenario.on_exit(state, event);
  else if constexpr(requires{scenario.onExit(state, event);}) scenario.onExit(state, event);
  else if constexpr(requires{on_exit(scenario, state, event);}) on_exit(scenario, state, event);
  else if constexpr(requires{on_exit(scenario, state);}) on_exit(scenario, state);
  else if constexpr(requires{onExit(scenario, state, event);}) onExit(scenario, state, event);
  else if constexpr(requires{OnExit(scenario, state);}) OnExit(scenario, state);
}
constexpr void call_on_enter(const auto& factory, auto& scenario, auto& state, const auto& event) {
  if constexpr(requires{scenario.on_enter(factory, state, event);}) scenario.on_enter(factory, state, event);
  else if constexpr(requires{scenario.onEnter(factory, state, event);}) scenario.onEnter(factory, state, event);
  else if constexpr(requires{on_enter(factory, scenario, state, event);}) on_enter(factory, scenario, state, event);
  else if constexpr(requires{on_enter(factory, scenario, state);}) on_enter(factory, scenario, state);
  else if constexpr(requires{onEnter(factory, scenario, state, event);}) onEnter(factory, scenario, state, event);
  else if constexpr(requires{OnEnter(factory, scenario, state);}) OnEnter(factory, scenario, state);
// and without factory
  else if constexpr(requires{scenario.on_enter(state, event);}) scenario.on_enter(state, event);
  else if constexpr(requires{scenario.onEnter(state, event);}) scenario.onEnter(state, event);
  else if constexpr(requires{on_enter(scenario, state, event);}) on_enter(scenario, state, event);
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
  if constexpr(requires{erase_if(f, con, static_cast<decltype(fnc)&&>(fnc));}) erase_if(f, con, static_cast<decltype(fnc)&&>(fnc));
  else {
    static_assert( requires{con[0];}, "try to provide erase_if method for this container");
    for (auto i=0;i<con.size();++i) if (fnc(con[i])) erase(f, con, i--);
  }
}

template<typename scenario, typename transaction, typename next> constexpr void call_with_try_catch(const auto& f, auto&& fnc) {
  if constexpr(!requires{on_exception<scenario, transaction, next>(f);}) fnc();
#ifdef __EXCEPTIONS //TODO: remove it when fix https://github.com/llvm/llvm-project/issues/138939 (needed for compile visualizator)
  else {
    try{fnc();} catch (...){on_exception<scenario, transaction, next>(f);}
  }
#endif
}

template<auto at_least, auto cmd> constexpr bool call_if_need_not_enough_data(const auto& f, auto* buf, auto sz) {
  if (sz < at_least) {
    if constexpr(requires{on_not_enough_syn_data<cmd>(f, buf, sz);}) on_not_enough_syn_data<cmd>(f, buf, sz);
    return true;
  }
  return false;
}

}