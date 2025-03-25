#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "hana.hpp"
#include "customizations.hpp"

namespace xmsm {

template<typename _from, typename _to, typename _event, typename... mods>
struct trans_info {
  constexpr static bool is_trans_info = true;
  constexpr static auto to = type_c<_to>;
  constexpr static auto from = type_c<_from>;
  constexpr static auto event = type_c<_event>;
};
template<typename type> struct def_state {
  constexpr static bool is_def_state = true;
  constexpr static auto st = type_c<type>;
};

template<typename factory, typename object>
struct scenario {
  friend constexpr auto mk_sm_description(const scenario&, auto&&... args) {
    return type_list<decltype(+type_dc<decltype(args)>)...>{};
  }
  template<typename from, typename to, typename event, typename... mods>
  friend constexpr auto mk_trans(const scenario&) {
    return trans_info<from, to, event, mods...>{};
  }
  template<typename st> friend constexpr auto pick_def_state(const scenario&) {
    return def_state<st>{};
  }

  using info = decltype(object::describe_sm(std::declval<scenario>()));
  constexpr static auto all_trans_info() ;
  constexpr static auto all_states() ;
  constexpr static auto all_events() ;
  constexpr static auto search(auto from, auto event) ;
  constexpr static auto mk_states_type() {
    return unpack(all_states(), [](auto... states) {
      return type_c<typename factory::template variant_t< decltype(+states)... >>;
    });
  }

  factory f;
  object obj;
  decltype(+mk_states_type()) state;

  constexpr explicit scenario(factory f) : f(std::move(f)) {}

  constexpr void on(const auto& e) {
    constexpr auto e_type = type_dc<decltype(e)>;
    if constexpr (contains(all_events(), e_type)) visit([&](auto& s) {
      constexpr auto info = search(type_dc<decltype(s)>, e_type);
      if constexpr (info != type_c<>) {
        using next_type = decltype(+decltype(+info)::to);
        auto next = create_state<next_type>(f, e);
        call_on_exit(obj, s, e);
        call_on_enter(obj, variant_emplace<next_type>(f, state, next), e);
      }
    }, state);
  }
  constexpr auto index() const {
    return state.index();
  }
  template<typename type> constexpr bool in_state() const {
    return test_var_in_state<type>(state);
  }
  constexpr static auto states_count() {
    return size(all_states());
  }
  constexpr static auto events_count() {
    return size(all_events());
  }
};

template<typename factory, typename object> constexpr auto scenario<factory, object>::all_events() {
  return unpack(all_trans_info(), [](auto... states){ return (type_list<>{} << ... << decltype(+states)::event); });
}

template<typename factory, typename object> constexpr auto scenario<factory, object>::all_trans_info() {
  return unpack(info{}, [](auto... info) {
    return (type_list<>{} << ... << [](auto item){
      if constexpr (requires{decltype(+item)::is_trans_info;}) return item;
      else return type_c<>;
    }(info));
  });
}

template<typename factory, typename object> constexpr auto scenario<factory, object>::all_states() {
  auto def = filter(info{}, [](auto info){if constexpr(requires{decltype(+info)::is_def_state;}) return decltype(+info)::st; else return type_c<>;});
  static_assert( size(def) < 2, "few default states was picked to scenario" );
  return unpack(all_trans_info(), [&](auto... states){ return (((type_list<>{} << first(def)) << ... << decltype(+states)::from ) << ... << decltype(+states)::to); });
}

template<typename factory, typename object> constexpr auto scenario<factory, object>::search(auto from, auto event) {
  constexpr auto found = filter(all_trans_info(), [&](auto info) {
    if constexpr (decltype(+info)::from == from && decltype(+info)::event == event) return info;
    else return type_c<>;
  });
  static_assert( size(found) < 2, "only single transition from state by event is possible" );
  if constexpr (size(found) == 0) return type_c<>;
  else return get<0>(found);
}

}