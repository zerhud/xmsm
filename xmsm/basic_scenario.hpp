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
  consteval static auto all_mod_only_if() { return filter(type_list<_mods...>{}, [](auto v){return requires{decltype(+v){}.is_only_if;};}); }
  consteval static auto all_mod_move_to() { return filter(type_list<_mods...>{}, [](auto v){return requires{decltype(+v){}.is_move_to;};}); }
  consteval static auto all_mod_try_move_to() { return filter(type_list<_mods...>{}, [](auto v){return requires{decltype(+v){}.is_try_move_to;};}); }

  constexpr static bool is_trans_info = true;
  constexpr static bool is_queue_allowed = []{return size(filter(type_list<_mods...>{}, [](auto v){return v == type_c<modificators::allow_queue>;})) > 0;}();
  constexpr static bool is_move_allowed = []{return size(filter(type_list<_mods...>{}, [](auto v){return v == type_c<modificators::allow_move>;})) > 0;}();
  constexpr static auto to = type_c<_to>;
  constexpr static auto from = type_c<_from>;
  constexpr static auto event = type_c<_event>;
  constexpr static auto mods = type_list<_mods...>{};
  constexpr static auto mod_stack_by_event = first(all_stack_by_event());
  constexpr static auto mod_stack_by_expression = first(all_stack_by_expression());
  constexpr static auto mod_when = first(all_mod_when());
  constexpr static auto mod_only_if = first(all_mod_only_if());
  constexpr static auto mod_move_to = all_mod_move_to();
  constexpr static auto mod_try_move_to = all_mod_try_move_to();

  static_assert( size(all_stack_by_event()) < 2, "only single stack by event modification is available for transition" );
};

template<typename... adding, typename _from, typename _to, typename _event, typename... _mods>
constexpr auto add_mods(_type_c<trans_info<_from, _to, _event, _mods...>>, const type_list<adding...>&) {
  return type_c<trans_info<_from, _to, _event, _mods..., adding...>>;
}

