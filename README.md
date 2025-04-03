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
1. Copy the header files from the `include` directory to a location accessible to your compiler (e.g., `/usr/local/include/` or your project):
   ```bash
   cp -r include/xmsm.hpp /usr/local/include/
   cp -r include/xmsm /usr/local/include/
   ```
1. For Nix users: the repository contains `flake.nix` file, so you can use `nix develop` with it

### Building tests
1. Navigate to the repository root.
2. Ensure `make` and a compiler (e.g., `g++`) are installed.
3. Run `make`

## Usage Guide
### Defining Scenarios
Scenario is a class with `static auto describe_sm(const auto& f)` method. The method returns machine description.

For example:
```c++
struct my_object {
  int value{0}; // any data you want
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

### Available Transition Modifiers
Transitions can be customized with modifiers:

- `when`: Triggers a transition if a condition on other scenarios is true.
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
- `move_to`: Attempts to move another scenario to a specified state. If it fails, move the scenario to fail state (or block the transition)
- `try_move_to`: Same as `move_to`, but dose nothing on fail

# License
xmsm is distributed under the [GNU Affero General Public License](https://www.gnu.org/licenses/). See the COPYING file in the repository root.

# Contributing
Contributions are welcome! Fork the repo, create a PR, or open an issue on GitHub.