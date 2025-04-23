#pragma once

#include <memory>
#include <type_traits>
#include <vector>

#include "task.h"

enum VISIT {
  NOT_VISITED,
  VISITING,
  VISITED
};

template <typename T> struct is_future_result : std::false_type {};
template <typename T>
struct is_future_result<FutureResult<T>> : std::true_type {};


class TTaskScheduler {

public:
  template <typename Callable, typename = std::enable_if_t<!is_member_function_pointer<std::decay_t<Callable>>::value>> 
  auto add(Callable&& callable) {
    using ReturnType = decltype(callable());
    auto task =
        std::make_shared<Task<ReturnType>>(std::forward<Callable>(callable));
    tasks_.push_back(task);
    return task;
  }

  template <
      typename Callable, typename Arg1,
      typename = std::enable_if_t<!is_future_result<std::decay_t<Arg1>>::value &&
                                  !is_member_function_pointer<std::decay_t<Callable>>::value>>
  auto add(Callable&& callable, Arg1&& arg1) {
    using ReturnType = decltype(callable(std::declval<std::decay_t<Arg1>>()));
    auto task = std::make_shared<Task<ReturnType>>(
        std::forward<Callable>(callable), std::forward<Arg1>(arg1));
    tasks_.push_back(task);
    return task;
  }

  template <typename Callable, typename T,
  typename = std::enable_if_t<!is_member_function_pointer<std::decay_t<Callable>>::value>>
  auto add(Callable&& callable, const FutureResult<T>& future) {
    using ReturnType = decltype(callable(std::declval<T>()));
    auto task = std::make_shared<Task<ReturnType>>(
        std::forward<Callable>(callable), future);

    task->AddDependendTask(future.getTask());

    tasks_.push_back(task);
    return task;
  }

  template <typename Callable, typename Arg1, typename Arg2,
            typename =
                std::enable_if_t<!is_future_result<std::decay_t<Arg1>>::value &&
                                 !is_future_result<std::decay_t<Arg2>>::value &&
                                 !is_member_function_pointer<std::decay_t<Callable>>::value>>
  auto add(Callable&& callable, Arg1&& arg1, Arg2&& arg2) {
    using ReturnType = decltype(callable(std::declval<std::decay_t<Arg1>>(),
                                         std::declval<std::decay_t<Arg2>>()));

    auto task = std::make_shared<Task<ReturnType>>(
        std::forward<Callable>(callable), std::forward<Arg1>(arg1),
        std::forward<Arg2>(arg2));

    tasks_.push_back(task);
    return task;
  }

  template <
      typename Callable, typename T, typename Arg2,
      typename = std::enable_if_t<!is_future_result<std::decay_t<Arg2>>::value &&
                                  !is_member_function_pointer<std::decay_t<Callable>>::value>>
  auto add(Callable &&callable, const FutureResult<T>& future, Arg2&& arg2) {
    using ReturnType = decltype(callable(std::declval<T>(),
                                         std::declval<std::decay_t<Arg2>>()));

    auto task = std::make_shared<Task<ReturnType>>(
        std::forward<Callable>(callable), future, std::forward<Arg2>(arg2));

    task->AddDependendTask(future.getTask());

    tasks_.push_back(task);
    return task;
  }

  template <
      typename Callable, typename Arg1, typename T,
      typename = std::enable_if_t<!is_future_result<std::decay_t<Arg1>>::value &&
                                  !is_member_function_pointer<std::decay_t<Callable>>::value>>
  auto add(Callable&& callable, Arg1&& arg1, const FutureResult<T>& future) {
    using ReturnType = decltype(callable(std::declval<std::decay_t<Arg1>>(),
                                         std::declval<T>()));

    auto task = std::make_shared<Task<ReturnType>>(
        std::forward<Callable>(callable), std::forward<Arg1>(arg1), future);

    task->AddDependendTask(future.getTask());

    tasks_.push_back(task);
    return task;
  }

  template <typename Callable, typename T1, typename T2,
            typename = std::enable_if_t<!is_member_function_pointer<std::decay_t<Callable>>::value>>
  auto add(Callable&& callable, const FutureResult<T1>& future1,
           const FutureResult<T2>& future2) {
    using ReturnType =
        decltype(callable(std::declval<T1>(), std::declval<T2>()));

    auto task = std::make_shared<Task<ReturnType>>(
        std::forward<Callable>(callable), future1, future2);

    task->AddDependendTask(future1.getTask());
    task->AddDependendTask(future2.getTask());

    tasks_.push_back(task);
    return task;
  }

