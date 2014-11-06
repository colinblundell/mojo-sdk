// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_IMPL_H_
#define MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_IMPL_H_

#include "third_party/mojo/public/cpp/bindings/interface_request.h"
#include "third_party/mojo/public/cpp/bindings/lib/interface_impl_internal.h"
#include "third_party/mojo/public/cpp/environment/environment.h"
#include "third_party/mojo/public/cpp/system/macros.h"

namespace mojo {

// InterfaceImpl<..> is designed to be the base class of an interface
// implementation. It may be bound to a pipe or a proxy, see BindToPipe and
// BindToProxy.
template <typename Interface>
class InterfaceImpl : public internal::InterfaceImplBase<Interface> {
 public:
  typedef typename Interface::Client Client;
  typedef Interface ImplementedInterface;

  InterfaceImpl() : internal_state_(this) {}
  virtual ~InterfaceImpl() {}

  // Returns a proxy to the client interface. This is null upon construction,
  // and becomes non-null after OnClientConnected. NOTE: It remains non-null
  // until this instance is deleted.
  Client* client() { return internal_state_.client(); }

  // Blocks the current thread for the first incoming method call, i.e., either
  // a call to a method or a client callback method. Returns |true| if a method
  // has been called, |false| in case of error. It must only be called on a
  // bound object.
  bool WaitForIncomingMethodCall() {
    return internal_state_.WaitForIncomingMethodCall();
  }

  // Called when the client has connected to this instance.
  virtual void OnConnectionEstablished() {}

  // Called when the client is no longer connected to this instance. NOTE: The
  // client() method continues to return a non-null pointer after this method
  // is called. After this method is called, any method calls made on client()
  // will be silently ignored.
  virtual void OnConnectionError() {}

  // DO NOT USE. Exposed only for internal use and for testing.
  internal::InterfaceImplState<Interface>* internal_state() {
    return &internal_state_;
  }

 private:
  internal::InterfaceImplState<Interface> internal_state_;
  MOJO_DISALLOW_COPY_AND_ASSIGN(InterfaceImpl);
};

// Takes an instance of an InterfaceImpl<..> subclass and binds it to the given
// MessagePipe. The instance is returned for convenience in member initializer
// lists, etc.
//
// If the pipe is closed, the instance's OnConnectionError method will be called
// and then the instance will be deleted.
//
// The instance is also bound to the current thread. Its methods will only be
// called on the current thread, and if the current thread exits, then the end
// point of the pipe will be closed and the error handler's OnConnectionError
// method will be called.
//
// Before returning, the instance's OnConnectionEstablished method is called.
template <typename Impl>
Impl* BindToPipe(
    Impl* instance,
    ScopedMessagePipeHandle handle,
    const MojoAsyncWaiter* waiter = Environment::GetDefaultAsyncWaiter()) {
  instance->internal_state()->Bind(handle.Pass(), true, waiter);
  return instance;
}

// Like BindToPipe but does not delete the instance after a channel error.
template <typename Impl>
Impl* WeakBindToPipe(
    Impl* instance,
    ScopedMessagePipeHandle handle,
    const MojoAsyncWaiter* waiter = Environment::GetDefaultAsyncWaiter()) {
  instance->internal_state()->Bind(handle.Pass(), false, waiter);
  return instance;
}

// Takes an instance of an InterfaceImpl<..> subclass and binds it to the given
// InterfacePtr<..>. The instance is returned for convenience in member
// initializer lists, etc. If the pipe is closed, the instance's
// OnConnectionError method will be called and then the instance will be
// deleted.
//
// The instance is also bound to the current thread. Its methods will only be
// called on the current thread, and if the current thread exits, then it will
// also be deleted, and along with it, its end point of the pipe will be closed.
//
// Before returning, the instance's OnConnectionEstablished method is called.
template <typename Impl, typename Interface>
Impl* BindToProxy(
    Impl* instance,
    InterfacePtr<Interface>* ptr,
    const MojoAsyncWaiter* waiter = Environment::GetDefaultAsyncWaiter()) {
  instance->internal_state()->BindProxy(ptr, true, waiter);
  return instance;
}

// Like BindToProxy but does not delete the instance after a channel error.
template <typename Impl, typename Interface>
Impl* WeakBindToProxy(
    Impl* instance,
    InterfacePtr<Interface>* ptr,
    const MojoAsyncWaiter* waiter = Environment::GetDefaultAsyncWaiter()) {
  instance->internal_state()->BindProxy(ptr, false, waiter);
  return instance;
}

// Takes an instance of an InterfaceImpl<..> subclass and binds it to the given
// InterfaceRequest<..>. The instance is returned for convenience in member
// initializer lists, etc. If the pipe is closed, the instance's
// OnConnectionError method will be called and then the instance will be
// deleted.
//
// The instance is also bound to the current thread. Its methods will only be
// called on the current thread, and if the current thread exits, then it will
// also be deleted, and along with it, its end point of the pipe will be closed.
//
// Before returning, the instance will receive a SetClient call, providing it
// with a proxy to the client on the other end of the pipe.
template <typename Impl, typename Interface>
Impl* BindToRequest(
    Impl* instance,
    InterfaceRequest<Interface>* request,
    const MojoAsyncWaiter* waiter = Environment::GetDefaultAsyncWaiter()) {
  return BindToPipe(instance, request->PassMessagePipe(), waiter);
}

// Like BindToRequest but does not delete the instance after a channel error.
template <typename Impl, typename Interface>
Impl* WeakBindToRequest(
    Impl* instance,
    InterfaceRequest<Interface>* request,
    const MojoAsyncWaiter* waiter = Environment::GetDefaultAsyncWaiter()) {
  return WeakBindToPipe(instance, request->PassMessagePipe(), waiter);
}

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_IMPL_H_
