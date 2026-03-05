#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + static_cast<uint32_t>( n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint32_t offset = raw_value_ - zero_point.raw_value_;
  auto checkpoint_offset = static_cast<uint32_t>(checkpoint);
  
  uint32_t diff = offset - checkpoint_offset;
  
  if (diff <= (1U << 31) || checkpoint + diff < (1ULL << 32)) {
    return checkpoint + diff;
  }
  return checkpoint - (1ULL << 32) + diff;
}
