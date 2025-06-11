#include "../xmsm.hpp"
#include "machine.hpp"

extern unsigned char __heap_base;

using size_t = decltype(sizeof(int));
inline void* operator new(size_t, void* ptr) noexcept { return ptr; }
inline void operator delete(void*, void*) noexcept {}

namespace wstd {

template<typename t> struct _type_c{using type=t;t operator+()const;};
template<typename t> constexpr static auto type_c = _type_c<t>{};
template<typename t> constexpr bool operator==(_type_c<t>, _type_c<t>) { return true;  }
template<typename l, typename r> constexpr bool operator==(_type_c<l>, _type_c<r>) { return false;  }


struct exception {};
struct overflow : exception {};

unsigned char* bump_pointer = &__heap_base;
void* dalloc(int n) {
  auto* r = bump_pointer;
  bump_pointer += n+n%8;
  return (void *)r;
}

template<typename type, auto cop> struct static_vector {
  type* base{nullptr};
  size_t sz{};

  static_vector() : base((type*)dalloc(cop)) {}

  constexpr bool empty() const { return sz==0; }
  constexpr type* data() {return base;}
  constexpr auto size() const { return sz; }
  constexpr auto& back() { return base[sz-1]; }
  constexpr const auto& back() const { return base[sz-1]; }
  constexpr type& operator[](auto ind) {return base[ind]; }
  constexpr const type& operator[](auto ind) const {return base[ind]; }
  constexpr type& emplace_back(auto&&...args) {
    if (cop <= sz) return base[sz-1];
    auto* ret = new(&base[sz++])type(static_cast<decltype(args)&&>(args)...);
    return *ret;
  }
  constexpr void pop() { base[sz--].~type(); }
  constexpr void erase(auto ind) {
    for (auto i=ind;i<sz;++i) {
      base[i].~type();
      if (i+1<sz) base[i] = static_cast<type&&>(base[+1]);
    }
    --sz;
  }
};

struct string_view {
  const char* base;
  size_t sz;

  constexpr string_view() : base(nullptr), sz(0) {}
  constexpr explicit string_view(const char* b, size_t s) : base(b), sz(s) {}

  const char* data() const {return base;}
  constexpr auto size() const { return sz; }
  constexpr char& operator[](auto ind)const{return base[ind];}
};

template<typename... types> struct variant {
  constexpr static auto max_size() {
    size_t max=0;
    ((max=( (sizeof(types)*(max<sizeof(types))) + (max*(sizeof(types)<=max)) ) ),...);
    return max + (max%64);
  }
  size_t cur{};
  char storage[max_size()]{};
  decltype(xmsm::mk_tuple((types*)nullptr...)) poitners;

  constexpr explicit variant() {
    new(&storage)types...[0]();
  }
  constexpr variant(variant&& other) : cur(other.cur) { other.visit([&](const auto& v){ emplace<decltype(+xmsm::type_dc<decltype(v)>)>(v); }); }
  constexpr variant(const variant& other) : cur(other.cur) { other.visit([&](auto&& v){ emplace<decltype(+xmsm::type_dc<decltype(v)>)>(static_cast<decltype(v)&&>(v)); }); }
  constexpr variant& operator=(const variant& other) { cur = other.cur; other.visit([&](const auto& v){emplace<decltype(+xmsm::type_dc<decltype(v)>)>(v);}); return *this; }
  constexpr variant(auto& v) {
    constexpr auto ind = index_of(xmsm::type_list<types...>{}, xmsm::type_dc<decltype(v)>);
    new(&storage)types...[ind](v);
    cur = ind;
  }
  constexpr variant(auto&& v) {
    constexpr auto ind = index_of(xmsm::type_list<types...>{}, xmsm::type_dc<decltype(v)>);
    new(&storage)types...[ind](v);
    cur = ind;
  }
  constexpr auto index() const { return cur; }
  template<typename type> constexpr type& emplace(auto&&... args) {
    static_assert( ind_of<type>() >= 0 );
    this->visit([]<typename t>(t&v){v.~t();});
    auto ret = new(&storage)type(static_cast<decltype(args)&&>(args)...);
    cur = ind_of<type>();
    get<ind_of<type>()>(poitners) = ret;
    return *ret;
  }
  template<typename t> constexpr static auto ind_of() {
    int  ret=-1;
    (void)( (++ret,type_c<t> == type_c<types>)||... );
    return ret;
  }

  template<auto cur_ind=0> constexpr auto visit(auto&& fnc) {
    if constexpr(cur_ind == sizeof...(types)-1) return fnc(*get<cur_ind>(poitners));
    else {
      static_assert( cur_ind < sizeof...(types) );
      if (index()==cur_ind) return fnc(*get<cur_ind>(poitners));
      return visit<cur_ind+1>(static_cast<decltype(fnc)&&>(fnc));
    }
  }
  template<auto cur_ind=0> constexpr auto visit(auto&& fnc) const {
    if constexpr(cur_ind == sizeof...(types)-1) return fnc(*get<cur_ind>(poitners));
    else {
      static_assert( cur_ind < sizeof...(types) );
      if (index()==cur_ind) return fnc(*get<cur_ind>(poitners));
      return visit<cur_ind+1>(static_cast<decltype(fnc)&&>(fnc));
    }
  }
  template<typename type> constexpr friend auto& get(variant& self) {
    constexpr auto ind = index_of(xmsm::type_list<types...>{}, xmsm::type_dc<type>);
    return *get<ind>(self.poitners);
  }

  constexpr friend decltype(auto) visit(auto&& fnc, variant& self) { return self.visit(static_cast<decltype(fnc)&&>(fnc)); }
};
}

