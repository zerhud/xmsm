#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "hana.hpp"
#include "utils.hpp"

namespace xmsm {

struct allow_queue;
namespace move_to_helpers {
template<typename tt> constexpr auto trans_info_to(auto tinfo, _type_c<tt>) { return filter(tinfo, [](auto i){return decltype(+i)::to == type_c<tt>;}); }
template<typename tt> constexpr auto filter_by_target(auto tinfo, _type_c<tt>, auto tail) {
  auto selected = trans_info_to<tt>(tinfo, type_c<tt>);
  if constexpr (size(selected)==0) return type_list<decltype(tail)>{};
  else return unpack(selected, [&](auto... s) {
    return type_list<decltype(type_list<decltype(+decltype(+s)::from)>{} << tail)...>{};
  });
}
constexpr auto filter_by_target(auto tinfo, auto path) {
  auto iter = filter_by_target(tinfo, first(path), path);
  if constexpr(size(iter)==1 && first(iter)()==path) return iter;
  else return unpack(iter, [&](auto... pathes){return (type_list{} << ... << [&](auto cur) {
    return filter_by_target(tinfo, cur);
  }(pathes()));});
}
}

template<typename target_t> constexpr auto mk_allow_queue_st(auto tinfo, _type_c<target_t> target) {
  constexpr auto list = filter(tinfo, [](auto i){return decltype(+i)::is_queue_allowed;});
  constexpr auto to_target = filter(list, [](auto i){return decltype(+i)::to == type_c<target_t>;});
  constexpr auto to_target_st = unpack(to_target, [](auto... t){return type_list<type_list<decltype(+decltype(+t)::to)>...>{};});
  return unpack(to_target_st, [&](auto... pathes){return (type_list{} << ... << move_to_helpers::filter_by_target(list, pathes()));});
}

template<typename target_t>  constexpr auto all_targets_for_move_to(auto tinfo, _type_c<target_t> target) {
  constexpr auto src = mk_allow_queue_st(tinfo, target);
  return unpack(src, [](auto... seq){return (type_list{} << ... << pop_front(seq()));});
}

template<typename factory, typename list, typename scenario, typename target_state, typename fail_state>
struct state_queue_tracker {
  int indexes[size(list{})];

  constexpr state_queue_tracker() { deactivate(); }

  constexpr bool deactivate() {return [this]<auto...inds>(std::index_sequence<inds...>){return ((indexes[inds]=-1)+...);}(std::make_index_sequence<size(list{})>{});}
  constexpr bool is_active() const {return [this]<auto...inds>(std::index_sequence<inds...>){return ((indexes[inds]>=0)+...);}(std::make_index_sequence<size(list{})>{});}
  consteval static bool is_valid() {return size(list{})>0;}

  constexpr static auto get_state(auto st_list, auto ind) {
    return [&]<auto...inds>(std::index_sequence<inds...>){
      return (0+...+((inds==ind)*hash<factory>(get<inds>(st_list))));
    }(std::make_index_sequence<size(st_list)>{});
  }
  constexpr void activate(auto cur_hash) {
    if (is_active()) return;
    if (cur_hash==hash<factory>(last(first(list{})()))) {deactivate(); return;}
    [&]<auto...inds>(std::index_sequence<inds...>){
      (void) (0 + ... + [&](auto ind, auto st_list) {
        return indexes[ind] = index_of_by_hash<factory>(st_list(), cur_hash);
      }(inds, get<inds>(list{})));
    }(std::make_index_sequence<size(list{})>{});
  }
  constexpr bool check_and_shift_state(auto cur_hash) {
    if (!is_active()) return true;
    if (cur_hash==hash<factory>(last(first(list{})()))) {deactivate(); return true;}
    return [&]<auto...inds>(std::index_sequence<inds...>){
      return (0 + ... + [&](auto ind, auto st_list) {
        if (indexes[ind]<0) return false;
        const bool is_next_state = get_state(st_list(), indexes[ind]+1)==cur_hash;
        indexes[ind] += is_next_state;
        return is_next_state || get_state(st_list(), indexes[ind])==cur_hash ;
      }(inds, get<inds>(list{})));
    }(std::make_index_sequence<size(list{})>{});
  }
  template<typename tgt_sc, typename tgt_st, typename fail_st> constexpr state_queue_tracker* search()
  requires(type_c<tgt_sc> == type_c<scenario> && type_c<tgt_st> == type_c<target_state> && type_c<fail_st> == type_c<fail_state>){ return this; }
  constexpr void update(auto* cur_scenario, const auto& e, auto&&... others) {
    auto scenario_cur_hash  = utils::cur_state_hash_from_set<factory, scenario>(others...);
    if (!check_and_shift_state(scenario_cur_hash)) cur_scenario->template force_move_to<fail_state>(e, others...);
  }
};

template<typename factory,typename base> struct final_tracker : base {
  using bases = base::bases;
  constexpr bool is_active() const { return unpack(bases{}, [this](auto... b){return (static_cast<const decltype(+b)*>(this)->is_active() + ...);});}
  constexpr void update(auto* cur_scenario, const auto& e, auto&&... others) {
    foreach(bases{}, [&](auto cur_base){static_cast<decltype(+cur_base)*>(this)->update(cur_scenario, e, others...);return false;});
  }
  constexpr void activate(auto mt, auto&&... scenarios) { if constexpr(requires{this->template search<decltype(+mt().scenario), decltype(+mt().state), decltype(+mt().fail_state)>();}) {
    this->template search<decltype(+mt().scenario), decltype(+mt().state), decltype(+mt().fail_state)>()->activate(
      (0+...+(scenarios.cur_state_hash()*(hash<factory>(mt().scenario)==scenarios.own_hash())))
    );
  }}
};

struct fake_state_queue_tracker {
  constexpr static bool is_active() { return false; }
  constexpr static void update(auto&&...){}
  constexpr static void activate(auto mt, auto&&... scenarios) {}
};

template<typename factory, typename... others>
constexpr auto mk_tracker(auto target_info) {
  constexpr auto move_to_list = unpack(target_info, [](auto... i){return (type_list{} << ... << decltype(+i)::mod_move_to);});
  if constexpr(size(move_to_list)==0) return fake_state_queue_tracker{};
  else {
    constexpr auto base_class_list = unpack(move_to_list, [](auto... mt) {
      constexpr auto single = [](auto mt){ return mk_allow_queue_st(basic_scenario<factory, decltype(+mt().scenario)>::all_trans_info(), mt().state); };
      auto list = (type_list{}<<...<<type_c<state_queue_tracker<factory, decltype(single(mt)), decltype(+mt().scenario), decltype(+mt().state), decltype(+mt().fail_state)>>);
      return filter(list, [](auto t){return decltype(+t)::is_valid();});
    });
    constexpr auto tracker = unpack(base_class_list, [&](auto... t) {
      struct tracker : decltype(+t)... {
        using decltype(+t)::search...;
        using bases = type_list<decltype(+t)...>;
      };
      return final_tracker<factory,tracker>{};
    });
    return tracker;
  }
}

}