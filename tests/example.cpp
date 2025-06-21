/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include <cassert>
#include <xmsm.hpp>

#include "xmsm/std_factory.hpp"

namespace traffic_light_system {

struct tick_event {};
struct emergency_event {};
struct emergency_off_event {};

struct red_color {};
struct green_color {};
struct yellow_color {};

// Сценарий для главного светофора
struct main_road_traffic_light {
  int cycle_count{0}; // Счетчик циклов для демонстрации

  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      , mk_trans<red_color, green_color, tick_event>(f) // red -> green on tick
      , mk_trans<green_color, yellow_color, tick_event>(f) // green -> yellow on tick
      , mk_trans<yellow_color, red_color, tick_event>(f) // yellow -> red on tick
      , pick_def_state<red_color>(f)                   // Начальное состояние: red
    );
  }

  void on_enter(auto& f, auto& state, auto& event) {
    if constexpr (std::is_same_v<decltype(state), green_color>) { // Вход в green
      ++cycle_count;
    }
  }
};

// Сценарий для второстепенного светофора
struct side_road_traffic_light {
  int emergency_count{0}; // Счетчик чрезвычайных ситуаций

  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
      // red -> green (when main going to red on current tick)
      , mk_trans<red_color, green_color>(f, when(f, in<main_road_traffic_light, red_color>(f) && affected<main_road_traffic_light>(f)))
      // green -> red (when main going to green)
      , mk_trans<green_color, red_color>(f, when(f, in<main_road_traffic_light, green_color>(f)))
      , mk_trans<green_color, red_color, emergency_event>(
          f, stack_by_event<emergency_off_event>(f))                // green -> red on emergency, сохранить red в стек
      , pick_def_state<red_color>(f)                           // Начальное состояние: red
    );
  }

  void on_enter(auto& f, auto& state, const emergency_event& event) {
    if constexpr (std::is_same_v<std::decay_t<decltype(state)>, red_color>) { // Вход в red по emergency
      ++emergency_count;
    }
  }
};

// define reaction outside
void on_enter(side_road_traffic_light& sc, const green_color&, const emergency_off_event&) {
  --sc.emergency_count;
}

// Фабрика для создания объектов
struct factory : xmsm::std_factory { };

// Тестирование
void test_traffic_light() {
  xmsm::machine<factory, main_road_traffic_light, side_road_traffic_light> m(factory{});

  // Проверяем начальные состояния
  assert(( m.in_state<main_road_traffic_light, red_color>() )); // main_road: red
  assert(( m.in_state<side_road_traffic_light, red_color>() )); // side_road: red

  // Первый тик: главный переходит в green, второстепенный остается в red
  m.on(tick_event{});
  assert(( m.in_state<main_road_traffic_light, green_color>() )); // main_road: green
  assert(( m.in_state<side_road_traffic_light, red_color>() ));   // side_road: red

  // Второй тик: главный переходит в yellow, второстепенный остается в red
  m.on(tick_event{});
  assert(( m.in_state<main_road_traffic_light, yellow_color>() )); // main_road: yellow
  assert(( m.in_state<side_road_traffic_light, red_color>() ));    // side_road: red

  // Третий тик: главный переходит в red, второстепенный в green
  m.on(tick_event{});
  assert(( m.in_state<main_road_traffic_light, red_color>() ));    // main_road: red
  assert(( m.in_state<side_road_traffic_light, green_color>() ));  // side_road: green

  // Чрезвычайная ситуация: второстепенный переходит в red, сохраняя green в стек
  m.on(emergency_event{});
  assert( get<side_road_traffic_light>(m).emergency_count==1 ); // the handler was called
  assert(( m.in_state<side_road_traffic_light, red_color>() ));    // side_road: red
  assert(( stack_size<side_road_traffic_light>(m) == 1 ));
//  assert(m.debug_get_base().scenarios.g<1>().stack_size() == 1);   // green в стеке

  // Еще одно событие emergency: возвращаемся к green из стека
  m.on(emergency_off_event{});
  assert(( stack_size<side_road_traffic_light>(m) == 0 ));
  assert( get<side_road_traffic_light>(m).emergency_count==0 ); // outside reaction decreases the counter

  // final state tests
  assert(( m.in_state<side_road_traffic_light, green_color>() ));  // side_road: green
  assert(( m.in_state<main_road_traffic_light, red_color>() ));    // main_road: red
}


} // namespace traffic_light_system

int main(int,char**) {
  traffic_light_system::test_traffic_light();
}