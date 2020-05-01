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

struct add_to_queue_payload *add_to_queue_payload_new(struct scrobbler *scrobbler)
{
    struct add_to_queue_payload *p = calloc(1, sizeof(struct add_to_queue_payload));
    p->scrobbler = scrobbler;
    return p;
}

void player_events_free(struct player_events *ev)
{
    if (NULL == ev) { return; }

    if (NULL != ev->now_playing_payload) {
        now_playing_payload_free(ev->now_playing_payload);
    }
}

void events_free(struct events *ev)
{
    if (NULL == ev) { return; }

    _trace2("mem::free::event(%p):SIGINT", ev->sigint);
    event_free(ev->sigint);
    _trace2("mem::free::event(%p):SIGTERM", ev->sigterm);
    event_free(ev->sigterm);
    _trace2("mem::free::event(%p):SIGHUP", ev->sighup);
    event_free(ev->sighup);
}

struct events *events_new(void)
{
    struct events *result = calloc(1, sizeof(struct events));

    return result;
}

void events_init(struct events *ev, struct state *s)
{
    if (NULL == ev) { return; }


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
        struct timeval now_playing_tv = { .tv_sec = NOW_PLAYING_DELAY };
        _trace("events::add::now_playing(%p//%p): ellapsed %2.3f seconds, next in %2.3f seconds", state->event, track, track->position, (double)(now_playing_tv.tv_sec + now_playing_tv.tv_usec));
        event_add(state->event, &now_playing_tv);
    } else {
        _trace("events::end::now_playing(%p//%p)", state->event, state->tracks);
    }
}

static bool add_event_now_playing(struct mpris_player *player, struct scrobble *track)
{
    if (NULL == track) { return false; }
    if (NULL == player) { return false; }
    if (player->ignored) {
        _info("events::add_event:now_playing: skipping, player %s is ignored", player->name);
        return false;
    }

    struct timeval now_playing_tv = { .tv_sec = 0 };
    struct player_events *ev = &player->events;
    struct now_playing_payload *payload = NULL;

    now_playing_payload_free(ev->now_playing_payload);
    // TODO(marius): we need to add the payloads to the state so we can free it for events destroyed before being triggered
    ev->now_playing_payload = now_playing_payload_new(player->scrobbler, track);
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
        //_warn("events::add_event_failed(%p):now_playing", ev->scrobble_payload->event);
    }

    return true;
}

static void add_to_queue(evutil_socket_t fd, short event, void *data)
{
    if (NULL == data) {
        _warn("events::triggered::queue[%d:%d]: missing data", fd, event);
        return;
    }
    struct add_to_queue_payload *state = data;
    struct scrobbler *scrobbler = state->scrobbler;
    if (NULL == scrobbler) {
        _warn("events::triggered::queue[%d:%d]: missing scrobbler", fd, event);
        return;
    }
    struct scrobble *scrobble = state->scrobble;
    if (NULL == scrobble) {
        _warn("events::triggered::queue[%d:%d]: missing track", fd, event);
        return;
    }

    _debug("events::triggered(%p:%p):queue", state, scrobbler->queue);
    if (scrobble_is_valid(scrobble)) {
        scrobbles_append(scrobbler, scrobble);

        int queue_count = arrlen(scrobbler->queue);
        _debug("events::new_queue_length: %zu", queue_count);
    }

    //scrobble_payload_free(state);
}

static void send_scrobble(evutil_socket_t fd, short event, void *data)
{
    if (NULL == data) {
        _warn("events::triggered::scrobble[%d:%d]: missing data", fd, event);
        return;
    }
    struct add_to_queue_payload *payload = data;
    struct scrobbler *scrobbler = payload->scrobbler;

    int queue_count = 0;
    if (NULL == payload->scrobbler) {
        _warn("events::triggered::scrobble[%p]: missing scrobbler info", payload);
        return;
    }
    if (NULL == scrobbler->queue) {
        _warn("events::triggered::scrobble[%p]: nil queue", payload);
        return;
    }
    queue_count = arrlen(scrobbler->queue);
    if (queue_count > 0) {
        _trace("events::triggered(%p:%p):scrobble", payload->event, scrobbler->queue);
        scrobbles_consume_queue(scrobbler, scrobbler->queue);

        scrobbles_free(&scrobbler->queue, false);
        queue_count = arrlen(scrobbler->queue);
        _debug("events::new_queue_length: %zu", queue_count);
    }
}

static bool add_event_add_to_queue(struct scrobbler *scrobbler, struct scrobble *track, struct event_base *base)
{
    if (NULL == scrobbler) { return false; }
    if (NULL == track) { return false; }

    struct timeval timer = {.tv_sec = 0 };
    scrobbler->payload.scrobbler = scrobbler;
    scrobbler->payload.scrobble = track;

    // This is the event that adds a scrobble to the queue after the correct amount of time
    if (event_assign(&scrobbler->payload.event, base, -1, EV_PERSIST, add_to_queue, &scrobbler->payload) == 0) {
        // round to the second
        timer.tv_sec = min_scrobble_seconds(track);
        _debug("events::add_event:to_queue(%p:%p) in %2.3f seconds", &scrobbler->payload, track, (double)(timer.tv_sec + timer.tv_usec));
        event_add(&scrobbler->payload.event, &timer);
    } else {
        _warn("events::add_event_failed(%p):scrobble", scrobbler->payload.event);
    }

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
        scrobble_tv.tv_sec = min_scrobble_seconds(track);
        _debug("events::add_event(%p):scrobble in %2.3f seconds", payload->event, (double)(scrobble_tv.tv_sec + scrobble_tv.tv_usec));
        event_add(payload->event, &scrobble_tv);
    } else {
        _warn("events::add_event_failed(%p):scrobble", payload->event);
    }

    return true;
}
#endif

static inline bool mpris_event_changed_playback_status(const struct mpris_event *ev)
{
    return ev->loaded_state & mpris_load_property_playback_status;
}
static inline bool mpris_event_changed_track(const struct mpris_event *ev)
{
    return ev->loaded_state > mpris_load_property_position;
}
static inline bool mpris_event_changed_volume(const struct mpris_event *ev)
{
    return ev->loaded_state & mpris_load_property_volume;
}
static inline bool mpris_event_changed_position(const struct mpris_event *ev)
{
    return ev->loaded_state & mpris_load_property_position;
}

static inline void mpris_event_clear(struct mpris_event *ev)
{
    ev->playback_status_changed = false;
    ev->volume_changed = false;
    ev->track_changed = false;
    ev->position_changed = false;
    ev->loaded_state = 0;
    _trace2("mem::zeroed::mpris_event");
}

bool state_dbus_is_valid(struct dbus *bus)
{
    return (NULL != bus->conn);

}

bool state_player_is_valid(struct mpris_player *player)
{
    return (NULL != player->mpris_name);
}

bool state_is_valid(struct state *state) {
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
    for (int i = 0; i < state->player_count; i++) {
        struct mpris_player *player = &state->players[i];
        get_mpris_properties(state->dbus->conn, player);
        struct scrobble scrobble = {0};
        if (load_scrobble(&scrobble, &player->properties) && now_playing_is_valid(&scrobble)) {
            add_event_now_playing(player, &scrobble);
        }
    }
}

#endif // MPRIS_SCROBBLER_SEVENTS_H
