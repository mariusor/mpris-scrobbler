/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SCROBBLER_H
#define MPRIS_SCROBBLER_SCROBBLER_H

#include <assert.h>
#include <curl/curl.h>

void scrobbler_connection_free (struct scrobbler_connection *conn)
{
    if (NULL == conn) { return; }
    _trace("scrobbler::connection_free[%zd:%p]: %s", conn->idx, conn, get_api_type_label(conn->credentials->end_point));
    if (NULL != conn->headers) {
        int headers_count = arrlen(conn->headers);
        for (int i = headers_count - 1; i >= 0; i--) {
            _trace2("scrobbler::connection_free[%zd]::curl_headers(%zd::%zd:%p)", conn->idx, i, headers_count, conn->headers[i]);
            curl_slist_free_all(conn->headers[i]);
            (void)arrpop(conn->headers);
            conn->headers[i] = NULL;
        }
        assert(arrlen(conn->headers) == 0);
        arrfree(conn->headers);
        conn->headers = NULL;
    }
    if (NULL != conn->ev) {
        _trace2("scrobbler::connection_free[%zd]::conn:(%p)", conn->idx, conn->ev);
        event_free(conn->ev);
        conn->ev = NULL;
    }
    if (NULL != conn->request) {
        _trace2("scrobbler::connection_free[%zd]::request(%p)", conn->idx, conn->request);
        http_request_free(conn->request);
        conn->request = NULL;
    }
    if (NULL != conn->response) {
        _trace2("scrobbler::connection_free[%zd]::response(%p)", conn->idx, conn->response);
        http_response_free(conn->response);
        conn->response = NULL;
    }
    if (NULL != conn->handle) {
        _trace2("scrobbler::connection_free[%zd]::curl_easy_handle(%p): parent: %p", conn->idx, conn->handle, conn->parent->handle);
        curl_multi_remove_handle(conn->parent->handle, conn->handle);
        curl_easy_cleanup(conn->handle);
        conn->handle = NULL;
    }
    free(conn);
}

struct scrobbler_connection *scrobbler_connection_new(void)
{
    struct scrobbler_connection *s = calloc(1, sizeof(struct scrobbler_connection));
    s->idx = -1;
    return (s);
}

#if DEBUG
static void dump(const char *text, FILE *stream, unsigned char *ptr, size_t size, char nohex)
{
  size_t i;
  size_t c;

  unsigned int width = 0x10;

  if(nohex)
    /* without the hex output, we can fit more on screen */
    width = 0x40;

  fprintf(stream, "%s, %10.10lu bytes (0x%8.8lx)\n",
          text, (unsigned long)size, (unsigned long)size);

  for(i = 0; i<size; i += width) {

    fprintf(stream, "%4.4lx: ", (unsigned long)i);

    if(!nohex) {
      /* hex not disabled, show it */
      for(c = 0; c < width; c++)
        if(i + c < size)
          fprintf(stream, "%02x ", ptr[i + c]);
        else
          fputs("   ", stream);
    }

    for(c = 0; (c < width) && (i + c < size); c++) {
      /* check for 0D0A; if found, skip past and start a new line of output */
      if(nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D &&
         ptr[i + c + 1] == 0x0A) {
        i += (c + 2 - width);
        break;
      }
      fprintf(stream, "%c",
              (ptr[i + c] >= 0x20) && (ptr[i + c]<0x80)?ptr[i + c]:'.');
      /* check again for 0D0A, to avoid an extra \n if it's at width */
      if(nohex && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D &&
         ptr[i + c + 2] == 0x0A) {
        i += (c + 3 - width);
        break;
      }
    }
    fputc('\n', stream); /* newline */
  }
  fflush(stream);
}

struct data {
    char trace_ascii;
};

static int my_trace(CURL *handle, curl_infotype type, char *data, size_t size, void *userp)
{
    struct data *config = (struct data *)userp;
    const char *text;
    (void)handle; /* prevent compiler warning */

    switch(type) {
    case CURLINFO_TEXT:
        fprintf(stderr, "== Info: %s", data);
        /* FALLTHROUGH */
    default: /* in case a new one is introduced to shock us */
        return 0;

    case CURLINFO_HEADER_OUT:
        text = "=> Send header";
        break;
    case CURLINFO_DATA_OUT:
        text = "=> Send data";
        break;
    case CURLINFO_SSL_DATA_OUT:
        text = "=> Send SSL data";
        break;
    case CURLINFO_HEADER_IN:
        text = "<= Recv header";
        break;
    case CURLINFO_DATA_IN:
        text = "<= Recv data";
        break;
    case CURLINFO_SSL_DATA_IN:
        text = "<= Recv SSL data";
        break;
    }

    dump(text, stderr, (unsigned char *)data, size, config->trace_ascii);
    return 0;
}
#endif

