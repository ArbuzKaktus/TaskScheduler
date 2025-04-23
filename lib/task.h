#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
struct is_member_function_pointer : std::is_member_function_pointer<T> {};

template <typename T> class FutureResult;

class TaskBase {
public:
  TaskBase() : executed_(false) {}

  virtual ~TaskBase() {}

  virtual void Execute() = 0;

  bool IsExecuted() const { return executed_; }

  void AddDependendTask(std::shared_ptr<TaskBase> task) {
    dependencies_.push_back(task);
  }

  const std::vector<std::shared_ptr<TaskBase>> GetDependecies() {
    return dependencies_;
  }

protected:
  bool executed_;
  std::vector<std::shared_ptr<TaskBase>> dependencies_;
};

template <typename ReturnType> 
class Task : public TaskBase {
public:
  template <typename Callable,
  typename = std::enable_if_t<!is_member_function_pointer<std::decay_t<Callable>>::value>>
  Task(Callable&& callable)
      : callable_([callable = std::forward<Callable>(callable)]() {
          return callable();
        }) {}

  template <typename Callable, typename Arg1,
  typename = std::enable_if_t<!is_member_function_pointer<std::decay_t<Callable>>::value>>
  Task(Callable&& callable, Arg1&& arg1)
      : callable_([callable = std::forward<Callable>(callable),
                   arg1 = std::forward<Arg1>(arg1)]() {
          return callable(getValue(arg1));
        }) {}

  template <typename Callable, typename Arg1, typename Arg2,
  typename = std::enable_if_t<!is_member_function_pointer<std::decay_t<Callable>>::value>>
  Task(Callable&& callable, Arg1&& arg1, Arg2&& arg2)
      : callable_([callable = std::forward<Callable>(callable),
                   arg1 = std::forward<Arg1>(arg1),
                   arg2 = std::forward<Arg2>(arg2)]() {
          return callable(getValue(arg1), getValue(arg2));
        }) {}

  template <typename ClassType>
  Task(ReturnType (ClassType::*method)() const, ClassType& instance)
      : callable_([method, &instance]() { return (instance.*method)(); }) {}
        
  template <typename ClassType, typename T>
  Task(ReturnType (ClassType::*method)(T) const, ClassType &instance,
       const FutureResult<T> &future)
      : callable_([method, &instance, future]() {
          return (instance.*method)(getValue(future));
        }) {}

  template <typename ClassType, typename T>
  Task(ReturnType (ClassType::*method)(const T&) const, ClassType &instance,
        const FutureResult<T>& future)
      : callable_([method, &instance, future]() {
          return (instance.*method)(getValue(future));
        }) {}

  template <typename ClassType, typename T>
  Task(ReturnType (ClassType::*method)(const T&), ClassType &instance,
        const FutureResult<T>& future)
      : callable_([method, &instance, future]() {
          return (instance.*method)(getValue(future));
        }) {}
  
  template <typename ClassType, typename Arg1>
  Task(ReturnType (ClassType::*method)(Arg1), ClassType& instance, Arg1&& arg1)
      : callable_([method, &instance, arg1 = std::forward<Arg1>(arg1)]() {
          return (instance.*method)(getValue(arg1));
        }) {}

  template <typename ClassType, typename Arg1>
  Task(ReturnType (ClassType::*method)(Arg1) const, ClassType& instance,
       Arg1&& arg1)
      : callable_([method, &instance, arg1 = std::forward<Arg1>(arg1)]() {
          return (instance.*method)(getValue(arg1));
        }) {}

  void Execute() override {
    if (!IsExecuted()) {
      try {
        if (!dependencies_.empty()) {
          for (auto& elem: dependencies_) {
            elem->Execute();
          }
        }
        result_ = callable_();
        executed_ = true;
      }
      catch(...) {
        throw;
      }
    }
  }

