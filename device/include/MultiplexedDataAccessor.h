#warning Including MultiplexDataAccessor.h is deprecated, include RegisterAccessor2D.h instead.
#include "RegisterAccessor2Dimpl.h"

namespace mtca4u {
  // Class backwards compatibility only. DEPCRECATED, DO NOT USE
  // @todo add printed warning after release of 0.6
  template<typename UserType>
  class MultiplexedDataAccessor : public RegisterAccessor2Dimpl<UserType> {};
}