static void event_cb(int, short, void *);
void scrobbler_connection_init(struct scrobbler_connection *connection, struct scrobbler *s, struct api_credentials *credentials, int idx)
{
    assert (s);

    connection->handle = curl_easy_init();
    connection->response = http_response_new();
    connection->credentials = credentials;
    connection->idx = idx;
    connection->error[0] = '\0';
    connection->parent = s;
    _trace("scrobbler::connection_init[%s:%d:%p]:curl_easy_handle(%p)", get_api_type_label(credentials->end_point), idx, connection, connection->handle);

#if DEBUG
    struct data config = {.trace_ascii = 1};

    curl_easy_setopt(connection->handle, CURLOPT_DEBUGFUNCTION, my_trace);
    curl_easy_setopt(connection->handle, CURLOPT_DEBUGDATA, &config);
    /* the DEBUGFUNCTION has no effect until we enable VERBOSE */
    curl_easy_setopt(connection->handle, CURLOPT_VERBOSE, 1L);
#endif
    curl_easy_setopt(connection->handle, CURLOPT_FOLLOWLOCATION, 1L);
}

static void scrobbler_clean(struct scrobbler *s)
{
    if (NULL == s) { return; }

    _trace("scrobbler::clean[%p]", s);

    int conn_count = arrlen(s->connections);
    if (conn_count > 0) {
        for (int i = conn_count - 1; i >= 0;  i--) {
            scrobbler_connection_free(s->connections[i]);
            s->connections[i] = NULL;
            (void)arrpop(s->connections);
        }
    }
    assert(arrlen(s->connections) == 0);
    arrfree(s->connections);
    s->connections = NULL;

#if 1
    if(evtimer_pending(s->timer_event, NULL)) {
        _trace2("curl::multi_timer_remove(%p)", s->timer_event);
        evtimer_del(s->timer_event);
    }
#endif
}

/*
 * Based on https://curl.se/libcurl/c/hiperfifo.html
 * Check for completed transfers, and remove their easy handles
 */
static void check_multi_info(struct scrobbler *s)
{
    char *eff_url;
    int msgs_left;
    struct scrobbler_connection *conn;
    CURL *easy;
    CURLMsg *msg;

    while((msg = curl_multi_info_read(s->handle, &msgs_left))) {
        easy = msg->easy_handle;
        curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
        curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
        curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &conn->response->code);
        _trace("curl::checking[%zd]: %p", conn->idx, conn);
        if(msg->msg == CURLMSG_DONE && conn->action == CURL_POLL_REMOVE) {
            if (strlen(conn->error) != 0) {
                _warn("curl::transfer::done[%zd]: %s => %s", conn->idx, eff_url, conn->error);
            } else {
                _trace("curl::transfer::done[%zd]: %s", conn->idx, eff_url);
            }

            http_response_print(conn->response, log_tracing2);

            _info(" api::submitted_to[%s:%d]: %s", get_api_type_label(conn->credentials->end_point), conn->idx, ((conn->response->code == 200) ? "ok" : "nok"));
        }
        scrobbler_connection_free(conn);
    }
    //if(s->still_running == 0) event_base_loopbreak(s->evbase);
}


/*
 * Based on https://curl.se/libcurl/c/hiperfifo.html
 * Assign information to a scrobbler_connection structure
 */
static void setsock(struct scrobbler_connection *conn, curl_socket_t sock, CURL *e, int act, struct scrobbler *s)
{
    int kind = ((act & CURL_POLL_IN) ? EV_READ : 0) | ((act & CURL_POLL_OUT) ? EV_WRITE : 0) | EV_PERSIST;

    if (conn->handle != e) {
        _error("curl::mismatched_handle %p %p kind %zd", conn->handle, e, kind);
        return;
    }

    conn->sockfd = sock;
    conn->action = act;

    if (NULL != conn->ev) {
        event_free(conn->ev);
        conn->ev = NULL;
    }
    evutil_make_socket_nonblocking(conn->sockfd);
    conn->ev = event_new(s->evbase, conn->sockfd, kind, event_cb, s);
    event_add(conn->ev, NULL);
}

static struct scrobbler_connection *scrobbler_connection_get(struct scrobbler *s, CURL *e, int *idx)
{
    struct scrobbler_connection *conn = NULL;
    int conn_count = arrlen(s->connections);
    for (int i = conn_count - 1; i >= 0; i--) {
        conn = s->connections[i];
        if (NULL == conn) {
            _warn("curl::invalid_connection_handle:idx[%d] total[%d]", i, conn_count);
            continue;
        }
        _trace2("curl::searching_easy_handle:idx[%d] e[%p:%p]", i, e, conn->handle);
        if (conn->handle == e) {
            *idx = conn->idx;
            _trace2("curl::found_easy_handle:idx[%d] e[%p:%p]", i, e, conn->handle);
            break;
        }
    }
    return conn;
}

