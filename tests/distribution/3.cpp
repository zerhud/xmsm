/*************************************************************************
* Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "factory.hpp"

static_assert( !xmsm::basic_scenario<factory_e2, ts_sep_e4>::need_sync_with_ent<entity_1>() );
static_assert( !xmsm::basic_scenario<factory_e2, ts_sep_e4>::need_sync_with_ent<entity_4>() );
static_assert( !xmsm::basic_scenario<factory_e2, ts_sep_e4>::need_sync_with_ent<entity_5>() );
static_assert(  xmsm::basic_scenario<factory_e2, ts_sep_e5>::need_sync_with_ent<entity_4>() );
static_assert(  machine1::need_sync<entity_5, entity_4>() );
static_assert(  machine1::need_sync<entity_4, entity_5>() );
static_assert( !machine1::need_sync<entity_4, entity_3>() );
static_assert( !machine1::need_sync<entity_5, entity_1>() );
static_assert( [] {
  auto [m1,m2,m3,m4,m5] = mk_m(); connect(m1,m2,m3,m4,m5);
  m4.on(event<1>{});
  return m4.in_state<ts_sep_e4, state<1>>() + 2*m5.in_state<ts_sep_e4, state<1>>() + 4*m1.in_state<ts_sep_e4, state<0>>();
}() == 7 );

struct ts_sep_e5_cond {
  static auto describe_sm(const auto& f) {
    return mk_sm_description(f
    , mk_trans<state<0>, state<1>, event<1>>(f, stack_by_expr(f, in<ts_sep_e4, state<1>>(f)))
    , mk_trans<state<1>, state<0>, event<0>>(f)
    , entity<entity_5>(f)
    );
  }
};

static_assert(  xmsm::basic_scenario<factory_e2, ts_sep_e5_cond>::need_sync_with_ent<entity_4>() );
static_assert( !xmsm::basic_scenario<factory_e2, ts_sep_e5_cond>::need_sync_with_ent<entity_3>() );

int main(int,char**) {
}