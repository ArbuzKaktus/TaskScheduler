#include "scheduler.h"
#include <unordered_map>

void TTaskScheduler::executeAll() {
  try {
    TopSort();
    for (auto& task : tasks_) {
      task->Execute();
    }
  } catch(...) {
    throw;
  }
}

void TTaskScheduler::TopSort() {
  std::vector<std::shared_ptr<TaskBase>> vec;
  
  std::unordered_map<TaskBase*, VISIT> state;
  
  for (auto& task : tasks_) {
    state[task.get()] = NOT_VISITED;
  }
  
  for (auto& task : tasks_) {
    if (state[task.get()] == 0) {
      DFS(vec, task, state);
    }
  }
  
  
  
  tasks_ = std::move(vec);
}

void TTaskScheduler::DFS(
    std::vector<std::shared_ptr<TaskBase>>& vec, 
    std::shared_ptr<TaskBase> start,
    std::unordered_map<TaskBase*, VISIT>& state) {
  
  VISIT& current_state = state[start.get()];
  
  if (current_state == VISITED) {
    return;
  }
  
  if (current_state == VISITING) {
    throw std::runtime_error("Dependency cycle detected!");
  }
  
  current_state = VISITING;
  
  for (auto& dep : start->GetDependecies()) {
    DFS(vec, dep, state);
  }
  
  current_state = VISITED;
  
  vec.push_back(start);
}