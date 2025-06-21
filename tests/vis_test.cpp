/*************************************************************************
* Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "factory.hpp"
#include "vis/machine.hpp"

int main(int,char**) {
  xmsm_vis::machine<tests::factory> m{tests::factory{}};
  m.on(xmsm_vis::desktop_app::crash{});
}