const char *whatstr[]={ "none", "IN", "OUT", "INOUT", "REMOVE" };
static void dispatch(int fd, short ev, void *data);
/* CURLMOPT_SOCKETFUNCTION */
static int curl_request_has_data(CURL *e, curl_socket_t sock, int what, void *data, void *conn_data)
{
    if (NULL == data) { return 0; }

    struct scrobbler *s = data;
    struct scrobbler_connection *conn = conn_data;
    _trace("curl::data_callback[%p:%zd]: s: %p, conn: %p", e, sock, data, conn_data);

    int events = 0;

    int idx = -1;
    if (NULL == conn) {
        conn = scrobbler_connection_get(s, e, &idx);
        _trace("curl::data_callback_found_connection[%zd:%p]: conn: %p", idx, e, conn);
    }

    switch(what) {
    case CURL_POLL_IN:
    case CURL_POLL_OUT:
    case CURL_POLL_INOUT:
        // set libevent context for a new connection
        if(what != CURL_POLL_IN) events |= EV_WRITE;
        if(what != CURL_POLL_OUT) events |= EV_READ;

        events |= EV_PERSIST;

        setsock(conn, sock, e, what, s);
        if (conn->action != what) {
            _trace("curl::data_callback[%zd:%p]: s=%d, action=%s->%s", idx, e, sock, whatstr[conn->action], whatstr[what]);
        } else {
            _trace2("curl::data_callback[%zd:%p]: s=%d, action=%s", idx, e, sock, whatstr[what]);
        }
        return CURLM_OK;
      break;
    case CURL_POLL_REMOVE:
        if (what == CURL_POLL_REMOVE && sock) {
            _trace2("curl::data_remove[%zd:%p]: action=%s", idx, e, whatstr[what]);
            scrobbler_connection_free(conn);
            curl_multi_assign(s->handle, conn->sockfd, NULL);
        }
        break;
    default:
        _trace2("curl::unknown_socket_action[%zd:%p]: action=%s", idx, e, whatstr[what]);
        abort();
    }
    return CURLM_CALL_MULTI_PERFORM;
}

/*
 * Based on https://curl.se/libcurl/c/hiperfifo.html
 * Called by libevent when our timeout expires
 */
static void timer_cb(int fd, short kind, void *data)
{
    assert(data);
    if (fd == -1) {
        int errcode = evutil_socket_geterror(fd);
        if (errcode != 0) {
            _warn("curl::multi_socket_activation:socket_error: %s", evutil_socket_error_to_string(errcode));
        }
        return;
    }
    struct scrobbler *s = data;
    CURLMcode rc = curl_multi_socket_action(s->handle, CURL_SOCKET_TIMEOUT, 0, &s->still_running);
    if (rc != CURLM_OK) {
        _warn("curl::multi_socket_activation:error: %s", curl_multi_strerror(rc));
    }
    check_multi_info(s);
    _trace2("curl::multi_socket_activation[%p:%d]: still_running: %d", s, fd, s->still_running);
}

/* Update the event timer after curl_multi library calls */
static int curl_request_wait_timeout(CURLM *multi, long timeout_ms, struct scrobbler *s)
{
    assert(multi);
    assert(s);

    double timeout_sec = (double)timeout_ms / (double)1000;
    struct timeval timeout = {
        .tv_sec = (int)timeout_sec,
        .tv_usec = (timeout_ms - timeout_sec * 1000) * 1000,
    };

    if (s->timer_event == NULL) {
        s->timer_event = evtimer_new(s->evbase, timer_cb, s);
        evtimer_add(s->timer_event, &timeout);
        _trace2("curl::multi_timer_add(%p:%p): timeout: %d", multi, s->timer_event, timeout_ms);
        return 0;
    }
    /**
     * if timeout_ms is  0, call curl_multi_socket_action() at once!
     * if timeout_ms is -1, just delete the timer
     *
     * for all other values of timeout_ms, this should set or *update*
     * the timer to the new value
     */
    _trace2("curl::multi_timer_triggered(%p:%p):still_running: %d, timeout: %d", s, s->timer_event, s->still_running, timeout_ms);
    if(timeout_ms == 0) {
        CURLMcode rc = curl_multi_socket_action(multi, CURL_SOCKET_TIMEOUT, 0, &s->still_running);
        if (rc != CURLM_OK) {
            _warn("curl::multi_timer_activation:failed: %s", curl_multi_strerror(rc));
        }
        return 0;
    }
    if (timeout_ms == -1) {
        evtimer_del(s->timer_event);
        event_free(s->timer_event);
        s->timer_event = NULL;
    }

    _trace2("curl::multi_timer_update(%p:%p): %d", multi, s->timer_event, timeout_ms);
    evtimer_assign(s->timer_event, s->evbase, timer_cb, &timeout);

    return 0;
}

