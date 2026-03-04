#include "byte_stream.hh"
#include "debug.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity )
  , buffer_( capacity + 1 )
  , written_bytes_( 0 )
  , read_bytes_( 0 )
  , head_( 0 )
  , tail_( capacity_ )
{}

uint64_t ByteStream::buffer_size() const
{
  return ( tail_ - head_ + 1 + capacity_ + 1 ) % ( capacity_ + 1 );
}

// Push data to stream, but only as much as available capacity allows.
void Writer::push( string data )
{
  auto num = min( data.size(), available_capacity() );
  for ( uint64_t i = 0; i < num; i++ ) {
    tail_ = ( tail_ + 1 ) % ( capacity_ + 1 );
    buffer_[tail_] = data[i];
  }
  written_bytes_ += num;
}

// Signal that the stream has reached its ending. Nothing more will be written.
void Writer::close()
{
  write_ended_flag_ = true;
}

// Has the stream been closed?
bool Writer::is_closed() const
{
  return write_ended_flag_;
}

// How many bytes can be pushed to the stream right now?
uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_size();
}

// Total number of bytes cumulatively pushed to the stream
uint64_t Writer::bytes_pushed() const
{
  return written_bytes_;
}

// Peek at the next bytes in the buffer -- ideally as many as possible.
// It's not required to return a string_view of the *whole* buffer, but
// if the peeked string_view is only one byte at a time, it will probably force
// the caller to do a lot of extra work.
string_view Reader::peek() const
{
  const uint64_t size = buffer_size();
  if ( size == 0 ) {
    return {};
  }

  const uint64_t len = ( head_ <= tail_ ) ? ( tail_ - head_ + 1 ) : ( capacity_ + 1 - head_ );
  return { &buffer_[head_], len };
}

// Remove `len` bytes from the buffer.
void Reader::pop( uint64_t len )
{
  const uint64_t num = min( len, bytes_buffered() );
  head_ = ( head_ + num ) % ( capacity_ + 1 );
  read_bytes_ += num;
}

// Is the stream finished (closed and fully popped)?
bool Reader::is_finished() const
{
  const bool fully_popped = bytes_buffered() == 0;
  return fully_popped && write_ended_flag_;
}

// Number of bytes currently buffered (pushed and not popped)
uint64_t Reader::bytes_buffered() const
{
  return buffer_size();
}

// Total number of bytes cumulatively popped from stream
uint64_t Reader::bytes_popped() const
{
  return read_bytes_;
}