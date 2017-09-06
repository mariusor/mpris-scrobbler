/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SEVENTS_H
#define MPRIS_SCROBBLER_SEVENTS_H

size_t now_playing_events_free(struct event *events[], size_t events_count)
{
    size_t freed_count = 0;
    if (events_count > 0) {
        for (size_t i = 0; i < events_count; i++) {
            //if (ev->now_playing_count > MAX_NOW_PLAYING_EVENTS) { break; }

            struct event* now_playing = events[i];
            if (NULL == now_playing) { continue; }

            _trace("mem::freeing_event(%p//%u):now_playing", now_playing, i);
            event_free(now_playing);
            events[i] = NULL;
            freed_count++;
        }
    }
    return freed_count;
}

void events_free(struct events *ev)
{
    if (NULL != ev->dispatch) {
        _trace("mem::freeing_event(%p):dispatch", ev->dispatch);
        event_free(ev->dispatch);
    }
    if (NULL != ev->scrobble) {
        _trace("mem::freeing_event(%p):scrobble", ev->scrobble);
        event_free(ev->scrobble);
    }
    size_t freed = now_playing_events_free(ev->now_playing, ev->now_playing_count);
    ev->now_playing_count -= freed;
#if 0
    if (NULL != ev->ping) {
        _trace("mem::freeing_event(%p):ping", ev->ping);
        event_free(ev->ping);
    }
#endif
    _trace("mem::freeing_event(%p):SIGINT", ev->sigint);
    event_free(ev->sigint);
    _trace("mem::freeing_event(%p):SIGTERM", ev->sigterm);
    event_free(ev->sigterm);
    _trace("mem::freeing_event(%p):SIGHUP", ev->sighup);
    event_free(ev->sighup);
    free(ev);
}

static void events_init(struct events* ev)
{
    ev->base = event_base_new();
    if (NULL == ev->base) {
        _error("mem::init_libevent: failure");
        return;
    } else {
        _trace("mem::inited_libevent(%p)", ev->base);
    }
    ev->sigint = evsignal_new(ev->base, SIGINT, sighandler, ev->base);
    if (NULL == ev->sigint || event_add(ev->sigint, NULL) < 0) {
        _error("mem::add_event(SIGINT): failed");
        return;
    }
    ev->sigterm = evsignal_new(ev->base, SIGTERM, sighandler, ev->base);
    if (NULL == ev->sigterm || event_add(ev->sigterm, NULL) < 0) {
        _error("mem::add_event(SIGTERM): failed");
        return;
    }
    ev->sighup = evsignal_new(ev->base, SIGHUP, sighandler, ev->base);
    if (NULL == ev->sighup || event_add(ev->sighup, NULL) < 0) {
        _error("mem::add_event(SIGHUP): failed");
        return;
    }
    ev->now_playing_count = 0;
    ev->dispatch = NULL;
    ev->scrobble = NULL;
}

struct events* events_new()
{
    struct events *result = malloc(sizeof(struct events));

    events_init(result);

    return result;
}

static void remove_event_now_playing(struct state *state)
{
    if (NULL == state->events->now_playing) { return; }
    if (state->events->now_playing == 0) { return; }

    _trace("events::remove_events(%p:%u):now_playing", state->events->now_playing, state->events->now_playing_count);
    size_t freed = now_playing_events_free(state->events->now_playing, state->events->now_playing_count);
    state->events->now_playing_count -= freed;
}

struct timeval lasttime;
void lastfm_now_playing(struct scrobbler *, const struct scrobble *);
static void now_playing(evutil_socket_t fd, short event, void *data)
{
#if 0
    struct timeval newtime, difference, now_playing_tv;
    double elapsed;
#endif
    struct state *state = data;
    if (fd) { fd = 0; }
    if (event) { event = 0; }

#if 0
    evutil_gettimeofday(&newtime, NULL);
    evutil_timersub(&newtime, &lasttime, &difference);
    elapsed = difference.tv_sec + (difference.tv_usec / 1.0e6);

    _trace("events::triggered(%p):now_playing %2.3f seconds elapsed", state->events->now_playing, elapsed);
#endif
    struct scrobble* current = scrobble_new();
    load_scrobble(current, state->player->current);
    _trace("events::triggered(%p):now_playing", current);
    lastfm_now_playing(state->scrobbler, current);
#if 0
    // disabling this as we add all now_playing events at track change time
    lasttime = newtime;

    time_t current_time;
    time(&current_time);
    evutil_timerclear(&now_playing_tv);
    // TODO: check if event will trigger before track ends
    //       and skip if not
    //now_playing_tv.tv_sec = NOW_PLAYING_DELAY;
    //event_add(state->events->now_playing, &now_playing_tv);
#endif
}

static void add_event_now_playing(struct state *state)
{
    struct events *ev = state->events;
    if (NULL != ev->now_playing) { remove_event_now_playing(state); }

    struct mpris_properties *current = state->player->current;
    // @TODO: take into consideration the current position
    unsigned current_position = 0;
    unsigned length = current->metadata->length / 1000000;
    ev->now_playing_count = (length - current_position) / NOW_PLAYING_DELAY;

    _trace ("track length: %u adding %u now_playing events", length, ev->now_playing_count);
    for (size_t i = 0; i < ev->now_playing_count; i++) {
        struct timeval now_playing_tv;
        now_playing_tv.tv_sec = NOW_PLAYING_DELAY * i;

        ev->now_playing[i] = malloc(sizeof(struct event));
        // Initalize timed event for now_playing
        event_assign(ev->now_playing[i], ev->base, -1, EV_PERSIST, now_playing, state);
        _trace("events::add_event(%p//%u):now_playing in %2.3f seconds", ev->now_playing[i], i, (double)now_playing_tv.tv_sec);
        evutil_timerclear(&now_playing_tv);

        event_add(ev->now_playing[i], &now_playing_tv);

        //evutil_gettimeofday(&lasttime, NULL);
    }
}

