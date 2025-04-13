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
  consteval static bool is_valid() {return size(list{})>0;}

  template<typename tgt_sc, typename tgt_st, typename fail_st> constexpr state_queue_tracker* search()
  requires(type_c<tgt_sc> == type_c<scenario> && type_c<tgt_st> == type_c<target_state> && type_c<fail_st> == type_c<fail_state>){ return this; }
  constexpr bool deactivate() {return [this]<auto...inds>(std::index_sequence<inds...>){return ((indexes[inds]=-1)+...);}(std::make_index_sequence<size(list{})>{});}
  constexpr bool is_active() const {return [this]<auto...inds>(std::index_sequence<inds...>){return ((indexes[inds]>=0)+...);}(std::make_index_sequence<size(list{})>{});}

  constexpr static auto get_state(auto st_list, auto ind) {
    return [&]<auto...inds>(std::index_sequence<inds...>){
      return (0+...+((inds==ind)*hash<factory>(get<inds>(st_list))));
    }(std::make_index_sequence<size(st_list)>{});
  }
  constexpr void activate(auto cur_hash) {
    if (is_active()) return;
    if (cur_hash==hash<factory>(last(first(list{})()))) return;
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
  constexpr void update(auto* cur_scenario, const auto& e, auto&&... others) {
    static_assert( !basic_scenario<factory, scenario>::is_multi() );
    auto scenario_cur_hash  = utils::cur_state_hash_from_set<factory, scenario>(others...);
    if (!check_and_shift_state(scenario_cur_hash)) {
      cur_scenario->template force_move_to<fail_state>(e, others...);
      deactivate();
    }
  }
};

template<typename factory, typename list, typename scenario, typename target_state, typename fail_state>
struct state_queue_tracker_multi {
  int sum{0}, max_steps{-1}, n{0};

  consteval static bool is_valid() {return size(list{})>0;}

  template<typename tgt_sc, typename tgt_st, typename fail_st> constexpr state_queue_tracker_multi* search()
  requires(type_c<tgt_sc> == type_c<scenario> && type_c<tgt_st> == type_c<target_state> && type_c<fail_st> == type_c<fail_state>){ return this; }
  constexpr bool deactivate() {return sum=0, max_steps=-1, n=0; }
  constexpr bool is_active() const {return max_steps>0;}
  constexpr auto count_to_end(auto h, auto _st_list) {
    auto st_list = revert(_st_list);
    return [&]<auto... inds>(std::index_sequence<inds...>){
      return (0+...+( inds*(hash<factory>(get<inds>(st_list))==h) ));
    }(std::make_index_sequence<size(st_list)>{});
  }
  constexpr auto scenario_maximum(const auto& s) {
    auto cur_hash = s.cur_state_hash();
    auto cur_max = 0;
    [&]<auto... inds>(std::index_sequence<inds...>){
      ([&](auto ind, auto st_list) {
        auto cur = count_to_end(cur_hash, st_list());
        cur_max = cur*(cur_max<cur);
      }(inds, get<inds>(list{})), ...);
    }(std::make_index_sequence<size(list{})>{});
    return cur_max;
  }
  constexpr void activate(const auto& ms) {
    if (is_active()) return;
    n = ms.count();
    if (n==0) return ;
    ms.foreach_scenario([this](const auto&s) {
      auto cur_max = scenario_maximum(s);
      max_steps = cur_max*(max_steps<cur_max);
      sum += cur_max;
    });
  }
  template<typename other_scenario, typename user_type> constexpr bool update(const xmsm::scenario<factory, other_scenario, user_type>& s) {
    return false;
  }
  template<typename user_type> constexpr bool update(const xmsm::scenario<factory, scenario, user_type>& s) {
    int max = 0, sum = 0;
    s.foreach_scenario([&](const auto&s) {
      auto cur_max = scenario_maximum(s);
      max = max*(max>cur_max) + cur_max*(max<=cur_max);
      sum += cur_max;
      sum *= cur_max>0;
    });
    this->sum = this->sum*(this->sum<=sum) + sum*(sum<this->sum);
    max_steps = max_steps*(max_steps<=max) + max*(max<max_steps);
    return !(this->sum>0 && sum <= this->sum && max <= max_steps);
  }
  constexpr void update(auto* cur_scenario, const auto& e, auto&&... others) {
    if (!is_active()) return;
    if constexpr (!basic_scenario<factory,scenario>::is_finish_state(type_c<target_state>)) {
      if (utils::count_from_set<factory,scenario>(others...) < n) {
        fail(cur_scenario, e, others...);
        return;
      }
    }
    else {
      if (utils::count_from_set<factory,scenario>(others...)==0) {
        deactivate();
        return;
      }
    }
    if ((false||...||update(others))) {
      fail(cur_scenario, e, others...);
    }
  }
  constexpr auto fail(auto* cur_scenario, const auto& e, auto&&... others) {
    cur_scenario->template force_move_to<fail_state>(e, others...);
    deactivate();
  }
};

template<typename factory,typename base> struct final_tracker : base {
  using bases = base::bases;
  constexpr bool is_active() const { return unpack(bases{}, [this](auto... b){return (static_cast<const decltype(+b)*>(this)->is_active() + ...);});}
  constexpr void update(auto* cur_scenario, const auto& e, auto&&... others) {
    foreach(bases{}, [&](auto cur_base){static_cast<decltype(+cur_base)*>(this)->update(cur_scenario, e, others...);return false;});
  }
  constexpr void activate(auto mt, auto&&... scenarios) { if constexpr(requires{this->template search<decltype(+mt().scenario), decltype(+mt().state), decltype(+mt().fail_state)>();}) {
    auto* self = this->template search<decltype(+mt().scenario), decltype(+mt().state), decltype(+mt().fail_state)>();
    if constexpr(basic_scenario<factory,decltype(+mt().scenario)>::is_multi()) self->activate(utils::search_scenario(mt().scenario, scenarios...));
    else self->activate(utils::cur_state_hash_from_set<factory, decltype(+mt().scenario)>(scenarios...)
    );
  }}
};

struct fake_state_queue_tracker {
  constexpr static bool is_active() { return false; }
  constexpr static void update(auto&&...){}
  constexpr static void activate(auto mt, auto&&... scenarios) {}
};

template<typename factory> constexpr auto mk_tracker_base_class(auto mt) {
  constexpr auto single = [](auto mt){ return mk_allow_queue_st(basic_scenario<factory, decltype(+mt().scenario)>::all_trans_info(), mt().state); };
  if constexpr(basic_scenario<factory,decltype(+mt().scenario)>::is_multi())
    return type_c<state_queue_tracker_multi<factory, decltype(single(mt)), decltype(+mt().scenario), decltype(+mt().state), decltype(+mt().fail_state)>>;
  else return type_c<state_queue_tracker<factory, decltype(single(mt)), decltype(+mt().scenario), decltype(+mt().state), decltype(+mt().fail_state)>>;
}
template<typename factory, typename... others>
constexpr auto mk_tracker(auto target_info) {
  constexpr auto move_to_list = unpack(target_info, [](auto... i){return (type_list{} << ... << decltype(+i)::mod_move_to);});
  if constexpr(size(move_to_list)==0) return fake_state_queue_tracker{};
  else {
    constexpr auto base_class_list = unpack(move_to_list, [](auto... mt) {
      auto list = (type_list{}<<...<<mk_tracker_base_class<factory>(mt));
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