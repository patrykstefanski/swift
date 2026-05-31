extension Vec {
  @cxx @implementation
  public static func make(_ v: Int32) -> Vec { return Vec(x: v) }

  @cxx @implementation
  public func dot(_ o: Vec) -> Int32 { return x * o.x }

  @cxx @implementation
  public mutating func scale(_ k: Int32) { x = x * k }
}
