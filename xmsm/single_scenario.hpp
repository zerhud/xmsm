#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "basic_scenario.hpp"
#include "move_to.hpp"

namespace xmsm {

template<typename state, auto max_event_count, typename expr>
struct stack_frame {
  state st;
  uint32_t back_event_non_zero_ids[max_event_count] = {};
  expr back_expression{};
};

struct fake_stack{ constexpr explicit fake_stack(const auto&){} };

template<typename factory, typename object, typename user_type=object, typename others_list=type_list<>>
struct single_scenario : basic_scenario<factory, object> {
  enum trans_check_result { done=1, move_to_fail=2, only_if=4 };
  using base = basic_scenario<factory, object>;
  using info = base::info;

  using base::all_states;
  using base::all_events;
  using base::all_trans_info;

  constexpr static bool is_stack_with_event_required() { return unpack(all_trans_info(), [](auto... i){return (0 + ... + (decltype(+i)::mod_stack_by_event!=type_c<>)); }); }
  constexpr static bool is_stack_with_expression_required() { return unpack(all_trans_info(), [](auto... i){return (0 + ... + (decltype(+i)::mod_stack_by_expression!=type_c<>)); }); }
  constexpr static bool is_mod_on_requires() { return unpack(all_trans_info(), [](auto... i){return (0 + ... + (decltype(+i)::mod_on!=type_c<>)); }); }
  constexpr static bool is_mod_when_requires() { return unpack(all_trans_info(), [](auto... i){return (0 + ... + (decltype(+i)::mod_when!=type_c<>)); }); }
  constexpr static auto mk_states_type(const auto& f) {
    return unpack(all_states(), [&]([[maybe_unused]] auto... states) {
      static_assert( size(all_states()) == size(((type_list{}<<base::initial_state())<<...<<decltype(details::declval<base>().ch_type(states)){})), "all replaced type must to be unique" );
      return mk_variant< decltype(+details::declval<base>().ch_type(states))... >(f);
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
        else return type_c<decltype(mk_variant< decltype(+expr)... >(details::declval<factory>()))>;
      });
      constexpr auto list_of_lists_back_events = unpack(all_trans_info(), [](auto&&... info){return (type_list{} + ... + type_c<decltype([](auto i) {
        if constexpr(decltype(+i)::mod_stack_by_event==type_c<>) return type_list{};
        else return type_list{} + decltype(+decltype(+i)::mod_stack_by_event)::back_events;
      }(info))> ); });
      return mk_list<stack_frame<decltype(mk_states_type(f)), unpack(list_of_lists_back_events, [](auto&&... i){ return max_size(decltype(+i){}...); }), decltype(+expr_variant_type)>>(f);
    }
  }

  user_type obj;
  decltype(mk_states_type(details::declval<factory>())) state;
  [[no_unique_address]] decltype(mk_stack_type(details::declval<factory>())) stack;
  scenario_state _own_state : 7 {scenario_state::ready};
  bool synced  : 1 {true};
  [[no_unique_address]] decltype(mk_tracker<factory>(all_trans_info(), others_list{})) move_to_tracker;

  constexpr explicit single_scenario(factory f) : single_scenario((factory&&)(f), user_type{}) {}
  constexpr explicit single_scenario(factory f, user_type uo) : base((factory&&)(f)), obj((user_type&&)(uo)), state(mk_states_type(this->f)), stack(mk_stack_type(this->f)), move_to_tracker(mk_tracker<factory>(all_trans_info(), others_list{})) {}

