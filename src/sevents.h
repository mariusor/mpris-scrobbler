/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SEVENTS_H
#define MPRIS_SCROBBLER_SEVENTS_H

void now_playing_payload_free(struct now_playing_payload *p)
{
    if (NULL == p) { return; }
    _trace2("mem::free::event_payload(%p):now_playing", p);

    if (NULL != p->tracks) {
        int track_count = arrlen(p->tracks);
        for (int i = track_count - 1; i >= 0; i--) {
            scrobble_free(p->tracks[i]);
            (void)arrpop(p->tracks);
            p->tracks[i] = NULL;
        }
        assert(arrlen(p->tracks) == 0);
        arrfree(p->tracks);
        p->tracks = NULL;
    }
    if (NULL != p->event) {
        _trace("events::remove_event(%p::%p):now_playing", p->event, p);
        event_free(p->event);
        p->event = NULL;
    }

    free(p);
    p = NULL;
}

struct now_playing_payload *now_playing_payload_new(struct scrobbler *scrobbler, struct scrobble *track)
{
    struct now_playing_payload *p = calloc(1, sizeof(struct now_playing_payload));

    if (NULL != track) {
        struct scrobble *t = scrobble_new();
        scrobble_copy(t, track);
        arrput(p->tracks, t);
    }
    p->event = calloc(1, sizeof(struct event));
    p->scrobbler = scrobbler;

    return p;
}

struct scrobble_payload *scrobble_payload_new(struct scrobbler *scrobbler, struct mpris_player *player)
{
    struct scrobble_payload *p = calloc(1, sizeof(struct scrobble_payload));

    p->event = calloc(1, sizeof(struct event));;
    p->scrobbler = scrobbler;
    p->player = player;

    return p;
}

void scrobble_payload_free(struct scrobble_payload *p)
{
    if (NULL == p) { return; }
    _trace2("mem::free::event_payload(%p):scrobble", p);

    _trace("events::remove_event(%p::%p):scrobble", p->event, p);
    event_free(p->event);

    free(p);
}

void events_free(struct events *ev)
{
    if (NULL == ev) { return; }

    if (NULL != ev->curl_timer) {
        _trace2("mem::free::event(%p):curl_timer", ev->curl_timer);
        event_free(ev->curl_timer);
    }
    if (NULL != ev->dispatch) {
        _trace2("mem::free::event(%p):dispatch", ev->dispatch);
        event_free(ev->dispatch);
    }
    if (NULL != ev->scrobble_payload) {
        _trace2("mem::free::event(%p):scrobble", ev->scrobble_payload);
        scrobble_payload_free(ev->scrobble_payload);
        ev->scrobble_payload = NULL;
    }
    if (NULL != ev->now_playing_payload) {
        now_playing_payload_free(ev->now_playing_payload);
    }
    _trace2("mem::free::event(%p):SIGINT", ev->sigint);
    event_free(ev->sigint);
    _trace2("mem::free::event(%p):SIGTERM", ev->sigterm);
    event_free(ev->sigterm);
    _trace2("mem::free::event(%p):SIGHUP", ev->sighup);
    event_free(ev->sighup);
    free(ev);
}

struct events *events_new(void)
{
    struct events *result = calloc(1, sizeof(struct events));

    return result;
}

void events_init(struct state *s)
{

    if (NULL == s) { return; }

    struct events *ev = s->events;
    ev->base = event_base_new();
    if (NULL == ev->base) {
        _error("mem::init_libevent: failure");
        return;
    }

    _trace2("mem::inited_libevent(%p)", ev->base);
    ev->dispatch = NULL;
    ev->scrobble_payload = NULL;
    ev->now_playing_payload = NULL;
    ev->curl_timer = calloc(1, sizeof(struct event));

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
    struct now_playing_payload *state = data;
    if (fd) { fd = 0; }
    if (event) { event = 0; }

    if (NULL == state || NULL == state->tracks) {
        _warn("events::triggered::now_playing: missing current track");
        return;
    }
    assert(arrlen(state->tracks) == 1);
    struct scrobble *track = state->tracks[0];
    if (track->position + NOW_PLAYING_DELAY > track->length) {
        return;
    }

    track->position += NOW_PLAYING_DELAY;
    _trace("events::triggered::now_playing(%p:%p)", state->event, state->tracks);

    const struct scrobble **tracks = (const struct scrobble **)state->tracks;
    _info("scrobbler::now_playing: %s//%s//%s", track->title, track->artist[0], track->album);
    api_request_do(state->scrobbler, tracks, api_build_request_now_playing);

    if (track->position + NOW_PLAYING_DELAY < track->length) {
        struct timeval now_playing_tv = { .tv_sec = NOW_PLAYING_DELAY, .tv_usec = 0 };
        _trace("events::add::now_playing(%p//%p): ellapsed %2.3f seconds, next in %2.3f seconds", state->event, track, track->position, (double)(now_playing_tv.tv_sec + now_playing_tv.tv_usec));
        event_add(state->event, &now_playing_tv);
    } else {
        _trace("events::end::now_playing(%p//%p)", state->event, state->tracks);
    }
}

