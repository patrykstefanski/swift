extension L2 {
  @cxx @implementation
  public func f() -> Int32 { return a * 2 }   // accesses inherited root field `a`
}
