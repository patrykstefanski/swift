#ifndef TEST_INTEROP_CXX_STDLIB_INPUTS_CUSTOM_ITERATOR_ADL_H
#define TEST_INTEROP_CXX_STDLIB_INPUTS_CUSTOM_ITERATOR_ADL_H

#include <iterator>

// MARK: Case 1 - Iterator in a namespace with operator== in the same namespace.
// ADL finds operator== via namespace association.

namespace ns1 {

struct NamespacedInputIterator {
  int value;

  using iterator_category = std::input_iterator_tag;
  using value_type = int;
  using pointer = int *;
  using reference = const int &;
  using difference_type = int;

  NamespacedInputIterator(int value) : value(value) {}
  NamespacedInputIterator(const NamespacedInputIterator &) = default;

  const int &operator*() const { return value; }

  NamespacedInputIterator &operator++() {
    value++;
    return *this;
  }
  NamespacedInputIterator operator++(int) {
    auto tmp = *this;
    value++;
    return tmp;
  }
};

inline bool operator==(const NamespacedInputIterator &lhs,
                        const NamespacedInputIterator &rhs) {
  return lhs.value == rhs.value;
}

} // namespace ns1

// MARK: Case 2 - Iterator with hidden friend operator==.
// ADL discovers friend declarations that are only visible via ADL.

struct HiddenFriendInputIterator {
  int value;

  using iterator_category = std::input_iterator_tag;
  using value_type = int;
  using pointer = int *;
  using reference = const int &;
  using difference_type = int;

  HiddenFriendInputIterator(int value) : value(value) {}
  HiddenFriendInputIterator(const HiddenFriendInputIterator &) = default;

  const int &operator*() const { return value; }

  HiddenFriendInputIterator &operator++() {
    value++;
    return *this;
  }
  HiddenFriendInputIterator operator++(int) {
    auto tmp = *this;
    value++;
    return tmp;
  }

  friend bool operator==(const HiddenFriendInputIterator &lhs,
                          const HiddenFriendInputIterator &rhs) {
    return lhs.value == rhs.value;
  }
};

// MARK: Case 3 - Full random access iterator in a namespace with template
// operators (==, -) found via ADL, and member operator+=.
// operator+= must be a member because findOperatorByADL passes two args of the
// record type, while operator+= takes (RecordType&, DifferenceType).

namespace ns3 {

template <typename T>
struct NamespacedTemplatedRACIterator {
  T value;

  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using pointer = T *;
  using reference = const T &;
  using difference_type = int;

  NamespacedTemplatedRACIterator(T value) : value(value) {}
  NamespacedTemplatedRACIterator(const NamespacedTemplatedRACIterator &) =
      default;

  const T &operator*() const { return value; }

  NamespacedTemplatedRACIterator &operator++() {
    value++;
    return *this;
  }
  NamespacedTemplatedRACIterator operator++(int) {
    auto tmp = *this;
    value++;
    return tmp;
  }

  void operator+=(difference_type v) { value += v; }
};

template <typename T>
bool operator==(const NamespacedTemplatedRACIterator<T> &lhs,
                const NamespacedTemplatedRACIterator<T> &rhs) {
  return lhs.value == rhs.value;
}

template <typename T>
typename NamespacedTemplatedRACIterator<T>::difference_type
operator-(const NamespacedTemplatedRACIterator<T> &lhs,
          const NamespacedTemplatedRACIterator<T> &rhs) {
  return lhs.value - rhs.value;
}

using NamespacedTemplatedRACIteratorInt =
    NamespacedTemplatedRACIterator<int>;

} // namespace ns3

// MARK: Case 4 - operator== defined in a different namespace from the type.
// ADL searches the namespaces associated with the argument types. Since the
// type is in ns4_types but operator== is in ns4_ops, ADL won't find it.
// This iterator should NOT get conformance.

namespace ns4_types {

struct UnfindableEqIterator {
  int value;

  using iterator_category = std::input_iterator_tag;
  using value_type = int;
  using pointer = int *;
  using reference = const int &;
  using difference_type = int;

  UnfindableEqIterator(int value) : value(value) {}
  UnfindableEqIterator(const UnfindableEqIterator &) = default;

  const int &operator*() const { return value; }

  UnfindableEqIterator &operator++() {
    value++;
    return *this;
  }
};

} // namespace ns4_types

namespace ns4_ops {

inline bool operator==(const ns4_types::UnfindableEqIterator &lhs,
                        const ns4_types::UnfindableEqIterator &rhs) {
  return lhs.value == rhs.value;
}

} // namespace ns4_ops

#endif
