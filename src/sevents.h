/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SEVENTS_H
#define MPRIS_SCROBBLER_SEVENTS_H

#include <pthread.h>
#include <event2/thread.h>

static void send_now_playing(evutil_socket_t, short, void *);
void events_free(const struct events *ev)
{
    if (NULL == ev) { return; }
    _trace2("mem::free::event(%p):SIGINT", ev->sigint);
    event_free(ev->sigint);
    _trace2("mem::free::event(%p):SIGTERM", ev->sigterm);
    event_free(ev->sigterm);
    _trace2("mem::free::event(%p):SIGHUP", ev->sighup);
    event_free(ev->sighup);
    _trace2("mem::free::event_base(%p)", ev->base);
    event_base_free(ev->base);
}

struct events *events_new(void)
{
    struct events *result = calloc(1, sizeof(struct events));
    return result;
}

static void log_event(const int severity, const char *msg)
{
    enum log_levels level = log_tracing2;
    switch (severity) {
        case EVENT_LOG_DEBUG:
            level = log_tracing2;
            break;
        case EVENT_LOG_WARN:
            level = log_warning;
            break;
        case EVENT_LOG_ERR:
            level = log_error;
            break;
        case EVENT_LOG_MSG:
        default:
            level = log_tracing;
    }
    _log(level, "libevent: %s", msg);
}

void events_init(struct events *ev, struct state *s)
{
    if (NULL == ev) { return; }

#if defined(LIBEVENT_DEBUG) && LIBEVENT_DEBUG
    event_enable_debug_mode();
    event_enable_debug_logging(EVENT_DBG_ALL);
    event_set_log_callback(log_event);
#endif
#if 1
    // as curl uses different threads, it's better to initialize support
    // for it in libevent2
    const int maybe_threads = evthread_use_pthreads();
    if (maybe_threads < 0) {
        _error("events::unable_to_setup_multithreading");
    }
#endif


    ev->base = event_base_new();
    if (NULL == ev->base) {
        _error("mem::init_libevent: failure");
        return;
    }

    _trace2("mem::inited_libevent(%p)", ev->base);
    ev->sigint = evsignal_new(ev->base, SIGINT, sighandler, s);
    if (NULL == ev->sigint || event_add(ev->sigint, NULL) < 0) {
        _error("mem::add_event(SIGINT): failed");
        return;
    }
    ev->sigterm = evsignal_new(ev->base, SIGTERM, sighandler, s);
    if (NULL == ev->sigterm || event_add(ev->sigterm, NULL) < 0) {
        _error("mem::add_event(SIGTERM): failed");
        return;
    }
    ev->sighup = evsignal_new(ev->base, SIGHUP, sighandler, s);
    if (NULL == ev->sighup || event_add(ev->sighup, NULL) < 0) {
        _error("mem::add_event(SIGHUP): failed");
        return;
    }
}

static void send_now_playing(evutil_socket_t fd, short event, void *data)
{
    assert(data);
    struct event_payload *state = data;

    struct scrobble *track = &state->scrobble;
    assert(track);
    if (scrobble_is_empty(track)) {
        _debug("events::now_playing: invalid scrobble %p", track);
        return;
    }

    if (track->position > (double)track->length) {
        _trace2("events::now_playing: track position out of bounds %d > %ld", track->position, (double)track->length);
        event_del(&state->event);
        return;
    }

    struct mpris_player *player = state->parent;
    assert(player);
    if (!mpris_player_is_valid(player)) {
        _debug("events::now_playing: invalid player %s", player->mpris_name);
        event_del(&state->event);
        return;
    }

    struct scrobbler *scrobbler = player->scrobbler;
    assert(scrobbler);

    _trace("events::triggered(%p:%p):now_playing", state, track);
    print_scrobble(track, log_debug);

    const struct scrobble *tracks[1] = {track};
    _info("scrobbler::now_playing[%s]: %s//%s//%s", player->name, track->title, track->artist[0], track->album);
    // TODO(marius): this requires the number of tracks to be passed down, to avoid dependency on arrlen
    api_request_do(scrobbler, tracks, 1, now_playing_is_valid, api_build_request_now_playing);

    if (track->position + NOW_PLAYING_DELAY < (double)track->length) {
        add_event_now_playing(player, track, NOW_PLAYING_DELAY);
    }
}

static bool add_event_now_playing(struct mpris_player *player, const struct scrobble *track, const time_t delay)
{
    assert(NULL != player);
    assert(mpris_player_is_valid(player));
    if (NULL == track || scrobble_is_empty(track)) {
        _trace2("events::add_event:now_playing: skipping, track is empty");
        return false;
    }

    if (player->ignored) {
        _trace2("events::add_event:now_playing: skipping, player %s is ignored", player->name);
        return false;
    }

    const struct timeval now_playing_tv = { .tv_sec = delay };

    struct event_payload *payload = &player->now_playing;
    scrobble_copy(&payload->scrobble, track);

    if (event_initialized(&payload->event)) {
        event_del(&payload->event);
    }
    event_assign(&payload->event, player->evbase, -1, EV_TIMEOUT, send_now_playing, payload);
    if (!event_initialized(&payload->event)) {
        _warn("events::add_event_failed(%p):now_playing", &payload->event);
        return false;
    }

    _debug("events::add_event:now_playing[%s] in %2.2lfs, elapsed %2.2lfs", player->name, timeval_to_seconds(now_playing_tv), (double)track->position);
    event_add(&payload->event, &now_playing_tv);
    payload->scrobble.position += (double)delay;
    payload->scrobble.play_time += (double)delay;

    return true;
}

