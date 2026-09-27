#include "platform/platformNet.h"
#include "platform/event.h"
#include "console/console.h"
#include "core/strings/stringFunctions.h"
#include "core/util/journal/process.h"
#include "core/util/tVector.h"

#include <SDL3_net/SDL_net.h>

static const S32 sDefaultPort = 28000;

ConnectionNotifyEvent   Net::smConnectionNotify;
ConnectionAcceptedEvent Net::smConnectionAccept;
ConnectionReceiveEvent  Net::smConnectionReceive;
PacketReceiveEvent      Net::smPacketReceive;


//-----------------------------------------------------------------------------
// Handle table
//
// platformNet.h hands out NetSocket as an integer and expects the BSD sequence
// of open, bind, then listen. SDL_net has no socket before you say what it is
// for -- NET_CreateServer binds and listens in one call -- so a handle starts
// out as nothing but a remembered port and becomes a server, a stream or a
// datagram socket when the engine says which it wanted.
//-----------------------------------------------------------------------------

namespace
{

enum EntryKind
{
   KindUnused,
   KindPending,    ///< openSocket has been called, nothing decided yet
   KindServer,
   KindStream,
   KindDatagram
};

enum LinkState
{
   StateNone,
   StateResolving, ///< waiting on NET_ResolveHostname
   StateConnecting,
   StateConnected
};

struct Entry
{
   EntryKind          kind;
   LinkState          state;

   NET_Server        *server;
   NET_StreamSocket  *stream;
   NET_DatagramSocket*datagram;

   NET_Address       *pending;  ///< host being resolved for openConnectTo
   U16                port;     ///< bind() target, or the port to connect to

   Entry() { reset(); }

   void reset()
   {
      kind = KindUnused;
      state = StateNone;
      server = NULL;
      stream = NULL;
      datagram = NULL;
      pending = NULL;
      port = 0;
   }
};

Vector<Entry> gEntries( __FILE__, __LINE__ );

Entry *entryFor( NetSocket socket )
{
   if ( socket < 0 || socket >= NetSocket( gEntries.size() ) )
      return NULL;

   Entry &entry = gEntries[ socket ];
   return entry.kind == KindUnused ? NULL : &entry;
}

NetSocket allocEntry( EntryKind kind )
{
   for ( U32 i = 0; i < gEntries.size(); ++i )
   {
      if ( gEntries[i].kind == KindUnused )
      {
         gEntries[i].reset();
         gEntries[i].kind = kind;
         return NetSocket( i );
      }
   }

   gEntries.increment();
   gEntries.last().reset();
   gEntries.last().kind = kind;

   return NetSocket( gEntries.size() - 1 );
}

void releaseEntry( Entry *entry )
{
   if ( entry->server )
      NET_DestroyServer( entry->server );
   if ( entry->stream )
      NET_DestroyStreamSocket( entry->stream );
   if ( entry->datagram )
      NET_DestroyDatagramSocket( entry->datagram );
   if ( entry->pending )
      NET_UnrefAddress( entry->pending );

   entry->reset();
}

/// The one unreliable socket the engine is allowed to open.
NetSocket gUdpSocket = InvalidSocket;
S32       gUdpPort = 0;

void addressFromNet( NET_Address *address, U16 port, NetAddress *out )
{
   dMemset( out, 0, sizeof( NetAddress ) );
   out->type = NetAddress::IPAddress;
   out->port = port;

   const char *text = address ? NET_GetAddressString( address ) : NULL;

   U32 quad[ 4 ];

   if ( text && dSscanf( text, "%u.%u.%u.%u", &quad[0], &quad[1], &quad[2], &quad[3] ) == 4 )
      for ( U32 i = 0; i < 4; i++ )
         out->netNum[ i ] = U8( quad[ i ] );
}

/// NetAddress carries the address as raw bytes; SDL_net wants it as text.
NET_Address *addressToNet( const NetAddress *address )
{
   char buffer[ 32 ];
   dSprintf( buffer, sizeof( buffer ), "%d.%d.%d.%d",
             address->netNum[0], address->netNum[1],
             address->netNum[2], address->netNum[3] );

   NET_Address *resolved = NET_ResolveHostname( buffer );
   if ( !resolved )
      return NULL;

   // Numeric, so this does not actually wait.
   if ( NET_WaitUntilResolved( resolved, -1 ) != NET_SUCCESS )
   {
      NET_UnrefAddress( resolved );
      return NULL;
   }

   return resolved;
}

} // namespace


//-----------------------------------------------------------------------------
// Lifetime
//-----------------------------------------------------------------------------

bool Net::init()
{
   if ( !NET_Init() )
   {
      Con::errorf( "Net::init - NET_Init failed: %s", SDL_GetError() );
      return false;
   }

   Process::notify( &Net::process, PROCESS_NET_ORDER );

   return true;
}