  constexpr bool is_synced() const {return synced;}
  constexpr scenario_state own_state() const { return this->_own_state; }
  constexpr void reset_own_state() noexcept { _own_state = scenario_state::ready; }
  constexpr auto cur_state_hash() const {
    return unpack(all_states(), [&](auto... st) {
      constexpr static decltype(hash(first(all_states()))) hashes[] = {hash(st)...};
      return hashes[cur_state().index()];
    });
  }
  template<typename self_type> constexpr auto&& cur_state(this self_type&& self) {
    if constexpr (!requires{self.stack.size();}) return self.state;
    else return 0 < self.stack.size() ? self.stack.back().st : self.state;
  }
  constexpr void on_other_scenarios_changed(const auto& e, auto&&... scenarios) {
    move_to_tracker.update(this, e, scenarios...);
    if constexpr(is_stack_with_expression_required()) clean_stack_by_expr(e, scenarios...);
    if constexpr(is_mod_when_requires()) while (exec_when(e, scenarios...));
  }
  constexpr void on(const auto& e, auto&&... scenarios) {
    constexpr auto e_type = type_dc<decltype(e)>;
    if constexpr (contains(all_events(), e_type)) {
      if constexpr(is_stack_with_event_required()) clean_stack(e);
      auto cur_hash = cur_state_hash();
      foreach(all_trans_info(), [&](auto t) {
        const bool match = cur_hash==hash(decltype(+t)::from) && decltype(+t)::event <= e_type;
        return match && (exec_trans<decltype(+t)>(e, scenarios...)&trans_check_result::only_if)==0;
      });
    }
  }
  constexpr auto stack_size() const { if constexpr (is_stack_with_event_required()) return stack.size(); else return 0; }
  constexpr auto index() const { return cur_state().index(); }
  template<typename type> constexpr friend bool in_state(const single_scenario& s) { return s.cur_state().index() == index_of(all_states(), type_dc<type>); }
  template<typename type> constexpr friend bool in_state(const single_scenario& s, _type_c<type>) { return s.cur_state().index() == index_of(all_states(), type_dc<type>); }
  template<typename... list> constexpr friend bool in_state(const single_scenario& s, const type_list<list...>&) {
    return (0+...+(s.cur_state().index() == index_of(all_states(), type_dc<list>)));
  }
  template<typename type> constexpr bool in_state() const { return cur_state().index() == index_of(all_states(), type_dc<type>); }
  constexpr static auto states_count() { return size(all_states()); }
  constexpr static auto events_count() { return size(all_events()); }
  template<typename target> constexpr void force_move_to(const auto& e, auto&&... scenarios) {
    if (!move_to<target>(e, scenarios...)) {
      const bool found_trans = foreach(all_trans_info(), [&](auto t) {
        const bool match = decltype(+t)::from==type_c<> && decltype(+t)::event==type_c<> && decltype(+t)::to <= type_c<target>;
        return match && exec_trans<decltype(+t)>(e, scenarios...)==trans_check_result::done;
      });
      if (!found_trans) change_state<target>(e);
    }
  }
  template<typename... target> constexpr bool move_to(const auto& e, auto&&... scenarios) {
    auto cur_hash = cur_state_hash();
    return foreach(all_trans_info(), [&](auto t) {
      bool match = cur_hash==hash(decltype(+t)::from) && ((type_c<target> == decltype(+t)::to) || ...) && decltype(+t)::is_queue_allowed && decltype(+t)::is_move_allowed;
      return match && exec_trans<decltype(+t)>(e, scenarios...)==trans_check_result::done;
    });
  }
  template<typename... target> constexpr bool move_to_or_wait(const auto& e, auto&&... scenarios) {
    return move_to_or_wait_cond(e, [](auto to)->bool{return (0+...+(type_c<target> == to));}, static_cast<decltype(scenarios)&&>(scenarios)...);
  }
  constexpr bool move_to_or_wait_cond(const auto& e, auto&& fnc, auto&&...scenarios) {
    return base::transactions_for_move_to(cur_state_hash(), fnc, [&](auto t) {
      return exec_trans<decltype(+t)>(e, scenarios...)==trans_check_result::done;
    });
  }
private:
  template<typename trans_info> constexpr bool handle_try_move_to(const auto& e, auto&&... scenarios) {
    foreach(trans_info::mod_try_move_to, [&](auto mt) {
      ([&](auto& s) {
        bool f = s.own_hash() == hash(mt().scenario); //TODO: we cannot use base class as target to pick up a derived class
        constexpr auto targets = all_targets_for_move_to(s.all_trans_info(), mt().state);
        if (f) size(targets)==0 || unpack(targets, [&](auto... tgts){return s.template move_to<decltype(+tgts)...>(e, scenarios...);});
      }(scenarios),...);
      return false;
    });
    return true;
  }
  template<typename trans_info> constexpr bool handle_move_to(const auto& e, auto&&... scenarios) {
    return !foreach(trans_info::mod_move_to, [&](auto mt) {
      constexpr auto mod = mt();
      bool fail = false;
      move_to_tracker.activate(mt, scenarios...);
      const int found = (0 + ... + [&](auto& s) {
        bool f = mod.scenario <= s.origin();
        constexpr auto targets = all_targets_for_move_to(s.all_trans_info(), mod.state);
        if (f) fail |= size(targets)==0 || !unpack(targets, [&](auto... tgts){return s.template move_to_or_wait<decltype(+tgts)...>(e, scenarios...);});
        return f;
      }(scenarios));
      if constexpr(requires{move_to_required_but_not_found(this->f, mod.scenario);}) if(!found) move_to_required_but_not_found(this->f, mod.scenario);
      if (fail || !found || !move_to_tracker.is_active()) {
        force_move_to<decltype(+mod.fail_state)>(e, scenarios...);
        move_to_tracker.update(this, e, scenarios...);
      }
      return !(!fail && found);
    });
  }
  template<typename _next_type, typename trans_info=trans_info<void,void,void>> constexpr void change_state(const auto& e) {
    _own_state = scenario_state::broken;
    using next_type = decltype(+this->ch_type(type_c<_next_type>));
    call_with_try_catch<object, trans_info, _next_type>(this->f, [&] {
      auto next = create_state<next_type>(this->f, e);
      visit([&](auto& s) { call_on_exit(this->f, obj, s, e); }, cur_state());
      call_on_enter(this->f, obj, make_next_state<next_type>(trans_info{}, next), e);
    });
    _own_state = scenario_state::fired;
  }
  template<typename info> constexpr trans_check_result exec_trans(const auto& e, auto&&... scenarios) {
    if constexpr(info::mod_only_if!=type_c<>) if (!info::mod_only_if().expression(scenarios...)) return trans_check_result::only_if;
    if (!handle_move_to<info>(e, scenarios...)) return trans_check_result::move_to_fail;
    change_state<decltype(+info::to), info>(e);
    handle_try_move_to<info>(e, scenarios...);
    return trans_check_result::done;
  }
  constexpr int exec_when(const auto& e, auto&&... scenarios) {
    auto cur_hash = cur_state_hash();
    return foreach(all_trans_info(), [&](auto t) {
      if constexpr(decltype(+t)::mod_when!=type_c<>) {
        if (cur_hash==hash(decltype(+t)::from) && t().mod_when().expression(scenarios...)) {
          return (exec_trans<decltype(+t)>(e, scenarios...) & trans_check_result::only_if)==0;
        }
      }
      return false;
    });
  }
  constexpr auto clean_stack_by_expr(const auto& e, auto&&... scenarios) {
    auto check = [&](auto&& e) { return visit([&](auto&& expr){return expr(static_cast<decltype(scenarios)&&>(scenarios)...);}, e); };
    pop_stack(e, [&](auto& frame){return check(frame.back_expression);});
  }
  constexpr auto clean_stack(const auto& e) /* pre(contains(all_events(), type_dc<decltype(e)>)) */ {
    auto ind = hash(find(all_events(), type_dc<decltype(e)>));
    // contract_assert( ind >= 0 );
    auto check_contains = []<auto sz>(auto val, auto(&ar)[sz]){ bool found=false; for (auto i=0;i<sz;++i) found |= ar[i]==val; return found; };
    pop_stack(e, [&](auto& frame){return check_contains(ind, frame.back_event_non_zero_ids);});
  }
  template<typename next_type> constexpr auto& make_next_state(auto tinfo, auto& next) {
    synced = false;
    constexpr bool is_stack_required = tinfo.mod_stack_by_event==type_c<> && tinfo.mod_stack_by_expression==type_c<>;
    if constexpr (is_stack_required) return variant_emplace<next_type>(this->f, cur_state(), next);
    else {
      auto& ret = xmsm_emplace_back(stack, details::move(next));
      if constexpr(tinfo.mod_stack_by_event!=type_c<>) unpack(decltype(+tinfo.mod_stack_by_event)::back_events, [&](auto... events) {
        auto ind=-1;
        (void)( true && ... && (ret.back_event_non_zero_ids[++ind]=hash(find(all_events(), events)),true));
      });
      if constexpr (tinfo.mod_stack_by_expression!=type_c<>) ret.back_expression = decltype(+decltype(+tinfo.mod_stack_by_expression)::expression){};
      return get<next_type>(ret.st);
    }
  }
  constexpr void pop_stack(const auto& e, auto&& check) {
    while (!stack.empty() && check(stack.back())) {
      visit([&](auto& s) { call_on_exit(this->f, obj, s, e); }, stack.back().st);
      pop_back(stack);
      visit([&](auto& s) { call_on_enter(this->f, obj, s, e); }, cur_state());
    }
    for (auto i=0;i<stack.size();++i) if (check(stack[i])) erase(this->f, stack, i--);
  }
};

}