static void queue(evutil_socket_t fd, short event, void *data)
{
    assert (data);
    struct event_payload *state = data;

    struct mpris_player *player = state->parent;
    if (NULL == player) {
        _debug("events::queue: invalid player %p", player);
        return;
    }

    struct scrobbler *scrobbler = player->scrobbler;
    if (NULL == scrobbler) {
        _debug("events::queue: invalid scrobbler %p", scrobbler);
        return;
    }

    const struct scrobble *scrobble = &state->scrobble;
    assert(NULL != scrobble && !scrobble_is_empty(scrobble));
    //print_scrobble(scrobble, log_tracing);

    _trace("events::triggered(%p:%p):queue", state, scrobbler->queue);
    scrobbles_append(scrobbler, scrobble);

    int queue_count = scrobbler->queue.length;
    if (queue_count > 0) {
        queue_count -= (int)scrobbler_consume_queue(scrobbler);
        _debug("events::new_queue_length: %zu", queue_count);
    }
}

static bool add_event_queue(struct mpris_player *player, const struct scrobble *track)
{
    assert (NULL != player && mpris_player_is_valid(player));
    assert (NULL != track && !scrobble_is_empty(track));

    if (player->ignored) {
        _debug("events::add_event:queue: skipping, player %s is ignored", player->name);
        return false;
    }

    struct event_payload *payload = &player->queue;
    scrobble_copy(&payload->scrobble, track);

    assert(!scrobble_is_empty(track));

    if (event_initialized(&payload->event)) {
        event_del(&payload->event);
    }
    event_assign(&payload->event, player->evbase, -1, EV_TIMEOUT, queue, payload);
    if (!event_initialized(&payload->event)) {
        _warn("events::add_event_failed(%p):queue", &payload->event);
    }

    // This is the event that adds a scrobble to the queue after the correct amount of time
    // round to the second
    const struct timeval timer = {
        .tv_sec = (time_t)(min_scrobble_delay_seconds(track)/1),
    };

    _debug("events::add_event:queue[%s] in %2.2lfs", player->name, timeval_to_seconds(timer));
    event_add(&payload->event, &timer);

    return true;
}

#if 0
static bool add_event_scrobble(struct mpris_player *scrobbler)
{
    if (NULL == scrobbler) { return false; }

    struct timeval scrobble_tv = {.tv_sec = 0 };
    struct player_events *ev = &player->events;
    struct scrobble_payload *payload = scrobble_payload_new(player->scrobbler);

    // Initalize timed event for scrobbling
    // TODO(marius): Split scrobbling into two events:
    //  1. Actually add the current track to the top of the queue in length / 2 or 4 minutes, whichever comes first
    //  2. Process the queue and call APIs with the current queue

    if (event_assign(payload->event, ev->base, -1, EV_PERSIST, send_scrobble, payload) == 0) {
        // round to the second
        scrobble_tv.tv_sec = min_scrobble_delay_seconds(track);
        _debug("events::add_event(%p):scrobble in %2.2lfs", payload->event, timeval_to_seconds(scrobble_tv));
        event_add(payload->event, &scrobble_tv);
    } else {
        _warn("events::add_event_failed(%p):scrobble", payload->event);
    }

    return true;
}
#endif

static void mpris_event_clear(struct mpris_event *ev)
{
    //memset(ev, 0, sizeof(*ev));
    ev->player_state = killed;
    ev->loaded_state = 0;
    ev->playback_status_changed = false;
    ev->position_changed = false;
    ev->track_changed = false;
    ev->volume_changed = false;
    memset(ev->sender_bus_id, 0, sizeof(ev->sender_bus_id));
    ev->timestamp = 0;
    _trace2("mem::zeroed::mpris_event");
}

static bool state_dbus_is_valid(struct dbus *bus)
{
    return (NULL != bus->conn);

}

static bool state_player_is_valid(struct mpris_player *player)
{
    return strlen(player->mpris_name) > 0;
}

static bool state_is_valid(struct state *state) {
    return (
        (NULL != state) &&
        (state->player_count > 0)/* && state_player_is_valid(state->player)*/ &&
        (NULL != state->dbus) && state_dbus_is_valid(state->dbus)
    );
}

void resend_now_playing (struct state *state)
{
    if (!state_is_valid(state)) {
        _error("events::invalid_state");
        return;
    }
    // NOTE(marius): cancel any pending connections
    scrobbler_connections_clean(&state->scrobbler.connections);
    for (int i = 0; i < state->player_count; i++) {
        struct mpris_player *player = &state->players[i];
        check_player(player);
    }
}

#endif // MPRIS_SCROBBLER_SEVENTS_H
