#ifndef __DELEGATORS_HPP__
#define __DELEGATORS_HPP__


#include <glib-object.h>
#include <functional>

namespace Delegator {

template<typename T>
void destroy_cxx_object_callback(gpointer ptr) {
  delete reinterpret_cast<T*>(ptr);
}

///////////////////////////////////////////////////////////////////////////////
template<typename Ret, typename... Args>
class Delegator {
  bool enabled;
public:
  Delegator() : enabled(true) {}
  virtual ~Delegator() {}
  virtual Ret emit(Args... args) = 0;

  void enable() { enabled = true; }
  void disable() { enabled = false; }
  bool is_enabled() { return enabled; }
  
  static Ret callback(Args... args, gpointer ptr) {
    Delegator* delegator = reinterpret_cast<Delegator*>(ptr);
    if (delegator->is_enabled())
      return delegator->emit(args...);
    return Ret();
  }
};

template<typename... Args>
class Delegator<void, Args...> {
  bool enabled;
public:
  Delegator() : enabled(true) {}
  virtual ~Delegator() {}
  virtual void emit(Args... args) = 0;
  
  void enable() { enabled = true; }
  void disable() { enabled = false; }
  bool is_enabled() { return enabled; }
  
  static void callback(Args... args, gpointer ptr) {
    Delegator* delegator = reinterpret_cast<Delegator*>(ptr);
    if (delegator->is_enabled())
      delegator->emit(args...);
  }
};

///////////////////////////////////////////////////////////////////////////////
template<typename Type, typename Ret, typename... Args>
class ObjectDelegator : public Delegator<Ret, Args...> 
{
public:
  typedef Type type;
  typedef Ret (Type::*Function)(Args...);

private:  
  Type*    obj;
  Function   func_ptr;

public:
  ObjectDelegator(Type* o, Function f) : obj(o), func_ptr(f) {};
  Ret emit(Args... args) {
    Ret result = Ret();
    if (obj)
      result = (obj->*func_ptr)(args...);
    if (result) return result;
    return result;
  }
};

template<typename Type, typename... Args>
class ObjectDelegator<Type, void, Args...> : public Delegator<void, Args...> 
{
public:
  typedef Type type;
  typedef void (Type::*Function)(Args...);

private:  
  Type*    obj;
  Function   func_ptr;

public:
  ObjectDelegator(Type* o, Function f) : obj(o), func_ptr(f) {};
  void emit(Args... args) {
    if (obj)
      (obj->*func_ptr)(args...);
  }
};

///////////////////////////////////////////////////////////////////////////////
template<typename Ret, typename... Args>
class FunctionDelegator :
  public Delegator<Ret, Args...> 
{
public:
  typedef Ret (*Function)(Args...);

private:  
  Function   func_ptr;

public:
  FunctionDelegator(Function f) : func_ptr(f) {};
  Ret emit(Args... args) {
    Ret result = Ret();
    if (func_ptr)
      result = (*func_ptr)(args...);
    return result;
  }
};

template<typename... Args>
class FunctionDelegator<void, Args...> :
  public Delegator<void, Args...> 
{
public:
  typedef void (*Function)(Args...);

private:  
  Function   func_ptr;

public:
  FunctionDelegator(Function f) : func_ptr(f) {};
  void emit(Args... args) {
    if (func_ptr)
      (*func_ptr)(args...);
  }
};

///////////////////////////////////////////////////////////////////////////////
template<typename Ret, typename... Args>
class CXXFunctionDelegator :
  public Delegator<Ret, Args...> 
{
public:
  typedef std::function<Ret (Args...)> Function;

private:  
  Function   func_ref;

public:
  CXXFunctionDelegator(Function f) : func_ref(f) {};
  Ret emit(Args... args) {
    Ret result = Ret();
    if (func_ref)
      result = func_ref(args...);
    return result;
  }
};

template<typename... Args>
class CXXFunctionDelegator<void, Args...> :
  public Delegator<void, Args...> 
{
public:
  typedef std::function<void (Args...)> Function;

private:  
  Function   func_ref;

public:
  CXXFunctionDelegator(Function f) : func_ref(f) {};
  void emit(Args... args) {
    if (func_ref)
      func_ref(args...);
  }
};

///////////////////////////////////////////////////////////////////////////////
template<typename Type, typename Ret, typename... Args>
inline ObjectDelegator<Type, Ret, Args...>* 
delegator(Type* obj, Ret (Type::*f)(Args...)) 
{
  return new ObjectDelegator<Type, Ret, Args...>(obj, f);
}

template<typename Ret, typename... Args>
inline FunctionDelegator<Ret, Args...>* 
delegator(Ret (*f)(Args...)) 
{
  return new FunctionDelegator<Ret, Args...>(f);
}

template<typename Ret, typename... Args>
inline CXXFunctionDelegator<Ret, Args...>* 
delegator(std::function<Ret (Args...)> f)
{
  return new CXXFunctionDelegator<Ret, Args...>(f);
}
///////////////////////////////////////////////////////////////////////////////
class Connection {
  gulong    handler_id;
  GObject*  target;
  gchar*    signal;
  GClosure* closure;
public:
  Connection(gulong handler_id_,
             GObject* target_,
             const gchar*   signal_,
             GClosure* closure_)
  {
    handler_id = handler_id_;
    target  = target_;
    signal  = g_strdup(signal_);
    closure = closure_;
  };

