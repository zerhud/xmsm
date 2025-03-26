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
#include "modificators.hpp"

namespace xmsm {

template<typename _from, typename _to, typename _event, typename... _mods>
struct trans_info {
  consteval static auto all_stack_by_event() { return filter(type_list<_mods...>{}, [](auto v){return requires{decltype(+v){}.is_stack_by_event;};}); }

  constexpr static bool is_trans_info = true;
  constexpr static auto to = type_c<_to>;
  constexpr static auto from = type_c<_from>;
  constexpr static auto event = type_c<_event>;
  constexpr static auto mods = type_list<_mods...>{};
  constexpr static auto mod_stack_by_event = first(all_stack_by_event());

  static_assert( size(all_stack_by_event()) < 2, "only single stack by event modification is available for transition" );
};

template<typename state, auto max_event_count>
struct stack_frame {
  state st;
  int back_event_non_zero_inds[max_event_count] = {};
};

struct fake_stack{ constexpr explicit fake_stack(const auto&){} };

template<typename factory, typename object>
struct scenario {
  friend constexpr auto mk_sm_description(const scenario&, auto&&... args) {
    return type_list<decltype(+type_dc<decltype(args)>)...>{};
  }
  template<typename from, typename to, typename event, typename... mods>
  friend constexpr auto mk_trans(const scenario&, auto&&... mods_obj) {
    constexpr auto mods_list = type_list<decltype(+type_dc<decltype(mods_obj)>)...>{};
    return unpack(mods_list, [](auto... s){return trans_info<from, to, event, decltype(+s)..., mods...>{};});
  }
  template<typename st> friend constexpr auto pick_def_state(const scenario&) {
    return modificators::def_state<st>{};
  }
  template<typename... e> friend constexpr auto stack_by_event(const scenario&) {
    return modificators::stack_by_event<e...>{};
  }

  using info = decltype(object::describe_sm(std::declval<scenario>()));
  constexpr static auto all_trans_info() ;
  constexpr static bool is_stack_with_event_required() { return unpack(all_trans_info(), [](auto... i){return (0 + ... + (decltype(+i)::mod_stack_by_event!=type_c<>)); }); }
  constexpr static auto all_states() ;
  constexpr static auto all_events() ;
  constexpr static auto search(auto from, auto event) ;
  constexpr static auto mk_states_type(const auto& f) {
    return unpack(all_states(), [&]([[maybe_unused]] auto... states) {
      return mk_variant< decltype(+states)... >(f);
    });
  }
  constexpr static auto mk_stack_type(const auto& f) {
    if constexpr(!is_stack_with_event_required()) return fake_stack{f};
    else {
      constexpr auto list_of_lists_back_events = unpack(all_trans_info(), [](auto&&... info){return (type_list{} + ... + type_c<decltype([](auto i) {
        if constexpr(decltype(+i)::mod_stack_by_event==type_c<>) return type_list{};
        else return type_list{} + decltype(+decltype(+i)::mod_stack_by_event)::back_events;
      }(info))> ); });
      return mk_list<stack_frame<decltype(mk_states_type(f)), [&] {
        //TODO: GCC15: we need to use a lambda here because structured binding cannot be constexpr
        auto [max,min] = unpack(list_of_lists_back_events, [](auto&&... i){ return max_min_size(decltype(+i){}...); });
        return max;
      }()>>(f);
    }
  }

  factory f;
  object obj;
  decltype(mk_states_type(std::declval<factory>())) state;
  [[no_unique_address]] decltype(mk_stack_type(std::declval<factory>())) stack;

  constexpr explicit scenario(factory f) : f(std::move(f)), state(mk_states_type(this->f)), stack(mk_stack_type(this->f)) {}

  template<typename self_type> constexpr auto&& cur_state(this self_type&& self) {
    if constexpr (!requires{self.stack.size();}) return self.state;
    else return 0 < self.stack.size() ? self.stack.back().st : self.state;
  }
  constexpr void on(const auto& e) {
    constexpr auto e_type = type_dc<decltype(e)>;
    if constexpr (contains(all_events(), e_type)) {
      if constexpr(is_stack_with_event_required()) clean_stack(e);
      visit([&](auto& s) {
        constexpr auto s_type = type_dc<decltype(s)>;
        if constexpr (auto info_on_event = search(s_type, e_type); info_on_event != type_c<>) {
          using next_type = decltype(+decltype(+info_on_event)::to);
          auto next = create_state<next_type>(f, e);
          call_on_exit(obj, s, e);
          call_on_enter(obj, make_next_state<next_type>(decltype(+info_on_event){}, next), e);
        }
      }, cur_state());
    }
  }
  constexpr auto stack_size() const { if constexpr (is_stack_with_event_required()) return stack.size(); else return 0; }
  constexpr auto index() const { return cur_state().index(); }
  template<typename type> constexpr bool in_state() const { return test_var_in_state<type>(cur_state()); }
  constexpr static auto states_count() { return size(all_states()); }
  constexpr static auto events_count() { return size(all_events()); }
private:
  constexpr auto clean_stack(const auto& e) /* pre(contains(all_events(), type_dc<decltype(e)>)) */ {
    auto ind = index_of(all_events(), type_dc<decltype(e)>) + 1;
    // contract_assert( ind >= 0 );
    auto check_contains = []<auto sz>(auto val, auto(&ar)[sz]){ bool found=false; for (auto i=0;i<sz;++i) found |= ar[i]==val; return found; };
    while (!stack.empty() && check_contains(ind, stack.back().back_event_non_zero_inds)) {
      visit([&](auto& s) { call_on_exit(obj, s, e); }, stack.back().st);
      pop_back(stack);
      visit([&](auto& s) { call_on_enter(obj, s, e); }, cur_state());
    }
    for (signed i=0;i<stack.size();++i) if (check_contains(ind, stack[i].back_event_non_zero_inds)) erase(f, stack, i--);
  }
  template<typename next_type> constexpr auto& make_next_state(auto tinfo, auto& next) {
    if constexpr (tinfo.mod_stack_by_event==type_c<>) return variant_emplace<next_type>(f, cur_state(), next);
    else {
      auto& ret = empace_back(stack, std::move(next));
      unpack(decltype(+tinfo.mod_stack_by_event)::back_events, [&](auto... events) {
        auto ind=-1;
        (void)( true && ... && (ret.back_event_non_zero_inds[++ind]=1+index_of(all_events(), events),true));
      });
      return ret;
    }
  }
};

template<typename factory, typename object> constexpr auto scenario<factory, object>::all_events() {
  return unpack(all_trans_info(), [](auto... states) {
    return (type_list{} << ... << [](auto st) {
      if constexpr (st.mod_stack_by_event==type_c<>) return st.event;
      else return type_list{} << st.event << decltype(+st.mod_stack_by_event)::back_events;
    }(decltype(+states){}));
  });
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