#include "reassembler.hh"
#include "debug.hh"

using namespace std;

Reassembler::Reassembler( ByteStream&& output )
  : output_( std::move( output ) )
  , storage( output_.writer().available_capacity() )
  , capacity_( output_.writer().available_capacity() )
  , cur_index_( 0 )
  , eof_index_( std::numeric_limits<uint64_t>::max() )
  , pending_bytes_( 0 )
{}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  auto start = max( first_index, cur_index_ );
  auto end = min( first_index + data.size(), cur_index_ + output_.writer().available_capacity() );
  if ( is_last_substring ) {
    eof_index_ = min( eof_index_, first_index + data.size() );
  }

  for ( uint64_t i = start; i < end; i++ ) {
    auto& t = storage[i % capacity_];
    if ( !t.second ) {
      t = make_pair( data[i - first_index], true );
      pending_bytes_++;
    } else if ( t.first != data[i - first_index] ) {
      throw runtime_error( "Substring not match!" );
    }
  }

  string whole_data;
  while ( cur_index_ < eof_index_ && storage[cur_index_ % capacity_].second ) {
    whole_data.push_back( storage[cur_index_ % capacity_].first );
    storage[cur_index_ % capacity_] = { 0, false };
    pending_bytes_--;
    cur_index_++;
  }
  output_.writer().push( whole_data );
  if ( cur_index_ == eof_index_ ) {
    output_.writer().close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  return pending_bytes_;
}
