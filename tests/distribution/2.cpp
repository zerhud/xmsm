/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "factory.hpp"

static_assert( [] {
  auto [m1,m2,m3] = mk_m(); connect(m1,m2,m3);
  m1.on(event<100>{}); m2.on(event<100>{}); m2.on(event<0>{}); m1.on(event<111>{});
  if (m2.scenarios_count<ts_multi_e2>()!=0) throw __LINE__;
  m2.on(event<100>{}); m1.on(event<100>{}); m2.on(event<0>{}); m1.on(event<111>{});
  return (m2.scenarios_count<ts_multi_e2>()==0) + 2*m2.in_state<ts_multi_e1, state<3>>() + 4*(m2.in_state_count<ts_multi_e1, state<3>>()==2);
}() == 7, "can fail twice" );
static_assert( [] {
  auto [m1,m2,m3] = mk_m(); connect(m1,m2,m3);
  m1.on(event<100>{}); m2.on(event<100>{}); m2.on(event<0>{}); m1.on(event<111>{});
  m1.on(event<100>{}); m1.on(event<0>{}); m1.on(event<111>{});
  m2.on(event<100>{}); m2.on(event<0>{});
  if (!m2.in_state<ts_multi_e1, state<3>>()) throw __LINE__;
  return m2.scenarios_count<ts_multi_e2>();
}() == 0, "fail if move multi scenario in wrong state" );

int main(int,char**) {
  std::cout
  << " e1: " << hash<factory_e2>(xmsm::type_c<entity_1>)
  << " e2: " << hash<factory_e2>(xmsm::type_c<entity_2>)
  << "\n0: " << hash<factory_e2>(xmsm::type_c<state<0>>)
  << " 1: " << hash<factory_e2>(xmsm::type_c<state<1>>)
  << " 2: " << hash<factory_e2>(xmsm::type_c<state<2>>)
  << " 3: " << hash<factory_e2>(xmsm::type_c<state<3>>)
  << " 100: " << hash<factory_e2>(xmsm::type_c<state<100>>)
  << " 111: " << hash<factory_e2>(xmsm::type_c<state<111>>)
  << "\n" << std::endl;
}
