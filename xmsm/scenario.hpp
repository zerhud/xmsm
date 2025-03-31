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
#include "scenario_checkers.hpp"
#include "declarations.hpp"

namespace xmsm {

template<typename _from, typename _to, typename _event, typename... _mods>
struct trans_info {
  consteval static auto all_stack_by_event() { return filter(type_list<_mods...>{}, [](auto v){return requires{decltype(+v){}.is_stack_by_event;};}); }
  consteval static auto all_stack_by_expression() { return filter(type_list<_mods...>{}, [](auto v){return requires{decltype(+v){}.is_stack_by_expression;};}); }
  consteval static auto all_mod_when() { return filter(type_list<_mods...>{}, [](auto v){return requires{decltype(+v){}.is_when;};}); }

  constexpr static bool is_trans_info = true;
  constexpr static auto to = type_c<_to>;
  constexpr static auto from = type_c<_from>;
  constexpr static auto event = type_c<_event>;
  constexpr static auto mods = type_list<_mods...>{};
  constexpr static auto mod_stack_by_event = first(all_stack_by_event());
  constexpr static auto mod_stack_by_expression = first(all_stack_by_expression());
  constexpr static auto mod_when = first(all_mod_when());

  static_assert( size(all_stack_by_event()) < 2, "only single stack by event modification is available for transition" );
};

template<typename state, auto max_event_count, typename expr>
struct stack_frame {
  state st;
  uint32_t back_event_non_zero_ids[max_event_count] = {};
  expr back_expression{};
};

struct fake_stack{ constexpr explicit fake_stack(const auto&){} };

template<typename factory, typename object, typename user_type=object>
struct scenario {
  friend constexpr auto mk_sm_description(const scenario&, auto&&... args) {
    return type_list<decltype(+type_dc<decltype(args)>)...>{};
  }
  template<typename from, typename to, typename event=void, typename... mods>
  friend constexpr auto mk_trans(const scenario&, auto&&... mods_obj) {
    constexpr auto mods_list = type_list<decltype(+type_dc<decltype(mods_obj)>)...>{};
    return unpack(mods_list, [](auto... s){return trans_info<from, to, event, decltype(+s)..., mods...>{};});
  }
  template<typename st> friend constexpr auto pick_def_state(const scenario&) { return modificators::def_state<st>{}; }
  template<typename... e> friend constexpr auto stack_by_event(const scenario&) { return modificators::stack_by_event<e...>{}; }
  template<typename s, typename... st> friend constexpr auto in(const scenario&){ return scenario_checker::in<s, st...>{}; }
  template<typename s, typename... st> friend constexpr auto now_in(const scenario&){ return scenario_checker::now_in<s, st...>{}; }
  template<typename e> friend constexpr auto stack_by_expr(const scenario&, e) { return modificators::stack_by_expression<e>{}; }
  template<typename e> friend constexpr auto when(const scenario&, e) { return modificators::when<e>{}; }

  using info = decltype(object::describe_sm(std::declval<scenario>()));
  constexpr static auto all_trans_info() ;
  constexpr static bool is_stack_with_event_required() { return unpack(all_trans_info(), [](auto... i){return (0 + ... + (decltype(+i)::mod_stack_by_event!=type_c<>)); }); }
  constexpr static bool is_stack_with_expression_required() { return unpack(all_trans_info(), [](auto... i){return (0 + ... + (decltype(+i)::mod_stack_by_expression!=type_c<>)); }); }
  constexpr static bool is_mod_on_requires() { return unpack(all_trans_info(), [](auto... i){return (0 + ... + (decltype(+i)::mod_on!=type_c<>)); }); }
  constexpr static bool is_mod_when_requires() { return unpack(all_trans_info(), [](auto... i){return (0 + ... + (decltype(+i)::mod_when!=type_c<>)); }); }
  constexpr static auto all_states() ;
  constexpr static auto all_events() ;
  constexpr static auto search(auto from, auto event) ;
  constexpr static auto mk_states_type(const auto& f) {
    return unpack(all_states(), [&]([[maybe_unused]] auto... states) {
      return mk_variant< decltype(+states)... >(f);
    });
  }
  constexpr static auto mk_stack_type(const auto& f) {
    if constexpr(!is_stack_with_event_required() && !is_stack_with_expression_required()) return fake_stack{f};
    else {
      constexpr auto list_of_expressions = unpack(all_trans_info(), [](auto&&... info){return (type_list{} << ... << [](auto i) {
        if constexpr(decltype(+i)::mod_stack_by_expression==type_c<>) return type_c<>;
        else return decltype(+decltype(+i)::mod_stack_by_expression)::expression;
      }(info));});
      constexpr auto expr_variant_type = unpack(list_of_expressions, [](auto&&... expr) {
        if constexpr (sizeof...(expr)==0) return type_c<int>;
        else return type_c<decltype(mk_variant< decltype(+expr)... >(std::declval<factory>()))>;
      });
      constexpr auto list_of_lists_back_events = unpack(all_trans_info(), [](auto&&... info){return (type_list{} + ... + type_c<decltype([](auto i) {
        if constexpr(decltype(+i)::mod_stack_by_event==type_c<>) return type_list{};
        else return type_list{} + decltype(+decltype(+i)::mod_stack_by_event)::back_events;
      }(info))> ); });
      return mk_list<stack_frame<decltype(mk_states_type(f)), [&] {
        //TODO: GCC15: we need to use a lambda here because structured binding cannot be constexpr
        auto [max,min] = unpack(list_of_lists_back_events, [](auto&&... i){ return max_min_size(decltype(+i){}...); });
        return max;
      }(), decltype(+expr_variant_type)>>(f);
    }
  }

