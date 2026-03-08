#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// How many sequence numbers are outstanding?
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return outstanding_count_;
}

// How many consecutive retransmissions have happened?
uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  while (true) {
    bool SYN_ {};
    bool FIN_ {};
    if ( next_seqno_ == 0 ) {
      SYN_ = true;
    }

    size_t payload_allowance = remaining_window(); 
    if ( SYN_ && payload_allowance > 0 ) {
      payload_allowance -= 1;
    }

    const size_t max_payload_size = min( TCPConfig::MAX_PAYLOAD_SIZE, payload_allowance );
    string payload_ = read_payload( max_payload_size );

    if ( reader().is_finished() && !fin_sent_ ) {
      if ( payload_allowance > payload_.size() ) {
        FIN_ = true;
        fin_sent_ = true;
      }
    }

    TCPSenderMessage msg = {
      .seqno = Wrap32::wrap( next_seqno_, isn_ ),
      .SYN = SYN_,
      .payload = std::move( payload_ ),
      .FIN = FIN_,
      .RST = input_.has_error(),
    };

    if (msg.sequence_length() == 0) {
      break;
    }

    push_outstanding( msg );
    transmit( msg );
  }
}

size_t TCPSender::remaining_window() const {
  uint64_t effective_window = window_size_ == 0 ? 1 : window_size_;
  uint64_t remaining_window = 0;
  if ( effective_window > sequence_numbers_in_flight() ) {
    remaining_window = effective_window - sequence_numbers_in_flight();
  }
  size_t payload_allowance = remaining_window;
  return payload_allowance;
}

string TCPSender::read_payload(size_t max_payload_size) {
  string payload;
  while ( reader().bytes_buffered() > 0 && payload.size() < max_payload_size ) {
    string_view view = reader().peek();
    if ( view.empty() ) {
      break;
    }
    size_t chunk_size = min( view.size(), max_payload_size - payload.size() );
    payload.append( view.substr( 0, chunk_size ) );
    input_.reader().pop( chunk_size );
  }
  return payload; 
}

void TCPSender::push_outstanding( const TCPSenderMessage& msg )
{
  uint64_t msg_end_seqno = next_seqno_ + msg.sequence_length();
  outstanding_segments_.push( { msg_end_seqno, msg } );
  next_seqno_ += msg.sequence_length();
  outstanding_count_ += msg.sequence_length();
  if (!timer.is_running()) {
    timer.start( initial_RTO_ms_ );
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return {
    .seqno = Wrap32::wrap( next_seqno_, isn_ ),
    .RST = input_.has_error(),
  };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if (msg.RST) {
    input_.reader().set_error();
  }
  window_size_ = msg.window_size;
  if (!msg.ackno.has_value()) {
    return;
  }
  uint64_t abs_ackno = msg.ackno->unwrap( isn_, next_seqno_ );

  if ( remove_outstanding(abs_ackno) ) {
    consecutive_retransmissions_ = 0;
    if ( outstanding_segments_.empty() ) {
      timer.stop();
    } else {
      timer.start( initial_RTO_ms_ );
    }
  }
}

bool TCPSender::remove_outstanding(uint64_t abs_ackno) {
  bool new_data_acked = false;

  while ( !outstanding_segments_.empty() ) {
    auto oldest_outstanding = outstanding_segments_.front();
    auto cur_seqno_end = oldest_outstanding.first;

    if ( abs_ackno >= cur_seqno_end && abs_ackno <= next_seqno_ ) {
      outstanding_segments_.pop();
      outstanding_count_ -= oldest_outstanding.second.sequence_length();
      new_data_acked = true;
    } else {
      break;
    }
  }

  return new_data_acked;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  timer.tick( ms_since_last_tick );
  if ( timer.is_expired() && !outstanding_segments_.empty() ) {
    auto oldest_outstanding = outstanding_segments_.front().second;
    transmit( oldest_outstanding );
    
    if ( window_size_ > 0 ) {
      consecutive_retransmissions_++;
      timer.double_RTO();
    }
    timer.restart();
  }
}

void Timer::tick( uint64_t ms_since_last_tick )
{
  time_elapsed_ms_ += ms_since_last_tick;
}

void Timer::start( uint64_t RTO_ms_ )
{
  current_RTO_ms_ = RTO_ms_;
  time_elapsed_ms_ = 0;
  timer_running = true;
}

void Timer::stop()
{
  time_elapsed_ms_ = 0;
  timer_running = false;
}

bool Timer::is_expired() const
{
  return time_elapsed_ms_ >= current_RTO_ms_;
}

void Timer::restart()
{
  time_elapsed_ms_ = 0;
}

void Timer::double_RTO() {
  current_RTO_ms_ *= 2;
}

bool Timer::is_running() const {
  return timer_running;
}