struct factory {
  using string_view = wstd::string_view;
};
template<typename... types> constexpr auto mk_variant(const factory&) {
  return wstd::variant<types...>{};
}
template<typename type> constexpr auto mk_vec(const factory&) { return wstd::static_vector<type,100>{}; }
template<typename type> constexpr auto mk_list(const factory& f) { return mk_vec<type>(f); }
template<typename type> constexpr auto mk_atomic(const factory&) { return type{}; }
constexpr void erase(const factory&, auto& cnt, auto ind) {
  cnt.erase(ind);
}

using vis_machine = xmsm_vis::machine<factory>;

extern "C" void js_callback(int);
extern "C" void change_state(uint64_t scenario);
extern "C" void clear_states();

auto& m() {
  static vis_machine machine{factory{}};
  return machine;
}

constexpr auto vertexes_size = unpack(vis_machine::all_scenarios(), [](auto...st) {
  return (0 + ... + [](auto s){return size(decltype(+s)::all_events()) + size(decltype(+s)::all_states());}(st));
});
auto& all_vertexes_src() {
  static struct {
    uint64_t hash{};
    uint32_t group{};
    wstd::string_view name{};
    uint32_t event_hash{};
  } ret[vertexes_size];
  return ret;
}
extern "C" uint32_t all_vertexes_size() { return vertexes_size; }
extern "C" uint64_t all_vertexes_hash(uint32_t num) {return all_vertexes_src()[num].hash;}
extern "C" uint32_t all_vertexes_group(uint32_t num) {return all_vertexes_src()[num].group;}
extern "C" bool all_vertexes_is_event(uint32_t num) {return all_vertexes_src()[num].event_hash!=0;}
extern "C" uint32_t all_vertexes_event_hash(uint32_t num) {return all_vertexes_src()[num].event_hash;}
extern "C" const char* all_vertexes_name(uint32_t num) {return all_vertexes_src()[num].name.base;}
extern "C" uint32_t all_vertexes_name_sz(uint32_t num) {return all_vertexes_src()[num].name.sz;}

