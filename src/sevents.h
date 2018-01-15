/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SEVENTS_H
#define MPRIS_SCROBBLER_SEVENTS_H

size_t now_playing_events_free(struct event *events[], size_t events_count, size_t how_many)
{
    if (NULL == events) {
        return 0;
    }
    if (events_count == 0) {
        return 0;
    }
    if (how_many > events_count) {
        how_many = events_count;
    }
    size_t freed_count = 0;
    _trace("mem::free::events[%p]:now_playing, events count: %u", events, how_many);
    for (size_t i = how_many; i > 0; i--) {
        size_t off = events_count - i;
        struct event *now_playing = events[off];
        if (NULL == now_playing) { continue; }

        //_trace("mem::free::event(%p)[%u]:now_playing", now_playing, off);
        event_free(now_playing);
        events[off] = NULL;
        freed_count++;
    }
    return freed_count;
}

void events_free(struct events *ev)
{
    if (NULL != ev->dispatch) {
        _trace("mem::free::event(%p):dispatch", ev->dispatch);
        event_free(ev->dispatch);
    }
    if (NULL != ev->scrobble) {
        _trace("mem::free::event(%p):scrobble", ev->scrobble);
        event_free(ev->scrobble);
    }
    ev->now_playing_count -= now_playing_events_free(ev->now_playing, ev->now_playing_count, ev->now_playing_count);
    _trace("mem::free::event(%p):SIGINT", ev->sigint);
    event_free(ev->sigint);
    _trace("mem::free::event(%p):SIGTERM", ev->sigterm);
    event_free(ev->sigterm);
    _trace("mem::free::event(%p):SIGHUP", ev->sighup);
    event_free(ev->sighup);
    free(ev);
}

struct events *events_new(void)
{
    struct events *result = calloc(1, sizeof(struct events));

    return result;
}

void events_init(struct events *ev, struct sighandler_payload *p)
{
    ev->base = event_base_new();
    if (NULL == ev->base) {
        _error("mem::init_libevent: failure");
        return;
    } else {
        _trace("mem::inited_libevent(%p)", ev->base);
    }
    p->event_base = ev->base;
    ev->dispatch = NULL;
    ev->scrobble = NULL;
    ev->now_playing_count = 0;
    ev->now_playing[0] = NULL;

    ev->sigint = evsignal_new(ev->base, SIGINT, sighandler, p);
    if (NULL == ev->sigint || event_add(ev->sigint, NULL) < 0) {
        _error("mem::add_event(SIGINT): failed");
        return;
    }
    ev->sigterm = evsignal_new(ev->base, SIGTERM, sighandler, p);
    if (NULL == ev->sigterm || event_add(ev->sigterm, NULL) < 0) {
        _error("mem::add_event(SIGTERM): failed");
        return;
    }
    ev->sighup = evsignal_new(ev->base, SIGHUP, sighandler, p);
    if (NULL == ev->sighup || event_add(ev->sighup, NULL) < 0) {
        _error("mem::add_event(SIGHUP): failed");
        return;
    }
}

static void remove_events_now_playing(struct state *state, size_t count)
{
    if (NULL == state) { return; }
    if (NULL == state->events) { return; }
    if (state->events->now_playing_count == 0) { return; }
    if (count == 0) {
        count = state->events->now_playing_count;
    }
    if (count == 0) { return; }

    _trace("events::remove_events(%u:%p):now_playing", count, state->events->now_playing);
    state->events->now_playing_count -= now_playing_events_free(state->events->now_playing, state->events->now_playing_count, count);
    if (count == state->events->now_playing_count && NULL != state->player->current) {
        mpris_properties_free(state->player->current);
    }
}

bool scrobbler_now_playing(struct scrobbler*, const struct scrobble*);
static void send_now_playing(evutil_socket_t fd, short event, void *data)
{
    if (NULL == data) {
        _warn("events::triggered::now_playing: missing data");
        return;
    }

    struct state *state = data;
    if (fd) { fd = 0; }
    if (event) { event = 0; }
    if (NULL == state->player->current) {
        _warn("events::triggered::now_playing: missing current track");
        return;
    }
    struct scrobble *current = scrobble_new();
    load_scrobble(current, state->player->current);
    _trace("events::triggered(%p:%p)[%u]:now_playing", state->events->now_playing[state->events->now_playing_count - 1], current, state->events->now_playing_count - 1);
    scrobbler_now_playing(state->scrobbler, current);
    scrobble_free(current);
}