static bool add_event_now_playing(struct state *state, struct scrobble *track)
{
    if (NULL == state) { return false; }
    if (NULL == track) { return false; }
    if (NULL == state->events) { return false; }

    struct timeval now_playing_tv = { .tv_sec = 0, .tv_usec = 0 };
    struct events *ev = state->events;
    struct now_playing_payload *payload = NULL;

    if (NULL == state->player) { return false; }

    now_playing_payload_free(ev->now_playing_payload);
    // TODO(marius): we need to add the payloads to the state so we can free it for events destroyed before being triggered
    ev->now_playing_payload = now_playing_payload_new(state->scrobbler, track);
    if (NULL == ev->now_playing_payload) {
        _warn("events::failed_add_event:now_playing: insuficient memory");
        return false;
    }
    payload = ev->now_playing_payload;

    //scrobble_zero(ev->now_playing_payload->track);
    //scrobble_copy(ev->now_playing_payload->track, track);

    // @TODO: take into consideration the current position
    //unsigned current_position = track->position;
    //unsigned length = track->length;
    //size_t now_playing_count = (length - current_position) / NOW_PLAYING_DELAY;

    //_trace("events::add_event(%p):now_playing: track_lenth: %zu(s), event_count: %u", ev->now_playing, length, now_playing_count);

    // Initalize timed event for now_playing
    if (event_assign(payload->event, ev->base, -1, EV_PERSIST, send_now_playing, payload) == 0) {
        _debug("events::add_event(%p//%p):now_playing in %2.3f seconds", payload->event, payload->tracks, (double)(now_playing_tv.tv_sec + now_playing_tv.tv_usec));
        event_add(payload->event, &now_playing_tv);
    } else {
        _warn("events::add_event_failed(%p):now_playing", ev->scrobble_payload->event);
    }

    return true;
}

static void send_scrobble(evutil_socket_t fd, short event, void *data)
{
    struct scrobble_payload *state = data;
    if (fd) { fd = 0; }
    if (event) { event = 0; }

    int queue_count = 0;
    if (NULL == state || NULL == state->player || NULL == state->scrobbler || NULL == state->player->queue) {
        _warn("events::triggered::scrobble: missing data");
        return;
    }
    queue_count = arrlen(state->player->queue);
    if (queue_count > 0) {
        _trace("events::triggered(%p:%p):scrobble", state->event, state->player->queue);
        scrobbles_consume_queue(state->scrobbler, state->player->queue);

        scrobbles_free(&state->player->queue, false);
        queue_count = arrlen(state->player->queue);
        _debug("events::new_queue_length: %zu", queue_count);
    }

    scrobble_payload_free(state);
}

static bool add_event_scrobble(struct state *state, struct scrobble *track)
{
    if (NULL == state) { return false; }
    if (NULL == track) { return false; }
    if (NULL == state->events) { return false; }

    struct timeval scrobble_tv = {.tv_sec = 0, .tv_usec = 0};
    struct events *ev = state->events;
    struct scrobble_payload *payload = ev->scrobble_payload;

    if (NULL == state->player) { return false; }

    if (NULL != payload) {
        scrobble_payload_free(payload);
        payload = NULL;
    }
    payload = scrobble_payload_new(state->scrobbler, state->player);

    // Initalize timed event for scrobbling
    // TODO(marius): Split scrobbling into two events:
    //  1. Actually add the current track to the top of the queue in length / 2 or 4 minutes, whichever comes first
    //  2. Process the queue and call APIs with the current queue

    if (event_assign(payload->event, ev->base, -1, EV_PERSIST, send_scrobble, payload) == 0) {
        scrobble_tv.tv_sec = min(track->length / 2.0, MIN_SCROBBLE_MINUTES * 60.0);
        _debug("events::add_event(%p):scrobble in %2.3f seconds", payload->event, (double)scrobble_tv.tv_sec);
        event_add(payload->event, &scrobble_tv);
    } else {
        _warn("events::add_event_failed(%p):scrobble", payload->event);
    }

    return true;
}