  const ReturnType& GetResult() {
    if (!IsExecuted()) {
      Execute();
    }
    return result_;
  }

  
private:
  std::function<ReturnType()> callable_;
  ReturnType result_;
  template <typename T> friend class FutureResult;

  template <typename T>
  static const T& getValue(const FutureResult<T> &future) {
    return future.get();
  }

  template <typename Arg> static Arg getValue(const Arg& arg) { return arg; }
};

template <> 
class Task<void> : public TaskBase {
public:
  template <typename Callable,
  typename = std::enable_if_t<!is_member_function_pointer<std::decay_t<Callable>>::value>>
  Task(Callable&& callable)
      : callable_([callable = std::forward<Callable>(callable)]() {
          callable();
        }) {}

  template <typename Callable, typename Arg1,
  typename = std::enable_if_t<!is_member_function_pointer<std::decay_t<Callable>>::value>>
  Task(Callable&& callable, Arg1&& arg1)
      : callable_([callable = std::forward<Callable>(callable),
                   arg1 = std::forward<Arg1>(arg1)]() {
          callable(getValue(arg1));
        }) {}

  template <typename Callable, typename Arg1, typename Arg2,
  typename = std::enable_if_t<!is_member_function_pointer<std::decay_t<Callable>>::value>>
  Task(Callable&& callable, Arg1&& arg1, Arg2&& arg2)
      : callable_([callable = std::forward<Callable>(callable),
                   arg1 = std::forward<Arg1>(arg1),
                   arg2 = std::forward<Arg2>(arg2)]() {
          callable(getValue(arg1), getValue(arg2));
        }) {}

  template <typename ClassType>
  Task(void (ClassType::*method)() const, ClassType& instance)
      : callable_([method, &instance]() { (instance.*method)(); }) {}
        
  template <typename ClassType, typename T>
  Task(void (ClassType::*method)(T) const, ClassType &instance,
       const FutureResult<T> &future)
      : callable_([method, &instance, future]() {
          (instance.*method)(getValue(future));
        }) {}

  template <typename ClassType, typename T>
  Task(void (ClassType::*method)(const T&) const, ClassType &instance,
        const FutureResult<T>& future)
      : callable_([method, &instance, future]() {
          (instance.*method)(getValue(future));
        }) {}

  template <typename ClassType, typename T>
  Task(void (ClassType::*method)(const T&), ClassType &instance,
        const FutureResult<T>& future)
      : callable_([method, &instance, future]() {
          (instance.*method)(getValue(future));
        }) {}
  
  template <typename ClassType, typename Arg1>
  Task(void (ClassType::*method)(Arg1), ClassType& instance, Arg1&& arg1)
      : callable_([method, &instance, arg1 = std::forward<Arg1>(arg1)]() {
          (instance.*method)(getValue(arg1));
        }) {}

  template <typename ClassType, typename Arg1>
  Task(void (ClassType::*method)(Arg1) const, ClassType& instance,
       Arg1&& arg1)
      : callable_([method, &instance, arg1 = std::forward<Arg1>(arg1)]() {
          (instance.*method)(getValue(arg1));
        }) {}

  void Execute() override {
    if (!IsExecuted()) {
      try {
        if (!dependencies_.empty()) {
          for (auto& elem: dependencies_) {
            elem->Execute();
          }
        }
        callable_();
        executed_ = true;
      }
      catch(...) {
        throw;
      }
    }
  }

  void GetResult() {
    if (!IsExecuted()) {
      Execute();
    }
  }

private:
  std::function<void()> callable_;

  template <typename T>
  static const T& getValue(const FutureResult<T> &future) {
    return future.get();
  }

  template <typename Arg> static Arg getValue(const Arg& arg) { return arg; }
};


template <typename T> class FutureResult {
public:
  using value_type = T;
  explicit FutureResult(std::shared_ptr<Task<T>> task) : task_(task) {}

  const T& get() const { return task_->result_; }

  std::shared_ptr<Task<T>> getTask() const { return task_; }

private:
  std::shared_ptr<Task<T>> task_;
};