void Net::shutdown()
{
   Process::remove( &Net::process );

   closePort();

   for ( U32 i = 0; i < gEntries.size(); ++i )
      if ( gEntries[i].kind != KindUnused )
         releaseEntry( &gEntries[i] );

   gEntries.clear();

   NET_Quit();
}


//-----------------------------------------------------------------------------
// Unreliable (UDP)
//-----------------------------------------------------------------------------

bool Net::openPort( S32 port, bool doBind )
{
   closePort();

   const SDL_PropertiesID props = SDL_CreateProperties();

   SDL_SetBooleanProperty( props, NET_PROP_DATAGRAM_SOCKET_ALLOW_BROADCAST_BOOLEAN, true );

   NET_DatagramSocket *socket = NET_CreateDatagramSocket( NULL, doBind ? U16( port ) : 0, props );

   SDL_DestroyProperties( props );

   if ( !socket )
   {
      Con::errorf( "Net::openPort - could not bind port %d: %s", port, SDL_GetError() );
      return false;
   }

   gUdpSocket = allocEntry( KindDatagram );
   gEntries[ gUdpSocket ].datagram = socket;
   gUdpPort = port;

   return true;
}

NetSocket Net::getPort()
{
   return gUdpSocket;
}

void Net::closePort()
{
   if ( gUdpSocket == InvalidSocket )
      return;

   if ( Entry *entry = entryFor( gUdpSocket ) )
      releaseEntry( entry );

   gUdpSocket = InvalidSocket;
}

Net::Error Net::sendto( const NetAddress *address, const U8 *buffer, S32 bufferSize )
{
   Entry *entry = entryFor( gUdpSocket );
   if ( !entry || !entry->datagram )
      return NotASocket;

   NET_Address *target = addressToNet( address );
   if ( !target )
      return UnknownError;

   const bool ok = NET_SendDatagram( entry->datagram, target, address->port, buffer, bufferSize );
   NET_UnrefAddress( target );

   return ok ? NoError : UnknownError;
}


//-----------------------------------------------------------------------------
// Reliable (TCP)
//-----------------------------------------------------------------------------

NetSocket Net::openListenPort( U16 port )
{
   NET_Server *server = NET_CreateServer( NULL, port, 0 );
   if ( !server )
   {
      Con::errorf( "Net::openListenPort - could not listen on %d: %s", port, SDL_GetError() );
      smConnectionNotify.trigger( InvalidSocket, Net::ConnectFailed );
      return InvalidSocket;
   }

   const NetSocket socket = allocEntry( KindServer );
   gEntries[ socket ].server = server;
   gEntries[ socket ].port = port;

   return socket;
}

NetSocket Net::openConnectTo( const char *addressString )
{
   if ( dStrnicmp( addressString, "ipx:", 4 ) == 0 )
      return InvalidSocket;

   if ( dStrnicmp( addressString, "ip:", 3 ) == 0 )
      addressString += 3;

   char remoteAddr[ 256 ];
   dStrncpy( remoteAddr, addressString, sizeof( remoteAddr ) - 1 );
   remoteAddr[ sizeof( remoteAddr ) - 1 ] = 0;

   U16 port = 0;
   if ( char *colon = dStrchr( remoteAddr, ':' ) )
   {
      *colon = 0;
      port = U16( dAtoi( colon + 1 ) );
   }

   if ( dStricmp( remoteAddr, "broadcast" ) == 0 )
      return InvalidSocket;

   // Resolution is asynchronous; process() picks it up and connects. The old
   // implementation had a thread of its own for this.
   NET_Address *pending = NET_ResolveHostname( remoteAddr );
   if ( !pending )
   {
      Con::errorf( "Net::openConnectTo - could not resolve %s: %s", remoteAddr, SDL_GetError() );
      return InvalidSocket;
   }

   const NetSocket socket = allocEntry( KindStream );
   gEntries[ socket ].pending = pending;
   gEntries[ socket ].port = port;
   gEntries[ socket ].state = StateResolving;

   return socket;
}

void Net::closeConnectTo( NetSocket socket )
{
   if ( Entry *entry = entryFor( socket ) )
      releaseEntry( entry );
}

Net::Error Net::sendtoSocket( NetSocket socket, const U8 *buffer, S32 bufferSize )
{
   return send( socket, buffer, bufferSize );
}


//-----------------------------------------------------------------------------
// Socket level
//-----------------------------------------------------------------------------

NetSocket Net::openSocket()
{
   return allocEntry( KindPending );
}

Net::Error Net::closeSocket( NetSocket socket )
{
   Entry *entry = entryFor( socket );
   if ( !entry )
      return NotASocket;

   releaseEntry( entry );
   return NoError;
}

