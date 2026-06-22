// Swift implementations of FRT-taking/returning C++ free functions, built into
// the executable for cxx-impl-frt-execution.cpp. Only the supported phase-1
// cases: a borrowed FRT parameter, and a `+1` (SWIFT_RETURNS_RETAINED) result.
@cxx @implementation
public func valueOf(_ n: Node) -> Int32 { return n.value }

@cxx @implementation
public func dup(_ n: Node) -> Node { return n }
