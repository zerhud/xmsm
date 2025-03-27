/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "factory.hpp"

#include <iostream>

using namespace std::literals;

struct foo;
namespace test::t::t{ template<auto i>struct bar{}; }
static_assert( name<tests::factory>(xmsm::type_c<foo>) == "foo"sv );
static_assert( name<tests::factory>(xmsm::type_c<test::t::t::bar<3>>) == "test::t::t::bar<3>"sv );
static_assert( hash<tests::factory>(xmsm::type_c<foo>) == 2414502773, "the murmurhash value calculated in external tool" );

int main(int,char**){
  std::cout << "hash42(foo) == |" << hash64<tests::factory>(xmsm::type_c<foo>) << '|' << std::endl;
}