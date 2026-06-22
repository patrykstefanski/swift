// Swift implementation of a virtual method of a foreign-reference type, built
// into the executable for cxx-impl-frt-virtual-execution.cpp. As the class's key
// function, this drives Swift to emit Animal's vtable/RTTI; the vtable slot points
// at this body, so a C++ virtual call reaches it.
extension Animal {
  @cxx @implementation
  public func sound() -> Int32 { return id * 10 }
}
