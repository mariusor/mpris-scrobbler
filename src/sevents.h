/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef SEVENTS_H
#define SEVENTS_H

void events_free(struct events *ev)
{
    if (NULL != ev->now_playing) {
        _trace("mem::freeing_event(%p):now_playing", ev->now_playing);
        event_free(ev->now_playing);
    }
    if (NULL != ev->dispatch) {
        _trace("mem::freeing_event(%p):dispatch", ev->dispatch);
        event_free(ev->dispatch);
    }
    if (NULL != ev->scrobble) {
        _trace("mem::freeing_event(%p):scrobble", ev->scrobble);
        event_free(ev->scrobble);
    }
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

void events_init(struct events* ev)
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
    ev->now_playing = NULL;
    ev->dispatch = NULL;
    ev->scrobble = NULL;
}

struct events* events_new()
{
    struct events *result = malloc(sizeof(struct events));

    events_init(result);

    return result;
}

void remove_event_now_playing(struct state *state)
{
    if (NULL == state->events->now_playing) { return; }

    _trace("events::remove_event(%p):now_playing", state->events->now_playing);

    event_free(state->events->now_playing);
    state->events->now_playing = NULL;
}

struct timeval lasttime;
void lastfm_now_playing(struct scrobbler *, const struct scrobble *);
void now_playing(evutil_socket_t fd, short event, void *data)
{
    struct state *state = data;
    struct timeval newtime, difference, now_playing_tv;
    double elapsed;
    if (fd) { fd = 0; }
    if (event) { event = 0; }

    evutil_gettimeofday(&newtime, NULL);
    evutil_timersub(&newtime, &lasttime, &difference);
    elapsed = difference.tv_sec + (difference.tv_usec / 1.0e6);

    _trace("events::triggered(%p):now_playing %2.3f seconds elapsed", state->events->now_playing, elapsed);
    lastfm_now_playing(state->scrobbler, state->player->current);
    lasttime = newtime;

    time_t current_time;
    time(&current_time);
    evutil_timerclear(&now_playing_tv);
    // TODO: check if event will trigger before track ends
    //       and skip if not
    now_playing_tv.tv_sec = NOW_PLAYING_DELAY;
    event_add(state->events->now_playing, &now_playing_tv);
}

void add_event_now_playing(struct state *state)
{
    struct timeval now_playing_tv;
    struct events *ev = state->events;
    if (NULL != ev->now_playing) { remove_event_now_playing(state); }

    ev->now_playing = malloc(sizeof(struct event));

    // Initalize timed event for now_playing
    event_assign(ev->now_playing, ev->base, -1, EV_PERSIST, now_playing, state);
    evutil_timerclear(&now_playing_tv);

    now_playing_tv.tv_sec = 0;
    _trace("events::add_event(%p):now_playing", ev->now_playing);
    event_add(ev->now_playing, &now_playing_tv);

    evutil_gettimeofday(&lasttime, NULL);
}

void remove_event_scrobble(struct state *state)
{
    if (NULL == state->events->scrobble) { return; }

    _trace("events::remove_event(%p):scrobble", state->events->scrobble);

    event_free(state->events->scrobble);
    state->events->scrobble = NULL;
}

void send_scrobble(evutil_socket_t fd, short event, void *data)
{
    if (NULL == data) { return; }
    struct state *state = data;
    if (fd) { fd = 0; }
    if (event) { event = 0; }

    _trace("events::triggered(%p):scrobble", state->events->scrobble);
    scrobbles_consume_queue(state->player, state->scrobbler);

    remove_event_scrobble(state);
}

void add_event_scrobble(struct state *state)
{
    struct timeval scrobble_tv;
    struct events *ev = state->events;
    //if (NULL != ev->scrobble) { remove_event_scrobble(state); }

    ev->scrobble = malloc(sizeof(struct event));

    // Initalize timed event for scrobbling
    event_assign(ev->scrobble, ev->base, -1, EV_PERSIST, send_scrobble, state);
    evutil_timerclear(&scrobble_tv);

    scrobble_tv.tv_sec = state->player->current->length / 2;
    _trace("events::add_event(%p):scrobble in %2.3f seconds", ev->scrobble, (double)scrobble_tv.tv_sec);
    event_add(ev->scrobble, &scrobble_tv);

    evutil_gettimeofday(&lasttime, NULL);
}

struct scrobble* scrobble_new();
void state_loaded_properties(struct state *state, mpris_properties *properties)
{
    mpris_event what_happened;
    load_event(&what_happened, properties, state);

    struct scrobble *scrobble = scrobble_new();
    load_scrobble(scrobble, properties);

    //mpris_properties_copy(state->properties, properties);
    state->player->player_state = what_happened.player_state;

    if(what_happened.playback_status_changed) {
        if (NULL != state->events->now_playing) { remove_event_now_playing(state); }
        if (NULL != state->events->scrobble) { remove_event_scrobble(state); }
        if (what_happened.player_state == playing) {
            if (now_playing_is_valid(scrobble)) {
                scrobbles_append(state->player, scrobble);
                add_event_now_playing(state);
            }
        }
    }
    if(what_happened.track_changed) {
        // TODO: maybe add a queue flush event
#if 1
        if (NULL != state->events->now_playing) { remove_event_now_playing(state); }
        if (NULL != state->events->scrobble) { remove_event_scrobble(state); }
#endif

        if(what_happened.player_state == playing && now_playing_is_valid(scrobble)) {
            scrobbles_append(state->player, scrobble);
            add_event_now_playing(state);
            add_event_scrobble(state);
        }
    }
    if (what_happened.volume_changed) {
        // trigger volume_changed event
    }
    scrobble_free(scrobble);
}

void remove_event_ping(struct state *state)
{
    if (NULL == state->events->ping) { return; }

    _trace("events::remove_event(%p):ping", state->events->ping);

    event_free(state->events->ping);
    state->events->ping = NULL;
}

void ping(evutil_socket_t fd, short event, void *data)
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

#endif // SEVENTS_H