Net::Error Net::connect( NetSocket socket, const NetAddress *address )
{
   Entry *entry = entryFor( socket );
   if ( !entry )
      return NotASocket;

   NET_Address *target = addressToNet( address );
   if ( !target )
      return UnknownError;

   entry->stream = NET_CreateClient( target, address->port, 0 );
   NET_UnrefAddress( target );

   if ( !entry->stream )
      return UnknownError;

   entry->kind = KindStream;
   entry->state = StateConnecting;

   return NoError;
}

Net::Error Net::bind( NetSocket socket, U16 port )
{
   Entry *entry = entryFor( socket );
   if ( !entry )
      return NotASocket;

   // Remembered until listen() decides what kind of socket this is.
   entry->port = port;
   return NoError;
}

Net::Error Net::listen( NetSocket socket, S32 backlog )
{
   TORQUE_UNUSED( backlog ); // SDL_net sizes its own accept queue

   Entry *entry = entryFor( socket );
   if ( !entry )
      return NotASocket;

   entry->server = NET_CreateServer( NULL, entry->port, 0 );
   if ( !entry->server )
   {
      Con::errorf( "Net::listen - could not listen on %d: %s", entry->port, SDL_GetError() );
      return UnknownError;
   }

   entry->kind = KindServer;
   return NoError;
}

NetSocket Net::accept( NetSocket acceptSocket, NetAddress *remoteAddress )
{
   Entry *entry = entryFor( acceptSocket );
   if ( !entry || !entry->server )
      return InvalidSocket;

   NET_StreamSocket *client = NULL;
   if ( !NET_AcceptClient( entry->server, &client ) || !client )
      return InvalidSocket;

   const NetSocket socket = allocEntry( KindStream );
   gEntries[ socket ].stream = client;
   gEntries[ socket ].state = StateConnected;

   if ( remoteAddress )
      addressFromNet( NET_GetStreamSocketAddress( client ), 0, remoteAddress );

   return socket;
}

Net::Error Net::send( NetSocket socket, const U8 *buffer, S32 bufferSize )
{
   Entry *entry = entryFor( socket );
   if ( !entry || !entry->stream )
      return NotASocket;

   return NET_WriteToStreamSocket( entry->stream, buffer, bufferSize ) ? NoError : UnknownError;
}

Net::Error Net::recv( NetSocket socket, U8 *buffer, S32 bufferSize, S32 *bytesRead )
{
   Entry *entry = entryFor( socket );
   if ( !entry || !entry->stream )
      return NotASocket;

   const int read = NET_ReadFromStreamSocket( entry->stream, buffer, bufferSize );
   if ( read < 0 )
      return UnknownError;

   *bytesRead = read == 0 ? -1 : read;
   return read == 0 ? WouldBlock : NoError;
}

/// SDL_net sockets are always non-blocking, which is what every caller here
/// asks for anyway.
Net::Error Net::setBlocking( NetSocket socket, bool blockingIO )
{
   Entry *entry = entryFor( socket );
   if ( !entry )
      return NotASocket;

   return blockingIO ? UnknownError : NoError;
}

/// Neither of these has an equivalent in SDL_net. Nothing in the engine calls
/// them; they stay because platformNet.h declares them.
Net::Error Net::setBufferSize( NetSocket socket, S32 bufferSize )
{
   TORQUE_UNUSED( bufferSize );
   return entryFor( socket ) ? NoError : NotASocket;
}

Net::Error Net::setBroadcast( NetSocket socket, bool broadcast )
{
   TORQUE_UNUSED( broadcast );
   return entryFor( socket ) ? NoError : NotASocket;
}


//-----------------------------------------------------------------------------
// Addresses
//-----------------------------------------------------------------------------

bool Net::compareAddresses( const NetAddress *a1, const NetAddress *a2 )
{
   return a1->type == a2->type
       && a1->port == a2->port
       && dMemcmp( a1->netNum, a2->netNum, sizeof( a1->netNum ) ) == 0;
}

bool Net::stringToAddress( const char *addressString, NetAddress *address )
{
   if ( dStrnicmp( addressString, "ipx:", 4 ) == 0 )
      return false;

   if ( dStrnicmp( addressString, "ip:", 3 ) == 0 )
      addressString += 3;

   char host[ 256 ];
   dStrncpy( host, addressString, sizeof( host ) - 1 );
   host[ sizeof( host ) - 1 ] = 0;

   U16 port = U16( sDefaultPort );
   if ( char *colon = dStrchr( host, ':' ) )
   {
      *colon = 0;
      port = U16( dAtoi( colon + 1 ) );
   }

   if ( dStricmp( host, "broadcast" ) == 0 )
   {
      address->type = NetAddress::IPAddress;
      dMemset( address->netNum, 0xFF, sizeof( address->netNum ) );
      address->port = port;
      return true;
   }

   NET_Address *resolved = NET_ResolveHostname( host );
   if ( !resolved )
      return false;

   // Callers treat this as synchronous, so a name costs a wait here. Only
   // openConnectTo goes through the asynchronous path.
   const bool ok = NET_WaitUntilResolved( resolved, -1 ) == NET_SUCCESS;
   if ( ok )
      addressFromNet( resolved, port, address );

   NET_UnrefAddress( resolved );
   return ok;
}

