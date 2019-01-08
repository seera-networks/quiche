// Copyright (C) 2018, Cloudflare, Inc.
// Copyright (C) 2018, Alessandro Ghedini
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#if defined(__cplusplus)
extern "C" {
#endif


#define QUICHE_VERSION_DRAFT17 0xff000011

#define QUICHE_MAX_CONN_ID_LEN 18

enum quiche_error {
    // An asynchronous operation (e.g. certificate lookup) is pending.
    QUICHE_ERR_PENDING = -1,

    // There is no more work to do.
    QUICHE_ERR_DONE = -2,

    // The provided buffer is too short.
    QUICHE_ERR_BUFFER_TOO_SHORT = -3,

    // The provided packet cannot be parsed because its version is unknown.
    QUICHE_ERR_UNKNOWN_VERSION = -4,

    // The provided packet cannot be parsed because it contains an invalid
    // frame.
    QUICHE_ERR_INVALID_FRAME = -5,

    // The provided packet cannot be parsed.
    QUICHE_ERR_INVALID_PACKET = -6,

    // The operation cannot be completed because the connection is in an
    // invalid state.
    QUICHE_ERR_INVALID_STATE = -7,

    // The operation cannot be completed because the stream is in an
    // invalid state.
    QUICHE_ERR_INVALID_STREAM_STATE = -8,

    // The peer's transport params cannot be parsed.
    QUICHE_ERR_INVALID_TRANSPORT_PARAM = -9,

    // A cryptographic operation failed.
    QUICHE_ERR_CRYPTO_FAIL = -10,

    // The TLS handshake failed.
    QUICHE_ERR_TLS_FAIL = -11,

    // The peer violated the local flow control limits.
    QUICHE_ERR_FLOW_CONTROL = -12,
};

// Enables logging. |cb| will be called with log messages
void quiche_enable_debug_logging(void (*cb)(const char *line, void *argp),
                                 void *argp);

// Stores configuration shared between multiple connections.
typedef struct Config quiche_config;

// Creates a config object with the given version.
quiche_config *quiche_config_new(uint32_t version);

// Configures the given certificate chain.
int quiche_config_load_cert_chain_from_pem_file(quiche_config *config,
                                                const char *path);

// Configures the given private key.
int quiche_config_load_priv_key_from_pem_file(quiche_config *config,
                                              const char *path);

// Enables logging of secrets.
void quiche_config_log_keys(quiche_config *config);

// Sets the `idle_timeout` transport parameter.
void quiche_config_set_idle_timeout(quiche_config *config, uint64_t v);

// Sets the `initial_max_data` transport parameter.
void quiche_config_set_initial_max_data(quiche_config *config, uint64_t v);

// Sets the `initial_max_stream_data_bidi_local` transport parameter.
void quiche_config_set_initial_max_stream_data_bidi_local(quiche_config *config, uint64_t v);

// Sets the `initial_max_stream_data_bidi_remote` transport parameter.
void quiche_config_set_initial_max_stream_data_bidi_remote(quiche_config *config, uint64_t v);

// Sets the `initial_max_streams_bidi` transport parameter.
void quiche_config_set_initial_max_streams_bidi(quiche_config *config, uint16_t v);

// Frees the config object.
void quiche_config_free(quiche_config *config);

// Extracts version, type and destination connection ID from the packet
// in |buf|.
int quiche_header_info(const uint8_t *buf, size_t buf_len, size_t dcil,
                       uint32_t *version, uint8_t *type, uint8_t dcid[],
                       size_t *dcid_len);

// A QUIC connection.
typedef struct Connection quiche_conn;

// Creates a new server-side connection.
quiche_conn *quiche_accept(const uint8_t *scid, size_t scid_len,
                           quiche_config *config);

// Creates a new client-side connection.
quiche_conn *quiche_connect(const char *server_name, const uint8_t *scid,
                            size_t scid_len, quiche_config *config);

// Processes QUIC packets received from the peer.
ssize_t quiche_conn_recv(quiche_conn *conn, uint8_t *buf, size_t buf_len);

// Writes a single QUIC packet to be sent to the peer.
ssize_t quiche_conn_send(quiche_conn *conn, uint8_t *out, size_t out_len);

// Buffer holding data at a specific offset.
typedef struct RangeBuf quiche_rangebuf;

// Reads contiguous data from a stream.
quiche_rangebuf *quiche_conn_stream_recv(quiche_conn *conn, uint64_t stream_id);

// Writes data to a stream.
ssize_t quiche_conn_stream_send(quiche_conn *conn, uint64_t stream_id,
                                const uint8_t *buf, size_t buf_len, bool fin);

// Returns the data of the buffer.
const uint8_t *quiche_rangebuf_data(quiche_rangebuf *b);

// Returns the length of the buffer.
size_t quiche_rangebuf_len(quiche_rangebuf *b);

// Returns whether `self` holds the final offset in the stream.
bool quiche_rangebuf_fin(quiche_rangebuf *b);

// Frees the buffer object.
void quiche_rangebuf_free(quiche_rangebuf *b);

// An iterator over the streams that have outstanding data to read.
typedef struct Readable quiche_readable;

// Creates an iterator of streams that have outstanding data to read.
quiche_readable *quiche_conn_readable(quiche_conn *conn);

// Fetches the next element from the stream iterator. Returns false if the
// iterator is empty.
bool quiche_readable_next(quiche_readable *iter, uint64_t *stream_id);

// Frees the readable object.
void quiche_readable_free(quiche_readable *r);

// Returns the amount of time until the next timeout event, as nanoseconds.
uint64_t quiche_conn_timeout_as_nanos(quiche_conn *conn);

// Processes a timeout event.
void quiche_conn_on_timeout(quiche_conn *conn);

// Closes the connection with the given error and reason.
int quiche_conn_close(quiche_conn *conn, bool app, uint16_t err,
                      const uint8_t *reason, size_t reason_len);

// Returns true if the connection handshake is complete.
bool quiche_conn_is_established(quiche_conn *conn);

// Returns true if the connection is closed.
bool quiche_conn_is_closed(quiche_conn *conn);

// Frees the connection object.
void quiche_conn_free(quiche_conn *conn);


#if defined(__cplusplus)
}  // extern C
#endif
