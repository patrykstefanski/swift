// A C++ namespace containing a free function. The namespace imports into Swift
// as an enum (`mathz`), and `nsAdd` becomes a static method on it. This lets us
// implement the C++ function from a Swift extension of the namespace enum.
namespace mathz {
int nsAdd(int a, int b);
}