struct scrobbler *scrobbler_new(void)
{
    struct scrobbler *result = calloc(1, sizeof(struct scrobbler));

    return (result);
}

void scrobbler_init(struct scrobbler *s, struct configuration *config, struct event_base *evbase)
{
    s->credentials = config->credentials;
    s->handle = curl_multi_init();

    /* setup the generic multi interface options we want */
    curl_multi_setopt(s->handle, CURLMOPT_SOCKETFUNCTION, curl_request_has_data);
    curl_multi_setopt(s->handle, CURLMOPT_SOCKETDATA, s);
    curl_multi_setopt(s->handle, CURLMOPT_TIMERFUNCTION, curl_request_wait_timeout);
    curl_multi_setopt(s->handle, CURLMOPT_TIMERDATA, s);

    curl_multi_setopt(s->handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, 1L);

    s->connections = NULL;
    s->evbase = evbase;
}

void scrobbler_free(struct scrobbler *s)
{
    if (NULL == s) { return; }

    scrobbler_clean(s);

    if (NULL != s->handle) {
        _trace2("mem::free::scrobbler::curl_multi(%p)", s->handle);
        curl_multi_cleanup(s->handle);
    }
    if (NULL != s->timer_event) {
        event_free(s->timer_event);
        s->timer_event = NULL;
    }

    free(s);
}

/* Called by libevent when we get action on a multi socket */
static void event_cb(int fd, short kind, void *data)
{
    struct scrobbler *s = (struct scrobbler*)data;
    if (fd == -1) {
        int errcode = evutil_socket_geterror(fd);
        if (errcode != 0) {
            _trace2("curl::transfer:socket_error: fd=%d, ev=%d %s",fd, kind, evutil_socket_error_to_string(errcode));
        }
    }

    int action = ((kind & EV_READ) ? CURL_CSELECT_IN : 0) | ((kind & EV_WRITE) ? CURL_CSELECT_OUT : 0);
    _trace2("curl::event_cb(%p:%p:%zd:%zd): still running: %d", s, s->handle, fd, action, s->still_running);

    assert(s->connections);
    assert(arrlen(s->connections) != 0);

    CURLMcode rc = curl_multi_socket_action(s->handle, fd, action, &s->still_running);
    if (rc != CURLM_OK) {
        _warn("curl::transfer::error: %s", curl_multi_strerror(rc));
    }

    check_multi_info(s);
    if(s->still_running <= 0) {
        _trace2("curl::transfers::finished_all");
        scrobbler_clean(s);
    }
}

typedef struct http_request*(*request_builder_t)(const struct scrobble*[], const int, const struct api_credentials*, CURL*);

void api_request_do(struct scrobbler *s, const struct scrobble *tracks[], const int track_count, request_builder_t build_request)
{
    if (NULL == s) { return; }
    if (NULL == s->credentials) { return; }

    int credentials_count = arrlen(s->credentials);
    int enabled_credentials_index = 0;

    for (int i = 0; i < credentials_count; i++) {
        struct api_credentials *cur = s->credentials[i];
        if (!credentials_valid(cur) ) {
            if (cur->enabled) { _warn("scrobbler::invalid_service[%s]", get_api_type_label(cur->end_point)); }
            continue;
        }

        struct scrobbler_connection *conn = scrobbler_connection_new();
        scrobbler_connection_init(conn, s, cur, enabled_credentials_index);
        conn->request = build_request(tracks, track_count, cur, conn->handle);

        arrput(s->connections, conn);

        build_curl_request(conn);

        curl_easy_setopt(conn->handle, CURLOPT_PRIVATE, conn);
        curl_easy_setopt(conn->handle, CURLOPT_ERRORBUFFER, conn->error);
        curl_easy_setopt(conn->handle, CURLOPT_TIMEOUT_MS, 2000L);
        curl_easy_setopt(conn->handle, CURLOPT_MAXCONNECTS, 1L);
        curl_easy_setopt(conn->handle, CURLOPT_FRESH_CONNECT, 1L);
        curl_easy_setopt(conn->handle, CURLOPT_FORBID_REUSE, 1L);

        curl_multi_add_handle(s->handle, conn->handle);
        enabled_credentials_index++;
    }
}

#endif // MPRIS_SCROBBLER_SCROBBLER_H
