add_library(
  scheduler_lib
  STATIC
  scheduler.cpp
  scheduler.h
)

add_library(
  task_lib
  INTERFACE
)

target_sources(
  task_lib
  INTERFACE
)

target_link_libraries(
  scheduler_lib PUBLIC task_lib
)