  factory f;
  user_type obj;
  decltype(mk_states_type(std::declval<factory>())) state;
  [[no_unique_address]] decltype(mk_stack_type(std::declval<factory>())) stack;
  scenario_state _own_state{scenario_state::ready};

  constexpr explicit scenario(factory f) : f(std::move(f)), state(mk_states_type(this->f)), stack(mk_stack_type(this->f)) {}

  constexpr scenario_state own_state() const { return _own_state; }
  constexpr void reset_own_state() { _own_state=scenario_state::ready; }
  constexpr auto cur_state_hash() const {
    return visit([](const auto& s) { return hash<factory>(find(all_states(), type_dc<decltype(s)>)); }, cur_state());
  }
  template<typename self_type> constexpr auto&& cur_state(this self_type&& self) {
    if constexpr (!requires{self.stack.size();}) return self.state;
    else return 0 < self.stack.size() ? self.stack.back().st : self.state;
  }
  constexpr void on_other_scenarios_changed(const auto& e, auto&&... scenarios) {
    if constexpr(is_stack_with_expression_required()) clean_stack_by_expr(e, std::forward<decltype(scenarios)>(scenarios)...);
    if constexpr(is_mod_when_requires()) while (exec_when(e, std::forward<decltype(scenarios)>(scenarios)...));
  }
  constexpr void on(const auto& e) {
    constexpr auto e_type = type_dc<decltype(e)>;
    if constexpr (contains(all_events(), e_type)) {
      if constexpr(is_stack_with_event_required()) clean_stack(e);
      visit([&](auto& s) {
        constexpr auto s_type = type_dc<decltype(s)>;
        if constexpr (auto info_on_event = search(s_type, e_type); info_on_event != type_c<>) {
          change_state<decltype(+decltype(+info_on_event)::to), decltype(+info_on_event)>(e);
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
  template<typename next_type, typename trans_info> constexpr void change_state(const auto& e) {
    _own_state = scenario_state::broken;
    auto next = create_state<next_type>(f, e);
    visit([&](auto& s) {
      call_on_exit(obj, s, e);
      call_on_enter(obj, make_next_state<next_type>(trans_info{}, next), e);
    }, cur_state());
    _own_state = scenario_state::fired;
  }
  constexpr int exec_when(const auto& e, auto&&... scenarios) {
    auto cur_hash = cur_state_hash();
    return foreach(all_trans_info(), [&](auto t) {
      if constexpr(decltype(+t)::mod_when!=type_c<>) {
        if (cur_hash==hash<factory>(decltype(+t)::from) && t().mod_when().expression(scenarios...)) {
          change_state<decltype(+decltype(+t)::to), decltype(+t)>(e);
          return true;
        }
      }
      return false;
    });
  }
  constexpr auto clean_stack_by_expr(const auto& e, auto&&... scenarios) {
    auto check = [&](auto e) { return visit([&](auto expr){return expr(std::forward<decltype(scenarios)>(scenarios)...);}, e); };
    while (!stack.empty() && check(stack.back().back_expression)) {
      visit([&](auto& s) { call_on_exit(obj, s, e); }, stack.back().st);
      pop_back(stack);
      visit([&](auto& s) { call_on_enter(obj, s, e); }, cur_state());
    }
    for (auto i=0;i<stack.size();++i) if (check(stack[i].back_expression)) erase(f, stack, i--);
  }
  constexpr auto clean_stack(const auto& e) /* pre(contains(all_events(), type_dc<decltype(e)>)) */ {
    auto ind = hash<factory>(find(all_events(), type_dc<decltype(e)>));
    // contract_assert( ind >= 0 );
    auto check_contains = []<auto sz>(auto val, auto(&ar)[sz]){ bool found=false; for (auto i=0;i<sz;++i) found |= ar[i]==val; return found; };
    while (!stack.empty() && check_contains(ind, stack.back().back_event_non_zero_ids)) {
      visit([&](auto& s) { call_on_exit(obj, s, e); }, stack.back().st);
      pop_back(stack);
      visit([&](auto& s) { call_on_enter(obj, s, e); }, cur_state());
    }
    for (auto i=0;i<stack.size();++i) if (check_contains(ind, stack[i].back_event_non_zero_ids)) erase(f, stack, i--);
  }
  template<typename next_type> constexpr auto& make_next_state(auto tinfo, auto& next) {
    if constexpr (tinfo.mod_stack_by_event==type_c<>) return variant_emplace<next_type>(f, cur_state(), next);
    else {
      auto& ret = empace_back(stack, std::move(next));
      unpack(decltype(+tinfo.mod_stack_by_event)::back_events, [&](auto... events) {
        auto ind=-1;
        (void)( true && ... && (ret.back_event_non_zero_ids[++ind]=hash<factory>(find(all_events(), events)),true));
      });
      if constexpr (tinfo.mod_stack_by_expression!=type_c<>) ret.back_expression = decltype(+decltype(+tinfo.mod_stack_by_expression)::expression){};
      return get<next_type>(ret.st);
    }
  }
};

template<typename factory, typename object, typename user_type> constexpr auto scenario<factory, object, user_type>::all_events() {
  constexpr auto list = unpack(all_trans_info(), [](auto... states) {
    return (type_list{} << ... << [](auto st) {
      if constexpr (st.mod_stack_by_event==type_c<>) return st.event;
      else return type_list{} << st.event << decltype(+st.mod_stack_by_event)::back_events;
    }(decltype(+states){}));
  });
  static_assert( unpack(list, [](auto... i){return (true && ... && hash<factory>(i));}), "cannot correct calculate hash of some events (hash==0)" );
  if constexpr (constexpr auto dup_cnt = unpack(list, [&](auto... i){return has_duplicates(hash<factory>(i)...);}); dup_cnt!=0) {
    list.__has_duplicates();
    static_assert(dup_cnt ==0, "hash collision in events found" );
  }
  return list;
}

template<typename factory, typename object, typename user_type> constexpr auto scenario<factory, object, user_type>::all_trans_info() {
  return unpack(info{}, [](auto... info) {
    return (type_list<>{} << ... << [](auto item){
      if constexpr (requires{decltype(+item)::is_trans_info;}) return item;
      else return type_c<>;
    }(info));
  });
}

template<typename factory, typename object, typename user_type> constexpr auto scenario<factory, object, user_type>::all_states() {
  auto def = filter(info{}, [](auto info){if constexpr(requires{decltype(+info)::is_def_state;}) return decltype(+info)::st; else return type_c<>;});
  static_assert( size(def) < 2, "few default states was picked to scenario" );
  auto list = unpack(all_trans_info(), [&](auto... states){ return (((type_list<>{} << first(def)) << ... << decltype(+states)::from ) << ... << decltype(+states)::to); });
  static_assert( unpack(list, [](auto... i){return (true && ... && hash<factory>(i));}), "cannot correct calculate hash of some states (hash==0)" );
  static_assert( unpack(list, [&](auto... i){return has_duplicates(hash<factory>(i)...);})==0, "hash collision in states found" );
  return list;
}

template<typename factory, typename object, typename user_type> constexpr auto scenario<factory, object, user_type>::search(auto from, auto event) {
  constexpr auto found = filter(all_trans_info(), [&](auto info) {
    if constexpr (decltype(+info)::from == from && decltype(+info)::event == event) return info;
    else return type_c<>;
  });
  static_assert( size(found) < 2, "only single transition from state by event is possible" );
  if constexpr (size(found) == 0) return type_c<>;
  else return get<0>(found);
}

}