void Net::addressToString( const NetAddress *address, char addressString[256] )
{
   dSprintf( addressString, 256, "IP:%d.%d.%d.%d:%d",
             address->netNum[0], address->netNum[1],
             address->netNum[2], address->netNum[3],
             address->port );
}


//-----------------------------------------------------------------------------
// Pump
//-----------------------------------------------------------------------------

void Net::process()
{
   // Incoming datagrams.
   if ( Entry *udp = entryFor( gUdpSocket ) )
   {
      NET_Datagram *dgram = NULL;
      while ( NET_ReceiveDatagram( udp->datagram, &dgram ) && dgram )
      {
         NetAddress srcAddress;
         addressFromNet( dgram->addr, dgram->port, &srcAddress );

         // Ignore what we sent to ourselves.
         const bool selfPacket = srcAddress.netNum[0] == 127 && srcAddress.netNum[1] == 0
                              && srcAddress.netNum[2] == 0   && srcAddress.netNum[3] == 1
                              && srcAddress.port == gUdpPort;

         if ( !selfPacket && dgram->buflen > 0 )
         {
            RawData data;
            data.data = reinterpret_cast< S8* >( dgram->buf );
            data.size = dgram->buflen;
            data.ownMemory = false;

            smPacketReceive.trigger( srcAddress, data );
         }

         NET_DestroyDatagram( dgram );
         dgram = NULL;
      }
   }

   // Connections being resolved, connected, or carrying data.
   for ( U32 i = 0; i < gEntries.size(); ++i )
   {
      Entry *entry = &gEntries[i];
      const NetSocket socket = NetSocket( i );

      if ( entry->kind == KindServer )
      {
         NET_StreamSocket *client = NULL;
         while ( NET_AcceptClient( entry->server, &client ) && client )
         {
            const NetSocket accepted = allocEntry( KindStream );
            gEntries[ accepted ].stream = client;
            gEntries[ accepted ].state = StateConnected;

            NetAddress remote;
            addressFromNet( NET_GetStreamSocketAddress( client ), 0, &remote );

            smConnectionAccept.trigger( socket, accepted, remote );

            client = NULL;
            entry = &gEntries[i]; // allocEntry may have grown the vector
         }
         continue;
      }

      if ( entry->kind != KindStream )
         continue;

      if ( entry->state == StateResolving )
      {
         const NET_Status status = NET_GetAddressStatus( entry->pending );
         if ( status == NET_WAITING )
            continue;

         if ( status == NET_FAILURE )
         {
            smConnectionNotify.trigger( socket, Net::DNSFailed );
            releaseEntry( entry );
            continue;
         }

         smConnectionNotify.trigger( socket, Net::DNSResolved );

         entry->stream = NET_CreateClient( entry->pending, entry->port, 0 );
         NET_UnrefAddress( entry->pending );
         entry->pending = NULL;

         if ( !entry->stream )
         {
            smConnectionNotify.trigger( socket, Net::ConnectFailed );
            releaseEntry( entry );
            continue;
         }

         entry->state = StateConnecting;
      }

      if ( entry->state == StateConnecting )
      {
         const NET_Status status = NET_GetConnectionStatus( entry->stream );
         if ( status == NET_WAITING )
            continue;

         if ( status == NET_FAILURE )
         {
            smConnectionNotify.trigger( socket, Net::ConnectFailed );
            releaseEntry( entry );
            continue;
         }

         entry->state = StateConnected;
         smConnectionNotify.trigger( socket, Net::Connected );
      }

      if ( entry->state != StateConnected )
         continue;

      U8 buffer[ MaxPacketDataSize ];
      const int read = NET_ReadFromStreamSocket( entry->stream, buffer, sizeof( buffer ) );

      if ( read > 0 )
      {
         RawData data;
         data.data = reinterpret_cast< S8* >( buffer );
         data.size = read;
         data.ownMemory = false;

         smConnectionReceive.trigger( socket, data );
      }
      else if ( read < 0 || NET_GetConnectionStatus( entry->stream ) == NET_FAILURE )
      {
         smConnectionNotify.trigger( socket, Net::Disconnected );
         releaseEntry( entry );
      }
   }
}
