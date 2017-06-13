/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef SEVENTS_H
#define SEVENTS_H

void events_free(events *ev)
{
    if (NULL != ev->now_playing) {
        _log(tracing, "mem::freeing_event(%p):now_playing", ev->now_playing);
        event_free(ev->now_playing);
    }
    if (NULL != ev->dispatch) {
        _log(tracing, "mem::freeing_event(%p):dispatch", ev->dispatch);
        event_free(ev->dispatch);
    }
    if (NULL != ev->scrobble) {
        _log(tracing, "mem::freeing_event(%p):scrobble", ev->scrobble);
        event_free(ev->dispatch);
    }
    _log(tracing, "mem::freeing_event(%p):SIGINT", ev->sigint);
    event_free(ev->sigint);
    _log(tracing, "mem::freeing_event(%p):SIGTERM", ev->sigterm);
    event_free(ev->sigterm);
    _log(tracing, "mem::freeing_event(%p):SIGHUP", ev->sighup);
    event_free(ev->sighup);
    free(ev);
}

void events_init(events* ev)
{
    ev->base = event_base_new();
    if (NULL == ev->base) {
        _log(error, "mem::init_libevent: failure");
        return;
    } else {
        _log(tracing, "mem::inited_libevent(%p)", ev->base);
    }
    ev->sigint = evsignal_new(ev->base, SIGINT, sighandler, ev->base);
    if (NULL == ev->sigint || event_add(ev->sigint, NULL) < 0) {
        _log(error, "mem::add_event(SIGINT): failed");
        return;
    }
    ev->sigterm = evsignal_new(ev->base, SIGTERM, sighandler, ev->base);
    if (NULL == ev->sigterm || event_add(ev->sigterm, NULL) < 0) {
        _log(error, "mem::add_event(SIGTERM): failed");
        return;
    }
    ev->sighup = evsignal_new(ev->base, SIGHUP, sighandler, ev->base);
    if (NULL == ev->sighup || event_add(ev->sighup, NULL) < 0) {
        _log(error, "mem::add_event(SIGHUP): failed");
        return;
    }
    ev->now_playing = NULL;
    ev->dispatch = NULL;
    ev->scrobble = NULL;
}

events* events_new()
{
    events *result = malloc(sizeof(events));

    events_init(result);

    return result;
}

struct timeval lasttime;
void lastfm_now_playing(lastfm_scrobbler *, const scrobble *);
void now_playing(evutil_socket_t fd, short event, void *data)
{
    state *state = data;
    struct timeval newtime, difference, now_playing_tv;
    double elapsed;
    if (fd) { fd = 0; }
    if (event) { event = 0; }

    evutil_gettimeofday(&newtime, NULL);
    evutil_timersub(&newtime, &lasttime, &difference);
    elapsed = difference.tv_sec + (difference.tv_usec / 1.0e6);

    _log(tracing, "events::triggered(%p):now_playing %2.3f seconds elapsed", state->events->now_playing, elapsed);
    lastfm_now_playing(state->scrobbler, state->current);
    lasttime = newtime;

    evutil_timerclear(&now_playing_tv);
    now_playing_tv.tv_sec = LASTFM_NOW_PLAYING_DELAY;
    event_add(state->events->now_playing, &now_playing_tv);
}

void remove_event_now_playing(state *state)
{
    if (NULL == state->events->now_playing) { return; }

    _log(tracing, "events::remove_event(%p):now_playing", state->events->now_playing);

    event_free(state->events->now_playing);
    state->events->now_playing = NULL;
}

void add_event_now_playing(state *state)
{
    struct timeval now_playing_tv;
    events *ev = state->events;
    if (NULL != ev->now_playing) { remove_event_now_playing(state); }

    ev->now_playing = malloc(sizeof(struct event));

    // Initalize timed event for now_playing
    event_assign(ev->now_playing, ev->base, -1, EV_PERSIST, now_playing, state);
    evutil_timerclear(&now_playing_tv);

    now_playing_tv.tv_sec = 0;
    _log(tracing, "events::add_event(%p):now_playing", ev->now_playing);
    event_add(ev->now_playing, &now_playing_tv);

    evutil_gettimeofday(&lasttime, NULL);
}

void remove_event_scrobble(state *state)
{
    if (NULL == state->events->scrobble) { return; }

    _log(tracing, "events::remove_event(%p):scrobble", state->events->scrobble);

    event_free(state->events->scrobble);
    state->events->scrobble = NULL;
}

void send_scrobble(evutil_socket_t fd, short event, void *data)
{
    state *state = data;
    if (fd) { fd = 0; }
    if (event) { event = 0; }

    _log(tracing, "events::triggered(%p):scrobble", state->events->scrobble);
    scrobbles_consume_queue(state);

    //remove_event_scrobble(state);
}

void add_event_scrobble(state *state)
{
    struct timeval scrobble_tv;
    events *ev = state->events;
    //if (NULL != ev->scrobble) { remove_event_scrobble(state); }

    ev->scrobble = malloc(sizeof(struct event));

    // Initalize timed event for scrobbling
    event_assign(ev->scrobble, ev->base, -1, EV_PERSIST, send_scrobble, state);
    evutil_timerclear(&scrobble_tv);

    scrobble_tv.tv_sec = state->current->length / 2;
    _log(tracing, "events::add_event(%p):scrobble in %2.3f seconds", ev->scrobble, (double)scrobble_tv.tv_sec);
    event_add(ev->scrobble, &scrobble_tv);

    evutil_gettimeofday(&lasttime, NULL);
}

scrobble* scrobble_new();
void state_loaded_properties(state *state, mpris_properties *properties)
{
    mpris_event what_happened;
    load_event(&what_happened, properties, state);

    scrobble *scrobble = scrobble_new();
    load_scrobble(scrobble, properties);

    mpris_properties_copy(state->properties, properties);
    state->player_state = what_happened.player_state;

#if 1
    if(what_happened.player_state == playing) {
        //add_event_now_playing(state);

        if (now_playing_is_valid(scrobble)) {
            scrobbles_append(state, scrobble);
            //add_event_scrobble(state);
        }
    } else {
        //remove_event_now_playing(state);
        if (NULL != state->events->scrobble) { remove_event_scrobble(state); }
    }
    if (what_happened.volume_changed) {
        // trigger volume_changed event
    }
#endif
    scrobble_free(scrobble);
}

#endif // SEVENTS_H
