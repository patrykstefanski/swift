import ForeignReferenceVirtual

// `super` references Base::describe before Swift defines it.
extension Derived {
  // int Derived::describe() const override;
  @cxx @implementation
  public func describe() -> Int32 { return super.describe() * 2 }
}

#if SWIFT_BASE
extension Base {
  // virtual int Base::describe() const;
  @cxx @implementation
  public func describe() -> Int32 { return value + 1000 }

  // virtual int Base::scaled(int factor) const;
  @cxx @implementation
  public func scaled(_ factor: Int32) -> Int32 { return value * factor + 1000 }
}
#endif

extension Leaf {
  // int Leaf::describe() const override;
  @cxx @implementation
  public func describe() -> Int32 { return super.describe() + 1 }

  // int Leaf::scaled(int factor) const override;
  @cxx @implementation
  public func scaled(_ factor: Int32) -> Int32 { return super.scaled(factor) * 3 }
}

extension Concrete {
  // int Concrete::run() const override;
  @cxx @implementation
  public func run() -> Int32 { return 1 }
}