template<typename factory, typename object> struct basic_scenario {
  struct multi_sm_indicator;
  factory f;

  constexpr static auto own_hash() { return hash<factory>(type_c<object>); }

  constexpr explicit basic_scenario(factory f) : f(std::move(f)) {}

  friend constexpr auto mk_sm_description(const basic_scenario&, auto&&... args) {
    return type_list<decltype(+type_dc<decltype(args)>)...>{};
  }
  friend constexpr auto mk_multi_sm_description(const basic_scenario&, auto&&... args) {
    return type_list<multi_sm_indicator, decltype(+type_dc<decltype(args)>)...>{};
  }
  template<typename from, typename to, typename event=void, typename... mods>
  friend constexpr auto mk_trans(const basic_scenario&, auto&&... mods_obj) {
    constexpr auto mods_list = type_list<decltype(+type_dc<decltype(mods_obj)>)...>{};
    return unpack(mods_list, [](auto... s){return trans_info<from, to, event, decltype(+s)..., mods...>{};});
  }
  template<typename from, typename to, typename event=void, typename... mods>
  friend constexpr auto mk_qtrans(const basic_scenario& s, auto&&... mods_obj) { return mk_trans<from, to, event, mods...>(s, modificators::allow_queue{}, std::forward<decltype(mods_obj)>(mods_obj)...); }
  template<typename st> friend constexpr auto pick_def_state(const basic_scenario&) { return modificators::def_state<st>{}; }
  template<typename... e> friend constexpr auto stack_by_event(const basic_scenario&) { return modificators::stack_by_event<e...>{}; }
  friend constexpr auto _true(const basic_scenario&){ return scenario_checker::_true{}; }
  friend constexpr auto _false(const basic_scenario&){ return scenario_checker::_false{}; }
  template<typename s, typename... st> friend constexpr auto in(const basic_scenario&){ return scenario_checker::in<s, st...>{}; }
  template<typename s, typename... st> friend constexpr auto now_in(const basic_scenario&){ return scenario_checker::now_in<s, st...>{}; }
  template<typename s, typename... st> friend constexpr auto all_in(const basic_scenario&){ return scenario_checker::all_in<s, st...>{}; }
  template<typename s, auto cnt, typename... st> friend constexpr auto cnt_in(const basic_scenario&){ return scenario_checker::count_in<s, cnt, st...>{}; }
  template<typename s> friend constexpr auto affected(const basic_scenario&){ return scenario_checker::affected<s>{}; }
  template<typename e> friend constexpr auto stack_by_expr(const basic_scenario&, e) { return modificators::stack_by_expression<e>{}; }
  template<typename e> friend constexpr auto when(const basic_scenario&, e) { return modificators::when<e>{}; }
  template<typename e> friend constexpr auto only_if(const basic_scenario&, e) { return modificators::only_if<e>{}; }
  template<typename st, typename... _mods> friend constexpr auto to_state_mods(const basic_scenario&, _mods...) { return modificators::to_state_mods<st, _mods...>{}; }
  template<typename st, typename... _mods> friend constexpr auto from_state_mods(const basic_scenario&, _mods...) { return modificators::from_state_mods<st, _mods...>{}; }
  template<typename st> friend constexpr auto finish_state(const basic_scenario&){ return modificators::finish_state<st>{}; }
  template<typename event> friend constexpr auto start_event(const basic_scenario&){ return modificators::start_event<event>{}; }
  friend constexpr auto allow_queue(const basic_scenario&) { return modificators::allow_queue{}; }
  friend constexpr auto allow_move(const basic_scenario&) { return modificators::allow_move{}; }
  template<typename sc, typename st, typename fst> friend constexpr auto move_to(const basic_scenario&) { return modificators::move_to<sc, st, fst>{}; }
  template<typename sc, typename st> friend constexpr auto try_move_to(const basic_scenario&) { return modificators::try_move_to<sc, st>{}; }
  template<typename type> friend constexpr auto mk_change(const basic_scenario&) { return type_c<type>; }

  using info = decltype(object::describe_sm(std::declval<basic_scenario>()));

  constexpr static bool is_multi() { return first(info{}) == type_c<multi_sm_indicator>; }
  constexpr static bool is_finish_state(auto st) { return unpack(info{}, [](auto... i){return (0+...+[](auto i) {
    if constexpr (requires{i().is_finish_state;}) return i().st==decltype(st){};
    else return 0;
  }(i));});}

  constexpr auto ch_type(auto from) const {
    if constexpr(!requires{ change_type<int>(f, *this); }) return from;
    else if constexpr(is_type_c<decltype(change_type<decltype(+from)>(f, *this))>)
      return change_type<decltype(+from)>(f, *this);
    else return from;
  }

  constexpr static auto all_trans_info() {
    constexpr auto tlist = unpack(info{}, [](auto... info) {
      return (type_list{} << ... << [](auto item) {
        if constexpr (requires { decltype(+item)::is_trans_info; }) return item;
        else return type_c<>;
      }(info));
    });
    constexpr auto to_list = filter(info{}, [](auto i) { return requires { i().is_to_state_mods; }; });
    constexpr auto from_list = filter(info{}, [](auto i) { return requires { i().is_from_state_mods; }; });
    constexpr auto fail_states = unpack(info{}, [](auto... i){ return (type_list{} << ... << [](auto i) {
      if constexpr(!requires{decltype(+i)::is_trans_info;}) return type_c<>;
      else if constexpr(size(decltype(+i)::mod_move_to)==0) return type_c<>;
      else return unpack(i().mod_move_to, [](auto... mt){return (type_list{}<<...<<mt().fail_state);});
    }(i));});
    constexpr auto user_ti = unpack(tlist, [&](auto... transitions) {
      return (type_list{} << ... << [&](auto cur) {
        constexpr auto cur_to_list = filter(to_list, [&](auto to) { return decltype(+to)::st == decltype(+cur)::to; });
        constexpr auto cur_from_list = filter(from_list, [&](auto to) {
          return decltype(+to)::st == decltype(+cur)::from;
        });
        auto mods = [](auto l) { return unpack(l, [](auto... i) { return (type_list{} << ... << i().mods); }); };
        return add_mods(add_mods(cur, mods(cur_to_list)), mods(cur_from_list));
      }(transitions));
    });
    return user_ti << unpack(fail_states, [](auto... fs) {
      constexpr auto mk_ti = [](auto fail_st) {
        constexpr auto mk_ti = [](auto from_st, auto info) {
          return unpack(info().mods, [](auto... mods) {
            return type_c<trans_info<decltype(+from_st), decltype(+fail_st), void, decltype(+mods)...>>;
          });
        };
        constexpr auto with_from = unpack(decltype(from_list){}, [&](auto... from_st){return(type_list{}<<...<<mk_ti(from_st().st, from_st));});
        constexpr auto with_to = unpack(decltype(to_list){}, [&](auto...to_st){return(type_list{}<<...<<mk_ti(type_c<>,to_st));});
        return with_from << with_to;
      };
      return (type_list() << ... << mk_ti(fs));
    });
  }
};

}