static void mpris_properties_print(struct mpris_metadata *s) {
    _trace("print metadata: \n" \
        "\ttitle: %s\n" \
        "\tartist: %s\n" \
        "\talbum: %s",
    s->title, s->artist, s->album);
}

static bool add_event_now_playing(struct state *state)
{
    struct events *ev = state->events;
    if (NULL != ev->now_playing[0]) { remove_events_now_playing(state, 0); }

    struct mpris_properties *current = state->player->current;
    if (NULL == current) { return false; }
    // @TODO: take into consideration the current position
    unsigned current_position = current->position / 1000000;
    unsigned length = current->metadata->length / 1000000;
    ev->now_playing_count = (length - current_position) / NOW_PLAYING_DELAY + 1;

    mpris_properties_print(current->metadata);
    _trace("events::add_event(%p):now_playing: track_lenth: %u(s), event_count: %u", ev->now_playing, length, ev->now_playing_count);
    for (size_t i = 0; i < ev->now_playing_count; i++) {
        struct timeval now_playing_tv = {
            .tv_sec = NOW_PLAYING_DELAY * (ev->now_playing_count - i - 1),
            .tv_usec = 0
        };

        ev->now_playing[i] = calloc(1, sizeof(struct event));
        // Initalize timed event for now_playing
        if ( event_assign(ev->now_playing[i], ev->base, -1, EV_PERSIST, send_now_playing, state) == 0) {
            _trace("events::add_event(%p//%p)[%u]:now_playing in %2.3f seconds", ev->now_playing[i], state->player->current, i, (double)(now_playing_tv.tv_sec + now_playing_tv.tv_usec));
            event_add(ev->now_playing[i], &now_playing_tv);
        }
    }

    return true;
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
    scrobbles_consume_queue(state->scrobbler, state->player);
}

static void add_event_scrobble(struct state *state)
{
    struct timeval scrobble_tv = {.tv_sec = 0, .tv_usec = 0};
    struct events *ev = state->events;

    if (NULL == state->player) { return; }
    if (NULL == state->player->current) { return; }
    if (NULL == state->player->current->metadata) { return; }
    if (NULL != ev->scrobble) { remove_event_scrobble(state); }

    ev->scrobble = calloc(1, sizeof(struct event));
    if (NULL == ev->scrobble) { return; }

    // Initalize timed event for scrobbling
    event_assign(ev->scrobble, ev->base, -1, EV_PERSIST, send_scrobble, state);

    scrobble_tv.tv_sec = state->player->current->metadata->length / 2000000;
    _trace("events::add_event(%p):scrobble in %2.3f seconds", ev->scrobble, (double)scrobble_tv.tv_sec);
    event_add(ev->scrobble, &scrobble_tv);
}

static inline void mpris_event_clear(struct mpris_event *ev)
{
    ev->playback_status_changed = false;
    ev->volume_changed = false;
    ev->track_changed = false;
    ev->position_changed = false;
    _trace("mem::zeroed::mpris_event\n");

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

void state_loaded_properties(struct state *state, struct mpris_properties *properties, const struct mpris_event *what_happened)
{
    if (!mpris_event_happened(what_happened)) {
        _debug("events::loaded_properties: nothing happened");
        return;
    }

    struct scrobble *scrobble = scrobble_new();
    load_scrobble(scrobble, properties);

    bool scrobble_added = false;
    bool now_playing_added = false;
    mpris_properties_copy(state->player->current, properties);
    if(what_happened->playback_status_changed && !what_happened->track_changed) {
        if (NULL != state->events->now_playing[0]) { remove_events_now_playing(state, 0); }
        if (NULL != state->events->scrobble) { remove_event_scrobble(state); }

        if (what_happened->player_state == playing && now_playing_is_valid(scrobble)) {
            now_playing_added = add_event_now_playing(state);
            scrobble_added = scrobbles_append(state->player, properties);
        }
    }
    if(what_happened->track_changed) {
        // TODO: maybe add a queue flush event
#if 1
        if (NULL != state->events->now_playing[0]) { remove_events_now_playing(state, 0); }
        if (NULL != state->events->scrobble) { remove_event_scrobble(state); }
#endif

        if(what_happened->player_state == playing && now_playing_is_valid(scrobble)) {
            if (!now_playing_added) {
                now_playing_added = add_event_now_playing(state);
            }
            if (!scrobble_added) {
                scrobble_added = scrobbles_append(state->player, properties);
            }
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
    mpris_event_clear(state->player->changed);
}

#endif // MPRIS_SCROBBLER_SEVENTS_H
