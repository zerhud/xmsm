/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "factory.hpp"

static_assert( [] {
  xmsm::remote_scenario<factory_e2, ts_no_entity, connector<30>> r{factory_e2{}};
  r.state(30);
  r.reset_own_state();
  if (r.own_state()!=xmsm::scenario_state::ready) throw __LINE__;
  r.state(30);
  if (r.own_state()!=xmsm::scenario_state::ready) throw __LINE__;
  r.state(31);
  return r.own_state();
}() == xmsm::scenario_state::fired );


static_assert( xmsm::scenario<factory_default, ts_no_entity>::entity_list() == xmsm::type_list<entity_1>{} );
static_assert( xmsm::scenario<factory_default, ts_queue_e2>::entity_list() == xmsm::type_list<entity_2>{} );
static_assert( xmsm::scenario<factory_e2, ts_no_entity>::entity_list() == xmsm::type_list<entity_1>{} );
static_assert( machine1{factory_default{}}.is_remote<ts_queue_e2>() );
static_assert( !machine1{factory_default{}}.is_remote<ts_no_entity>() );
static_assert( !machine2{factory_e2{}}.is_remote<ts_queue_e2>() );
static_assert( machine2{factory_e2{}}.is_remote<ts_no_entity>() );
static_assert( [] { machine1 m1{factory_default{}}; return std::is_same_v<decltype(get<ts_multi_e1>(m1.scenarios)), xmsm::scenario<factory_default, ts_multi_e1, ts_multi_e1>&>; }(), "get by scenario works" );

static_assert( [] {
  auto [m1,m2,m3] = mk_m(); connect(m1,m2,m3);
  m2.on(event<0>{});
  return m1.in_state<ts_queue_e2, state<1>>() + 2*m2.in_state<ts_queue_e2, state<1>>()
  + 4*m1.in_state<ts_no_entity, state<0>>() + 8*m2.in_state<ts_no_entity, state<0>>()
  ;
}() == 15 );
static_assert( [] {
  auto [m1,m2,m3] = mk_m(); connect(m1,m2,m3);
  m1.on(event<0>{});
  return m1.in_state<ts_no_entity, state<1>>() + 2*m1.in_state<ts_queue_e2, state<1>>()
    + 4*m2.in_state<ts_no_entity, state<1>>() + 8*m2.in_state<ts_queue_e2, state<1>>()
    + 16*!get<ts_no_entity>(m1.scenarios).move_to_tracker.is_active()
    + 32*m2.in_state<ts_when_e2, state<1>>() + 64*(get<ts_when_e2>(m2.scenarios).own_state()==xmsm::scenario_state::fired)
  ;
}() == 127 );
static_assert( [] {
  auto [m1,m2,m3] = mk_m(); connect(m1,m2,m3);
  m1.on(event<1>{});
  return m2.in_state<ts_queue_e2, state<2>>() + 2*m1.in_state<ts_no_entity, state<3>>() + 4*m2.in_state<ts_no_entity, state<3>>();
}() == 7 );

static_assert( [] {
  auto [m1,m2,m3] = mk_m(); connect(m1,m2,m3);
  m2.on(event<100>{}); m2.on(event<100>{}); m2.on(event<100>{}); m2.on(event<100>{});
  return (m1.scenarios_count<ts_multi_e2>()==4) + 2*(m2.scenarios_count<ts_multi_e2>()==4);
}() == 3 );

static_assert( [] {
  auto [m1,m2,m3] = mk_m(); connect(m1,m2,m3);
  m1.on(event<100>{});
  if (!m1.in_state<ts_multi_e1, state<0>>()) throw __LINE__;
  m2.on(event<100>{});
  m2.on(event<0>{});
  if (!m1.in_state<ts_multi_e1, state<1>>()) throw __LINE__;
  m1.on(event<111>{});
  return (m2.scenarios_count<ts_multi_e2>()==0) + 2*m2.in_state<ts_multi_e1, state<3>>();
}() == 3 );

int main(int,char**) {
}