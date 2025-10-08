# Task List
 - [ ] ECS System
  - [ ] Query API
    - [x]  iterator to go through elements
      - [ ] Make it non-allocating
    - [ ] Integrate the query API into systems
  - [ ] Allow for dynamic addition and removal of elements
- [ ] Renderer

# Learnings
List of things that I've learnt building this project
  1. templates and concepts, variadic template arguments
  2. Value categories: lvalues and rvalues
  3. Copy, move constructors, move assignments
  4. Perfect forwarding

# Task Log
1. `2025-08-19 13:20` AI assisted refactor of project structure after failed conversion of project to using c++ modules. no way of providing interface files as a static library was found. Modules depend on vendor specific implementation details and I gave up after banging my head and used AI to fix the broken project state
2. `2025-08-20 21:29` Explore C++ iterators with the goal of using them for the query API
3. `2025-08-22 14:32` Imlement query API
4. `2025-10-05 03:55` Primitive Assembly
5. `2025-10-06 13:14` Rasterization and shading
6. `2025-10-08 17:21` Add Resources to ECS
