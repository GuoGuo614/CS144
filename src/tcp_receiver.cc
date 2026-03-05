#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if ( message.RST ) {
    reassembler_.reader().set_error();
  }

  if ( !ISN_flag ) {
    if ( !message.SYN ) {
      return;
    }
    ISN = message.seqno;
    ISN_flag = true;
  }

  uint64_t checkpoint = reassembler_.writer().bytes_pushed() + 1;
  uint64_t abs_seqno = message.seqno.unwrap( ISN, checkpoint );
  uint64_t first_index = message.SYN ? 0 : abs_seqno - 1;

  reassembler_.insert( first_index, std::move( message.payload ), message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  std::optional<Wrap32> ack_opt {};
  if ( ISN_flag ) {
    ack_opt = ISN + reassembler_.writer().bytes_pushed() + 1 + ( reassembler_.writer().is_closed() ? 1 : 0 );
  }

  uint16_t win_size = min( reassembler_.writer().available_capacity(), static_cast<uint64_t>( UINT16_MAX ) );

  return { .ackno = ack_opt, .window_size = win_size, .RST = reassembler_.reader().has_error() };
}