constexpr auto edges_size = unpack(vis_machine::all_scenarios(), [](auto...st) {
  return (0 + ... + size(decltype(+st)::all_trans_info()));
});
auto& all_edges_src() {
  static struct {
    uint64_t hash{};
    uint64_t from{};
    uint64_t to{};
    uint64_t event{};
    uint32_t group{};
    wstd::string_view event_name{};
    wstd::static_vector<uint64_t, 20> move_to;
  } ret[edges_size];
  return ret;
}
extern "C" uint32_t all_edges_size() { return edges_size; }
extern "C" uint64_t all_edges_to(uint32_t num) { return all_edges_src()[num].to; }
extern "C" uint64_t all_edges_from(uint32_t num) { return all_edges_src()[num].from; }
extern "C" uint64_t all_edges_hash(uint32_t num) { return all_edges_src()[num].hash; }
extern "C" uint64_t all_edges_event(uint32_t num) { return all_edges_src()[num].event; }
extern "C" uint64_t all_edges_group(uint32_t num) { return all_edges_src()[num].group; }
extern "C" const char* all_edges_event_name(uint32_t num){ return all_edges_src()[num].event_name.base; }
extern "C" uint32_t all_edges_event_name_sz(uint32_t num){ return all_edges_src()[num].event_name.sz; }
extern "C" uint32_t all_edges_move_to_sz(uint32_t num){ return all_edges_src()[num].move_to.sz; }
extern "C" const uint64_t* all_edges_move_to(uint32_t num){ return all_edges_src()[num].move_to.base; }

auto& all_groups_src() {
  static struct {
    uint32_t hash{};
    wstd::string_view name{};
  } ret[size(vis_machine::all_scenarios())];
  return ret;
}
extern "C" uint32_t all_groups_size() { return size(vis_machine::all_scenarios()); }
extern "C" uint32_t all_groups_hash(uint32_t num){ return all_groups_src()[num].hash; }
extern "C" const char* all_groups_name(uint32_t num){ return all_groups_src()[num].name.base; }
extern "C" uint32_t all_groups_name_sz(uint32_t num){ return all_groups_src()[num].name.sz; }

extern "C" void update_states() {
  foreach(m().scenarios, [](auto& s) {
    change_state(((uint64_t)s.own_hash() << 32) + (uint64_t)s.cur_state_hash());
    return false;
  });
}
extern "C" void on_event(uint32_t hash) {
  clear_states();
  m().on_event_by_hash(hash);
  update_states();
}

extern "C" void main_function() {
  unpack(vis_machine::all_scenarios(), [](auto...s) {
    auto i=-1; (void)( [&] { ++i;
      all_groups_src()[i].hash = decltype(+s)::own_hash();
      all_groups_src()[i].name = decltype(+s)::own_name();
      return false;
    }()||...);

    auto add_fnc = [i=-1,ei=-1](auto s, auto sl, auto el, auto tl)mutable{
      constexpr auto st_hash_part = (uint64_t)decltype(+s)::own_hash() << 32;
      foreach(tl, [&](auto t)mutable{ ++ei;
        all_edges_src()[ei].hash = hash(t) + st_hash_part;
        all_edges_src()[ei].from = hash(t().from) + st_hash_part;
        all_edges_src()[ei].to = hash(t().to) + st_hash_part;
        all_edges_src()[ei].event = hash(t().event) + st_hash_part;
        all_edges_src()[ei].group = decltype(+s)::own_hash();
        all_edges_src()[ei].event_name = name<factory>(t().event);
        foreach(t().mod_move_to << t().mod_try_move_to, [&](auto mt) {
          all_edges_src()[ei].move_to.emplace_back(hash(mt().state) + ((uint64_t)xmsm::basic_scenario<factory,decltype(+mt().scenario)>::own_hash() << 32));
          return false;
        });
        return false;
      });
      foreach(sl, [&](auto st) { ++i;
        all_vertexes_src()[i].hash = hash(st) + st_hash_part;
        all_vertexes_src()[i].name = name<factory>(st);
        all_vertexes_src()[i].group = decltype(+s)::own_hash();
        return false;
      });
      foreach(el, [&](auto ev) { ++i;
        all_vertexes_src()[i].hash = hash(ev) + st_hash_part;
        all_vertexes_src()[i].name = name<factory>(ev);
        all_vertexes_src()[i].group = decltype(+s)::own_hash();
        all_vertexes_src()[i].event_hash = hash(ev);
        return false;
      });
      return false;
    };
    (void)(add_fnc(s, decltype(+s)::all_states(), decltype(+s)::all_events(), decltype(+s)::all_trans_info())||...);
  });

  js_callback(41);
  clear_states();
}
