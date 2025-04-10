/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#include "factory.hpp"

#include <string>
#include <stdexcept>
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
static_assert( xmsm::is_type_c<decltype([]{return xmsm::type_c<>;}())> );
static_assert( xmsm::is_type_c<decltype([]{return xmsm::type_c<int>;}())> );
static_assert( !xmsm::is_type_c<decltype([]{return int{};}())> );
static_assert( !xmsm::is_type_c<decltype([]{}())> );

static_assert( xmsm::has_duplicates(1,2,3) == 0 );
static_assert( xmsm::has_duplicates(1,2,1) == 1 );
static_assert( xmsm::has_duplicates(1,1,1) == 3, "the function are not really good to find duplications count, but 0 means there is no duplicates" );
static_assert( xmsm::has_duplicates(1,1,2,2) == 2 );

struct base{}; struct child : base{};
static_assert( xmsm::type_c<int> <= xmsm::type_c<int>, "test for integral types" );
static_assert( xmsm::type_c<base> <= xmsm::type_c<base>, "test for user types" );
static_assert( xmsm::type_c<base> <= xmsm::type_c<child>, "test for user types" );
static_assert(  contains(xmsm::type_list<int,char>{}, xmsm::type_c<char>) );
static_assert( !contains(xmsm::type_list<int,double>{}, xmsm::type_c<char>) );
static_assert( xmsm::type_list{} << xmsm::type_c<int> << xmsm::type_c<int> << xmsm::type_list<double,int,int>{} == xmsm::type_list<int,double>{} );
static_assert( xmsm::type_list{} << xmsm::type_c<base> << xmsm::type_c<child> == xmsm::type_list<base>{} );
static_assert( xmsm::type_list{} << xmsm::type_c<child> << xmsm::type_c<base> == xmsm::type_list<child,base>{} );
static_assert( revert(xmsm::type_list<char,double,long,int,bool>{}) == xmsm::type_list<bool,int,long,double,char>{} );

static_assert( index_of(xmsm::type_list<int,double>{}, xmsm::type_c<char>) == -1 );
static_assert( index_of(xmsm::type_list<int,double>{}, xmsm::type_c<double>) == 1 );

int main(int,char**){
  {
    constexpr auto ct_hash = hash<tests::factory>(xmsm::type_c<tests::state<0>>);
    auto rt_hash = hash<tests::factory>(xmsm::type_c<tests::state<0>>);
    std::cout << "cmp ct and rt hashes: " << ct_hash << "==" << rt_hash << std::endl;
    if (ct_hash!=rt_hash) throw std::runtime_error(std::to_string(__LINE__));
  }
  std::cout << "hash64(foo) == |" << hash64<tests::factory>(xmsm::type_c<foo>) << '|' << std::endl;
}