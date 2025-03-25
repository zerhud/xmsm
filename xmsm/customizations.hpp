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
  if constexpr(requires{on_exit(scenario, state, event);}) on_exit(scenario, state, event);
  else if constexpr(requires{on_exit(scenario, state);}) on_exit(scenario, state);
  else if constexpr(requires{onExit(scenario, state, event);}) onExit(scenario, state, event);
  else if constexpr(requires{OnExit(scenario, state);}) OnExit(scenario, state);
}
constexpr void call_on_enter(auto& scenario, auto& state, const auto& event) {
  if constexpr(requires{on_enter(scenario, state, event);}) on_enter(scenario, state, event);
  else if constexpr(requires{on_enter(scenario, state);}) on_enter(scenario, state);
  else if constexpr(requires{onEnter(scenario, state, event);}) onEnter(scenario, state, event);
  else if constexpr(requires{OnEnter(scenario, state);}) OnEnter(scenario, state);
}

}