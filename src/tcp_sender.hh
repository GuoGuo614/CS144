#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <functional>
#include <queue>

class Timer
{
public:
  void tick( uint64_t ms_since_last_tick );
  void start( uint64_t RTO_ms_ );
  void stop();
  bool is_expired() const;
  void restart();
  void double_RTO();
  bool is_running() const;
  uint64_t current_RTO_ms_;

private:
  uint64_t time_elapsed_ms_;
  bool timer_running;
};

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ), outstanding_segments_(), timer()
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive retransmissions have happened?
  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }

private:
  Reader& reader() { return input_.reader(); }
  void push_outstanding( const TCPSenderMessage& msg );
  size_t remaining_window() const;
  std::string read_payload(size_t max_payload_size);
  bool remove_outstanding(uint64_t abs_ackno);

  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  uint64_t next_seqno_ { 0 };
  uint64_t consecutive_retransmissions_ { 0 };
  uint64_t outstanding_count_ { 0 };
  uint16_t window_size_ { 1 };
  bool fin_sent_ {};

  std::queue<std::pair<uint64_t, TCPSenderMessage>> outstanding_segments_;
  Timer timer;
};