  template <typename ReturnType, typename ClassType>
  auto add(ReturnType (ClassType::*method)(), ClassType& instance) {
    auto task = std::make_shared<Task<ReturnType>>(method, instance);
    tasks_.push_back(task);
    return task;
  }

  template <typename ReturnType, typename ClassType>
  auto add(ReturnType (ClassType::*method)() const, ClassType& instance) {
    auto task = std::make_shared<Task<ReturnType>>(method, instance);
    tasks_.push_back(task);
    return task;
  }

  template <typename ReturnType, typename ClassType, typename T>
  auto add(ReturnType (ClassType::*method)(const T&) const, 
          ClassType& instance,
          const FutureResult<T>& future) {
    auto task = std::make_shared<Task<ReturnType>>(method, instance, future);
    task->AddDependendTask(future.getTask());
    tasks_.push_back(task);
    return task;
  }

  template <typename ReturnType, typename ClassType, typename T>
  auto add(ReturnType (ClassType::*method)(const T&), 
          ClassType& instance,
          const FutureResult<T>& future) {
    auto task = std::make_shared<Task<ReturnType>>(method, instance, future);
    task->AddDependendTask(future.getTask());
    tasks_.push_back(task);
    return task;
  }

  template <
      typename ReturnType, typename ClassType, typename Arg,
      typename = std::enable_if_t<!is_future_result<std::decay_t<Arg>>::value>>
  auto add(ReturnType (ClassType::*method)(Arg), ClassType& instance,
           Arg&& arg) {
    auto task = std::make_shared<Task<ReturnType>>(method, instance,
                                                   std::forward<Arg>(arg));

    tasks_.push_back(task);
    return task;
  }

  template <
      typename ReturnType, typename ClassType, typename Arg,
      typename = std::enable_if_t<!is_future_result<std::decay_t<Arg>>::value>>
  auto add(ReturnType (ClassType::*method)(Arg) const, ClassType& instance,
           Arg&& arg) {
    auto task = std::make_shared<Task<ReturnType>>(method, instance,
                                                   std::forward<Arg>(arg));

    tasks_.push_back(task);
    return task;
  }

  template <typename ReturnType, typename ClassType, typename T>
  auto add(ReturnType (ClassType::*method)(T), ClassType& instance,
           const FutureResult<T>& future) {
    auto task = std::make_shared<Task<ReturnType>>(method, instance, future);

    task->AddDependendTask(future.getTask());

    tasks_.push_back(task);
    return task;
  }

  template <typename ReturnType, typename ClassType, typename T>
  auto add(ReturnType (ClassType::*method)(T) const, ClassType& instance,
           const FutureResult<T>& future) {
    auto task = std::make_shared<Task<ReturnType>>(method, instance, future);

    task->AddDependendTask(future.getTask());

    tasks_.push_back(task);
    return task;
  }

  template <typename T>
  FutureResult<T> getFutureResult(std::shared_ptr<Task<T>> task) {
    return FutureResult<T>(task);
  }

  template <typename T> 
  const T& getResult(std::shared_ptr<Task<T>> task) {
    return task->GetResult();
  }

  void executeAll();

private:
  std::vector<std::shared_ptr<TaskBase>> tasks_;
  template <typename T> 
  void executeTask(std::shared_ptr<Task<T>> task) {
    if (task->IsExecuted()) {
      return;
    }

    for (auto &dep : task->GetDependecies()) {
      executeTaskBase(dep);
    }

    task->Execute();
  }

  void executeTaskBase(std::shared_ptr<TaskBase> task) {
    if (!task->IsExecuted()) {
      
      for (auto &dep : task->GetDependecies()) {
        executeTaskBase(dep);
      }

      task->Execute();
    }
  }

  template <typename TaskType, typename T>
  void addDependencyIfFuture(std::shared_ptr<TaskType> task,
                             const FutureResult<T>& future) {
    task->AddDependendTask(future.getTask());
  }

  template <typename TaskType, typename Arg>
  void addDependencyIfFuture(std::shared_ptr<TaskType>, const Arg& ) { }

  void DFS(std::vector<std::shared_ptr<TaskBase>>& vec,
           std::shared_ptr<TaskBase> start,
           std::unordered_map<TaskBase *, VISIT>& state);
  void TopSort();
};