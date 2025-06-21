# xmsm
extended meta finite state automaton

# xmsm

**xmsm** (eXtended Meta State Machine) is a header-only C++23/26 library for implementing advanced state machines with support for state stacks, inter-scenario interactions, and multiple instances. It focuses on performance (static polymorphism, `constexpr`) and flexibility (metaprogramming), making it ideal for complex systems with minimal runtime overhead.

## Key Features
- **State Stack**: Ability to preserve previous states and return to them.
- **Single and Multi Scenarios**: Supports both classic state machines (`single_scenario`) and collections of identical machines (`multi_scenario`).
- **Scenario Interaction**: Transitions can depend on or affect other scenarios via modifiers (`when`, `only_if`, `move_to`, `try_move_to`). Also an exprssion can to be used (for example `in<scenario, state1, state2>(f) && now_in<scenario2, state3>(f)`)
- **Compile-Time Logic**: Most computations (states, events, transitions) are performed at compile time.
- **Flexible Customization**: User-defined factories allow tailoring the library to any needs.

## Build and Installation

### Requirements
- A modern compiler with C++23 support (e.g., GCC 14+, Clang 20+). Some features leverage C++26 (optional).
- Minimal dependencies – only the C++ Standard Library is required.

### Installation
`xmsm` is a header-only library, so installation is straightforward:
1. Clone the repository:
   ```bash
   git clone https://github.com/zerhud/xmsm.git
   ```
1. Use `make install` in root of the project or copy the header files from the `include` directory to a location accessible to your compiler (e.g., `/usr/local/include/` or your project):
   ```bash
   cp xmsm.hpp /usr/local/include/
   cp -r xmsm /usr/local/include/
   ```
1. For Nix users: the repository contains `flake.nix` file, so you can use `nix develop` with it

You can also compile tests. It requires `g++` and `clang++` with c++23 support. The `make` command will build all tests (for example `make -j$(nproc --all)`).

## Usage Guide
To start using:
- implement a factory. the factory is a simple class without data. it allows to separte realization from the idea
- defining scenarios
- declare machine with scenarios. the machine requires factory and scenarios
  - `template<typename factory> usign machine = xmsm::machine<factory, scenario1, scenario2, scenario3>;` it allows to use machine with any other factory
  - `using machine = xmsm::machine<implemented_factory, scenaro1, scenario2, scenario3>;` for use with concrete factory.
 
There is example in [tests/example.cpp](tests/example.cpp)

### Implement a factory  
The factory is a class with realiztion. For example with C++ standard library

```c++
struct factory {
  using string_view = std::string_view; // will be removed soon
};

// variant to store states
template<typename... types> constexpr auto mk_variant(const factory&) {
  return std::variant<types...>{};
}

// containers and erase method for it (can to be implemented for each container separately)
template<typename type> constexpr auto mk_vec(const factory&) { return std::vector<type>{}; }
template<typename type> constexpr auto mk_list(const factory&) {
  return std::list<type>{}; // this container should keep pointers after deletion
}
constexpr void erase(const factory&, auto& cnt, auto ind) {
  cnt.erase(cnt.begin() + ind);
}

// mk_atomic allows to use xmsm with threads. it can to be implemented like this if only thread will be used.
// this method requires only with multi scenario
template<typename type> constexpr auto mk_atomic(const factory&) {
  return type{};
}

// also the method can to be implemented to handle exceptions in state creation and switch events
// or can to be omited. the method will be called from catch block, if exists.
template<typename scenario, typename trans, typename next> constexpr void on_exception(const factory& f) {
  throw;
}
```

Also, you can use [predefined factory](xmsm/std_factory.hpp) for std library

### Defining Scenarios
Scenario is a class with `static auto describe_sm(const auto& f)` method. The method returns machine description. The parameter used for methods in ADL.

For example:
```c++
struct my_object {
  int value{0}; // any data you want, or no fields at all
  static auto describe_sm(const auto& f) {
    return  mk_sm_description(f // the f object is used for ADL
      , mk_trans<state<0>, state<1>, event<0>>(f) // Transition from state<0> to state<1> on event<0>
      , mk_trans<state<1>, state<0>, event<1>>(f)
      , pick_def_state<state<0>>(f) // Default state (if not present the first, e.g. state<0> will be used)
    );
  }
};
```

the `mk_multi_sm_description` should be used for create a scenario with few sm inside.

the `mk_trans` methods describe a transition and has few template parameters:
1. state from (can to be void)
2. state to
3. event (optional)

also it accepts any parameters - modifiers. some modifiers can to be passed to `mk_sm_description`.

### Available Transition Modifiers
Transitions can be customized with modifiers:

- `when`: Triggers a transition if a condition on other scenarios is true. the condition will be describe later.
   ```c++
  mk_trans<state<0>, state<1>>(f, when(f, in<other_scenario1, state<1>>(f) && now_in<other_scenario2, state<2>>(f)))
   ```
- `only_if`: Blocks a transition unless a condition is met.
   ```c++
  mk_trans<state<0>, state<1>>(f, only_if(f, in<other_scenario1, state<1>>(f)))
   ```
- `stack_by_event`: Pushes the current state onto the stack, with return triggered by specified events. The current state of the scenario will be the second state in the transition description.
   ```c++
  // to to state<1> and save state<0> on previus stack frame
  mk_trans<state<0>, state<1>, event<0>>(f, stack_by_event<event<10>>(f))
   ```
  if few frames exist and the "back event" occurred, all stack frames with the same "back event" will be destroyed
- `stack_by_expr`: Same as `stack_by_event` but expression is used to destroy frame
   ```c++
  mk_trans<state<0>, state<1>, event<0>>(f, stack_by_expr(f, now_in<other_scenario1, state<1>>(f)))
   ```
- `move_to`: Attempts to move another scenario to a specified state. If it fails, move the scenario to fail state
  - if the required state cannot to be achived right now the scenario will track a path to it state and if scenario makes "wront turn" the fail state will be setted.
  - tansitions in other scenario have to be marked with `allow_queue` for to be trackable
  - transitions in other scenario can to be marked with `allow_move`. the mark allows transition happen on `move_to`.
- `try_move_to`: Same as `move_to`, but dose nothing on fail

there is also few modifiers for use in `mk_sm_description`:
- `to_state_mods` for add some modifiers to all transitions where the to state is same as in the method
- `from_state_mods` for add some modifiers to all transitions where the from state is same as in the method
- `pick_def_state` allow to set default state for scenario. if there is no such method the first state will be used as default
- `entity` allows to specify an entity for scenario. it allows to use the xmsm in different processes.
- also few modifiers onyl for multi scenario
  - `start_event` the scenario will start on the event
  - `finish_state` the scenario will be destroied when achive this state
 
### Available Conditions
some modifiers works with conditions. any conditions can to be combined with logical operators - for example `in<scenario, state>(f) && affected<scenario>(f)`.
- `in<scenario, state1, state2, state3>(f)` true when scenario in one of listed states
- `affected<scenario1, scenario2>(f)` true when all of listed scenario was changed in current event
- `cnt_in` for multi scenario, true when count of scenarios achives required state
- `broken` true when scenario is in broken state - for example due exception
- `now_in` is the same as `in` and `affected`

# License
xmsm is distributed under the [GNU Affero General Public License](https://www.gnu.org/licenses/). See the COPYING file in the repository root.

# Contributing
Contributions are welcome! Fork the repo, create a PR, or open an issue on GitHub.