  ~Connection()
  {
      disconnect();
  }
  
  bool is_valid() { return handler_id > 0; }

  void disconnect() {
    if (target && closure) {
      gulong handler_id = g_signal_handler_find(gpointer(target), G_SIGNAL_MATCH_CLOSURE, 0, 0, closure, NULL, NULL);
      g_signal_handler_disconnect(gpointer(target), handler_id);
    }
    target  = NULL;
    closure = NULL;
  };
  
  void block() {
    if (target && closure) {
//      g_signal_handlers_block_matched (target,
//                                       GSignalMatchType(G_SIGNAL_MATCH_DETAIL|G_SIGNAL_MATCH_CLOSURE),
//                                       0, g_quark_from_static_string (signal), closure, NULL, NULL);
    }
    if (target)
      g_signal_handler_block (target, handler_id);
  }
  
  void unblock() {
    if (target && closure) {
//      g_signal_handlers_unblock_matched (target,
//                                       GSignalMatchType(G_SIGNAL_MATCH_DETAIL|G_SIGNAL_MATCH_CLOSURE),
//                                       0, g_quark_from_static_string (signal), closure, NULL, NULL);
    }
    if (target)
      g_signal_handler_unblock (target, handler_id);
  }
};


}; // namespace
///////////////////////////////////////////////////////////////////////////////
template<typename T>
void closure_destroy_notify(gpointer data, GClosure* closure)
{
  Delegator::destroy_cxx_object_callback<T>(data);
}

template<typename Ret, typename... Args>
Delegator::Connection *
g_signal_connect_delegator (GObject* target, 
                            const gchar* event, 
                            Delegator::Delegator<Ret, Args...>* delegator, 
                            bool after=false)
{
  GClosure *closure;  
  closure = g_cclosure_new (G_CALLBACK ((&Delegator::Delegator<Ret, Args...>::callback)),
                            (gpointer)delegator, 
                            closure_destroy_notify<Delegator::Delegator<Ret, Args...> >);
  gulong handler_id = g_signal_connect_closure (target, event, closure, after);
  return new Delegator::Connection(handler_id, target, event, closure);
}

template<typename T>
inline void g_object_set_cxx_object (GObject* target, const gchar* key, T* object)
{
  g_object_set_data_full(target, key, (gpointer)object, Delegator::destroy_cxx_object_callback<T>);
}

template<typename T>
inline void g_object_set_cxx_object (GObject* target, const GQuark key, T* object)
{
  g_object_set_qdata_full(target, key, (gpointer)object, Delegator::destroy_cxx_object_callback<T>);
}

inline gulong
g_signal_handler_get_id(GObject* target, GClosure* closure)
{
  return g_signal_handler_find(gpointer(target), G_SIGNAL_MATCH_CLOSURE, 0, 0, closure, NULL, NULL);
}

inline void
g_signal_disconnect (GObject* target,
                     GClosure* closure)
{
  gulong handler_id = g_signal_handler_get_id(target, closure);
	g_signal_handler_disconnect(gpointer(target), handler_id);
}

inline void
g_signal_disconnect (GObject* target,
                     gulong handler_id)
{
	g_signal_handler_disconnect(gpointer(target), handler_id);
}

#endif
