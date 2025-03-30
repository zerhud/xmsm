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

static_assert( xmsm::type_c<foo> == xmsm::type_c<foo> );
static_assert( xmsm::type_c<int> != xmsm::type_c<foo> );
static_assert( xmsm::type_c<decltype(xmsm::type_list{} << xmsm::type_c<int> << xmsm::type_c<>)> == xmsm::type_c<xmsm::type_list<int>> );

static_assert( xmsm::has_duplicates(1,2,3) == 0 );
static_assert( xmsm::has_duplicates(1,2,1) == 1 );
static_assert( xmsm::has_duplicates(1,1,1) == 3, "the function are not really good to find duplications count, but 0 means there is no duplicates" );
static_assert( xmsm::has_duplicates(1,1,2,2) == 2 );

int main(int,char**){
  std::cout << "hash42(foo) == |" << hash64<tests::factory>(xmsm::type_c<foo>) << '|' << std::endl;
}