static void remove_event_scrobble(struct state *state)
{
    if (NULL == state->events->scrobble) { return; }

    _trace("events::remove_event(%p):scrobble", state->events->scrobble);

    if (NULL != state->events->scrobble) {
        event_free(state->events->scrobble);
        state->events->scrobble = NULL;
    }
}

static void send_scrobble(evutil_socket_t fd, short event, void *data)
{
    if (NULL == data) { return; }
    struct state *state = data;
    if (fd) { fd = 0; }
    if (event) { event = 0; }

    _trace("events::triggered(%p):scrobble", state->events->scrobble);
    scrobbles_consume_queue(state->player, state->scrobbler);

    remove_event_scrobble(state);
}

static void add_event_scrobble(struct state *state)
{
    struct timeval scrobble_tv;
    struct events *ev = state->events;
    if (NULL == state->player) { return; }
    if (NULL == state->player->current) { return; }
    if (NULL == state->player->current->metadata) { return; }
    //if (NULL != ev->scrobble) { remove_event_scrobble(state); }

    ev->scrobble = malloc(sizeof(struct event));

    // Initalize timed event for scrobbling
    event_assign(ev->scrobble, ev->base, -1, EV_PERSIST, send_scrobble, state);
    evutil_timerclear(&scrobble_tv);

    scrobble_tv.tv_sec = state->player->current->metadata->length / 2000000;
    _trace("events::add_event(%p):scrobble in %2.3f seconds", ev->scrobble, (double)scrobble_tv.tv_sec);
    event_add(ev->scrobble, &scrobble_tv);
}

static bool mpris_event_happened(const struct mpris_event *what_happened)
{
    return (
        what_happened->playback_status_changed ||
        what_happened->volume_changed ||
        what_happened->track_changed
    );
}

void state_loaded_properties(struct state *state, mpris_properties *properties, const struct mpris_event *what_happened)
{
#if 0
    load_event(&what_happened, state);
#endif

    if (!mpris_event_happened(what_happened)) { return; }

    struct scrobble *scrobble = scrobble_new();
    load_scrobble(scrobble, properties);

    bool scrobble_added = false;
    //mpris_properties_copy(state->properties, properties);
    if(what_happened->playback_status_changed) {
        if (NULL != state->events->now_playing) { remove_event_now_playing(state); }
        if (NULL != state->events->scrobble) { remove_event_scrobble(state); }
        if (what_happened->player_state == playing) {
            if (now_playing_is_valid(scrobble)) {
                scrobbles_append(state->player, properties);
                add_event_now_playing(state);
                scrobble_added = true;
            }
        }
    }
    if(what_happened->track_changed) {
        // TODO: maybe add a queue flush event
#if 1
        if (NULL != state->events->now_playing) { remove_event_now_playing(state); }
        if (NULL != state->events->scrobble) { remove_event_scrobble(state); }
#endif

        if(what_happened->player_state == playing && now_playing_is_valid(scrobble)) {
            if (!scrobble_added) {
                scrobbles_append(state->player, properties);
            }
            add_event_now_playing(state);
            add_event_scrobble(state);
        }
    }
    if (what_happened->volume_changed) {
        // trigger volume_changed event
    }
    if (what_happened->position_changed) {
        // trigger position event
    }
    scrobble_free(scrobble);
}

#if 0
static void remove_event_ping(struct state *state)
{
    if (NULL == state->events->ping) { return; }

    _trace("events::remove_event(%p):ping", state->events->ping);

    event_free(state->events->ping);
    state->events->ping = NULL;
}

static void ping(evutil_socket_t fd, short event, void *data)
{
    if (fd) { fd = 0; }
    if (event) { event = 0; }
    if (NULL == data) { return; }
    return;

#if 0
    struct state *state = data;
    bool have_player = false;
    if (NULL == state->player->mpris_name) {
        // try to get players in mpris
        state->player->mpris_name = get_player_namespace(state->dbus->conn);
    }

    if (NULL == state->player->mpris_name) {
        _warn("events::triggered(%p):ping_failed: mpris interface un available", state->events->ping);
        return;
    }
    // we already have a player, we check it's still there
    have_player = ping_player(state->dbus->conn, state->player->mpris_name);

    if (!have_player) {
        _warn("events::triggered(%p):ping_failed: %s", state->events->ping, state->player->mpris_name);
    } else {
        _debug("events::triggered(%p):ping_ok: %s", state->events->ping, state->player->mpris_name);
    }
#endif
}

void add_event_ping(struct state *state)
{
    struct timeval ping_tv;
    struct events *ev = state->events;

    ev->ping = malloc(sizeof(struct event));

    // Initalize timed event for scrobbling
    event_assign(ev->ping, ev->base, -1, EV_PERSIST, ping, state);
    evutil_timerclear(&ping_tv);

    ping_tv.tv_sec = 60;
    _trace("events::add_event(%p):ping in %2.3f seconds", ev->ping, (double)ping_tv.tv_sec);
    event_add(ev->ping, &ping_tv);

    evutil_gettimeofday(&lasttime, NULL);
}
#endif
#endif // MPRIS_SCROBBLER_SEVENTS_H