static inline void mpris_event_clear(struct mpris_event *ev)
{
    ev->playback_status_changed = false;
    ev->volume_changed = false;
    ev->track_changed = false;
    ev->position_changed = false;
    _trace2("mem::zeroed::mpris_event");
}

static inline bool mpris_event_happened(const struct mpris_event *what_happened)
{
    return (
        what_happened->playback_status_changed ||
        what_happened->volume_changed ||
        what_happened->track_changed ||
        what_happened->position_changed
    );
}
bool state_dbus_is_valid(struct dbus *bus)
{
    return (NULL != bus->conn);

}

bool state_player_is_valid(struct mpris_player *player)
{
    return (NULL != player->mpris_name) && (NULL != player->properties);
}

bool state_is_valid(struct state *state) {
    return (
        (NULL != state) &&
        (NULL != state->player) && state_player_is_valid(state->player) &&
        (NULL != state->dbus) && state_dbus_is_valid(state->dbus)
    );
}

void resend_now_playing (struct state *state)
{
    if (!state_is_valid(state)) {
        _error("events::invalid_state");
        return;
    }
    get_mpris_properties(state->dbus->conn, state->player->mpris_name, state->player->properties, state->player->changed);
    struct scrobble *scrobble = scrobble_new();
    if (!load_scrobble(scrobble, state->player->current) && !now_playing_is_valid(scrobble)) { return; }
    add_event_now_playing(state, scrobble);
    scrobble_free(scrobble);
}

void state_loaded_properties(struct state *state, struct mpris_properties *properties, const struct mpris_event *what_happened)
{
    if (!mpris_event_happened(what_happened)) {
        _debug("events::loaded_properties: nothing found");
        goto _exit;
    }
    if (mpris_properties_equals(state->player->current, properties)) {
        _debug("events::invalid_mpris_properties[%p::%p]: already loaded", state->player->current, properties);
        goto _exit;
    }

    debug_event(what_happened);

    struct scrobble *scrobble = scrobble_new();
    if (!load_scrobble(scrobble, properties)) {
        _warn("events::unable_to_load_scrobble[%p]");
        goto _exit_with_scrobble;
    }
    if (!now_playing_is_valid(scrobble)) {
        _warn("events::invalid_now_playing[%p]");
        goto _exit_with_scrobble;
    }
    mpris_properties_copy(state->player->current, properties);

#if 0
    if ( NULL != state->events->now_playing_payload) {
        now_playing_payload_free(state->events->now_playing_payload);
    }
    if ( NULL != state->events->scrobble_payload) {
        scrobble_payload_free(state->events->scrobble_payload);
    }
#endif

    bool scrobble_added = false;
    bool now_playing_added = false;
    if(what_happened->playback_status_changed && !what_happened->track_changed) {
        if (what_happened->player_state == playing) {
            now_playing_added = add_event_now_playing(state, scrobble);
            scrobble_added = scrobbles_append(state->player, scrobble);
        }
    }
    if(what_happened->track_changed) {
        if (what_happened->player_state == playing) {
            if (!now_playing_added) {
                add_event_now_playing(state, scrobble);
            }
            if (!scrobble_added) {
                scrobbles_append(state->player, scrobble);
            }
            add_event_scrobble(state, scrobble);
        }
    }
    if (what_happened->volume_changed) {
        // trigger volume_changed event
    }
    if (what_happened->position_changed) {
        // trigger position event
    }

_exit_with_scrobble:
    scrobble_free(scrobble);

_exit:
    mpris_event_clear(state->player->changed);
    _debug("events::zeroing properties");
    mpris_properties_zero(state->player->properties, true);
}

#endif // MPRIS_SCROBBLER_SEVENTS_H
