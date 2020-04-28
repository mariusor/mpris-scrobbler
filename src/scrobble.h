/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SCROBBLE_H
#define MPRIS_SCROBBLER_SCROBBLE_H

#include <assert.h>
#include <time.h>

#define NOW_PLAYING_DELAY 65.0 //seconds
#define MIN_TRACK_LENGTH  30.0 // seconds
#define MPRIS_SPOTIFY_TRACK_ID_PREFIX                          "spotify:track:"

int load_player_namespaces(DBusConnection *, struct mpris_player *, int);
void get_mpris_properties(DBusConnection*, struct mpris_player*);

struct dbus *dbus_connection_init(struct state*);

static void debug_event(const struct mpris_event *e)
{
    _trace2("scrobbler::player:                           %s", e->sender_bus_id);
    _debug("scrobbler::checking_volume_changed:             %3s", e->volume_changed ? "yes" : "no");
    _debug("scrobbler::checking_position_changed:           %3s", e->position_changed ? "yes" : "no");
    _debug("scrobbler::checking_playback_status_changed:    %3s", e->playback_status_changed ? "yes" : "no");
    _debug("scrobbler::checking_track_changed:              %3s", e->track_changed ? "yes" : "no");
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


#if 0
const char *get_api_status_label (api_return_codes code)
{
    switch (code) {
        case unavaliable:
            return "The service is temporarily unavailable, please try again.";
        case invalid_service:
            return "Invalid service - This service does not exist";
        case invalid_method:
            return "Invalid Method - No method with that name in this package";
        case authentication_failed:
            return "Authentication Failed - You do not have permissions to access the service";
        case invalid_format:
            return "Invalid format - This service doesn't exist in that format";
        case invalid_parameters:
            return "Invalid parameters - Your request is missing a required parameter";
        case invalid_resource:
            return "Invalid resource specified";
        case operation_failed:
            return "Operation failed - Something else went wrong";
        case invalid_session_key:
            return "Invalid session key - Please re-authenticate";
        case invalid_apy_key:
            return "Invalid API key - You must be granted a valid key by last.fm";
        case service_offline:
            return "Service Offline - This service is temporarily offline. Try again later.";
        case invalid_signature:
            return "Invalid method signature supplied";
        case temporary_error:
            return "There was a temporary error processing your request. Please try again";
        case suspended_api_key:
            return "Suspended API key - Access for your account has been suspended, please contact Last.fm";
        case rate_limit_exceeded:
            return "Rate limit exceeded - Your IP has made too many requests in a short period";
    }
    return "Unkown";
}
#endif

double min_scrobble_seconds(const struct scrobble *s)
{
    double result = MIN_SCROBBLE_MINUTES * 60.0;
    if (s->length > 0 && s->length / 2.0 > result) {
        result = s->length / 2.0;
    }
    return result;
}

static void scrobble_init(struct scrobble *s)
{
    if (NULL == s) { return; }
    s->scrobbled = false;
    s->length = 0;
    s->position = 0;
    s->start_time = 0;
    s->track_number = 0;
    s->play_time = 0UL;

    _trace2("mem::inited_scrobble(%p)", s);
}

struct scrobble *scrobble_new(void)
{
    struct scrobble *result = malloc(sizeof(struct scrobble));
    scrobble_init(result);

    return result;
}

void scrobble_free(struct scrobble *s)
{
    if (NULL == s) { return; }
    _trace2("mem::free::scrobble(%p)", s);
    free(s);
}

void scrobbles_free(struct scrobble **tracks[], bool free_buff)
{
    if (NULL == tracks) { return; }

    int track_count = arrlen(*tracks);
    if (track_count == 0) { goto _free_buffer; }

    for (int i = track_count - 1; i >= 0; i--) {
        if ( NULL == (*tracks)[i]) { continue; }

        _trace("scrobbler::freeing_track(%zu:%p) ", i, (*tracks)[i]);
        scrobble_free((*tracks)[i]);
        (void)arrpop(*tracks);
        (*tracks)[i] = NULL;
    }
    assert(arrlen(*tracks) == 0);
_free_buffer:
    if (free_buff) { arrfree(*tracks); }
}

#if 0
static bool scrobbles_remove(struct mpris_properties *queue[], size_t queue_length, size_t pos)
{
    struct mpris_properties *last = queue[pos];
    if (NULL == last) { return false; }
    struct mpris_metadata *d = last->metadata;
    _trace("scrobbler::remove_scrobble(%p//%u) %s//%s//%s", last, pos, d->title, d->artist, d->album);
    if (pos >= queue_length) { return queue_length; }

    mpris_properties_free(last);
    queue[pos] = NULL;

    //player->previous = player->queue[pos-1];
    return true;
}
#endif

void player_events_free(struct player_events*);
static void scrobble_init(struct scrobble*);
void scrobbles_free(struct scrobble***, bool);
static void mpris_player_free(struct mpris_player *player)
{
    if (NULL == player) { return; }

    if (NULL != player->queue) { scrobbles_free(&player->queue, true); }
#if 0
    if (NULL != player->current) { mpris_properties_free(player->current); }
#endif
    player_events_free(&player->events);
}

void events_free(struct events*);
void dbus_close(struct state*);
void scrobbler_free(struct scrobbler*);
void state_destroy(struct state *s)
{
    if (NULL != s->dbus) { dbus_close(s); }
    for (int i = 0; i < s->player_count; i++) {
        mpris_player_free(&s->players[i]);
    }
    if (NULL != s->scrobbler) { scrobbler_free(s->scrobbler); }
    events_free(&s->events);
}

static struct mpris_player *mpris_player_new(void)
{
    struct mpris_player *result = calloc(1, sizeof(struct mpris_player));

    return (result);
}

static void handle_dispatch_status(DBusConnection *, DBusDispatchStatus, void *);
static void toggle_watch(DBusWatch *, void *);
static void remove_watch(DBusWatch *, void *);
static unsigned add_watch(DBusWatch *, void *);
static void dispatch(int, short, void *);
static DBusHandlerResult add_filter(DBusConnection *, DBusMessage *, void *);
void state_loaded_properties(DBusConnection *, struct mpris_player *, struct mpris_properties *, const struct mpris_event *);
static void get_player_identity(DBusConnection*, const char*, char*);

static int mpris_player_init (struct dbus *dbus, struct mpris_player *player, struct events *events, struct scrobbler *scrobbler, const char ignored[MAX_PLAYERS][MAX_PROPERTY_LENGTH], int ignored_count)
{
    if (strlen(player->mpris_name)+strlen(player->bus_id) == 0) {
        return -1;
    }
    const char *identity = player->mpris_name;
    if (strlen(identity)) {
        identity = player->bus_id;
    }
    get_player_identity(dbus->conn, identity, player->name);
    for (int j = 0; j < ignored_count; j++) {
        if (
            !strncmp(player->mpris_name, ignored[j], MAX_PROPERTY_LENGTH) ||
            !strncmp(player->name, ignored[j], MAX_PROPERTY_LENGTH)
        ) {
            player->ignored = true;
        }
    }
    if (player->ignored) {
        _debug("mpris_player::ignored: %s", player->name);
        return 0;
    }
    player->scrobbler = scrobbler;
    player->events.base = events->base;
    player->events.scrobble_payload = NULL;
    player->events.now_playing_payload = NULL;
    player->queue = NULL;

    get_mpris_properties(dbus->conn, player);
    memcpy(player->properties.player_name, player->name, MAX_PROPERTY_LENGTH);

    player->current = mpris_properties_new();
    state_loaded_properties(dbus->conn, player, &player->properties, &player->changed);
    return 1;
}

static int mpris_players_init(struct dbus *dbus, struct mpris_player *players, struct events *events, struct scrobbler *scrobbler, const char ignored[MAX_PLAYERS][MAX_PROPERTY_LENGTH], int ignored_count)
{
    if (NULL == players){
        return -1;
    }
    if (NULL == dbus){
        _error("players::init: failed, unable to load from dbus");
        return -1;
    }
    int player_count = load_player_namespaces(dbus->conn, players, MAX_PLAYERS);
    int loaded_player_count = 0;
    for (int i = 0; i < player_count; i++) {
        struct mpris_player player = players[i];
        int loaded = mpris_player_init(dbus, &player, events, scrobbler, ignored, ignored_count);
        if (loaded == 0) {
            mpris_player_free(&player);
        } else if (loaded > 0) {
            loaded_player_count++;
            _debug("mpris_player::already_opened[%d]: %s%s", i+1, player.mpris_name, player.bus_id);
        }
    }

    return loaded_player_count;
}

static void print_scrobble_valid_check(const struct scrobble *s, enum log_levels log)
{
    if (NULL == s) {
        return;
    }
    _log(log, "scrobble::valid::title[%s]: %s", s->title, strlen(s->title) > 0 ? "yes" : "no");
    _log(log, "scrobble::valid::album[%s]: %s", s->album, strlen(s->album) > 0 ? "yes" : "no");
    _log(log, "scrobble::valid::length[%u]: %s", s->length, s->length > MIN_TRACK_LENGTH ? "yes" : "no");
    double scrobble_interval = min_scrobble_seconds(s);
    double d = 0;
    if (s->play_time > 0) {
        d = s->play_time;
    } else if (s->start_time > 0) {
        time_t now = time(0);
        d = difftime(now, s->start_time) + 1lu;
    }
    _log(log, "scrobble::valid::play_time[%.2lf:%.2lf]: %s", d, scrobble_interval, d >= scrobble_interval ? "yes" : "no");
    if (NULL != s->artist[0]) {
        _log(log, "scrobble::valid::artist[%s]: %s", s->artist[0], strlen(s->artist[0]) > 0 ? "yes" : "no");
    }
    _log(log, "scrobble::valid::scrobbled: %s", !s->scrobbled ? "yes" : "no");
}

static bool scrobble_is_valid(const struct scrobble *s)
{
    if (NULL == s) { return false; }
    if (array_count(s->artist) == 0 || NULL == s->artist[0]) { return false; }

    double scrobble_interval = min_scrobble_seconds(s);
    double d;
    if (s->play_time > 0) {
        d = s->play_time;
    } else {
        time_t now = time(0);
        d = difftime(now, s->start_time) + 1lu;
    }

    bool result = (
        s->length >= MIN_TRACK_LENGTH &&
        d >= scrobble_interval &&
        s->scrobbled == false &&
        strlen(s->title) > 0 &&
        strlen(s->artist[0]) > 0 &&
        strlen(s->album) > 0
    );
    if (!result) {
        print_scrobble_valid_check(s, log_warning);
    }
    return result;
}

bool now_playing_is_valid(const struct scrobble *m/*, const time_t current_time, const time_t last_playing_time*/) {
    if (NULL == m) { return false; }
    if (array_count(m->artist) == 0 || NULL == m->artist[0]) { return false; }

    bool result = (
        strlen(m->title) > 0 &&
        strlen(m->artist[0]) > 0 &&
        strlen(m->album) > 0 &&
//        last_playing_time > 0 &&
//        difftime(current_time, last_playing_time) >= LASTFM_NOW_PLAYING_DELAY &&
        m->length > 0.0
    );

#if 0
    if (!result) {
        print_scrobble_valid_check(m, log_debug);
    }
#endif
    return result;
}

#if 0
void scrobble_print(const struct scrobble *s, enum log_levels log) {
    _log(log, "scrobble[%p]: \n" \
        "\ttitle: %s\n" \
        "\tartist: %s\n" \
        "\talbum: %s",
        s->title, s->artist[0], s->album);
}

static void scrobble_zero (struct scrobble *s)
{
    if (NULL == s) { return; }
    if (NULL != s->title) {
        memset(s->title, 0, strlen(s->title));
    }
    if (NULL != s->album) {
        memset(s->album, 0, strlen(s->album));
    }
    if (NULL != s->artist) {
        memset(s->artist, 0, strlen(s->artist));
    }
    if (NULL != s->mb_track_id) {
        memset(s->mb_track_id, 0, strlen(s->mb_track_id));
    }
    if (NULL != s->mb_album_id) {
        memset(s->mb_album_id, 0, strlen(s->mb_album_id));
    }
    if (NULL != s->mb_artist_id) {
        memset(s->mb_artist_id, 0, strlen(s->mb_artist_id));
    }
    if (NULL != s->mb_album_artist_id) {
        memset(s->mb_album_artist_id, 0, strlen(s->mb_album_artist_id));
    }
    if (NULL != s->mb_spotify_id) {
        memset(s->mb_spotify_id, 0, strlen(s->mb_spotify_id));
    }

    s->length = 0;
    s->position = 0;
    s->start_time = 0;
    s->play_time = 0;
    s->track_number = 0;
}
#endif

static void scrobble_copy (struct scrobble *t, const struct scrobble *s)
{
    if (NULL == t) { return; }
    if (NULL == s) { return; }

    assert(s->title);
    assert(s->album);
    assert(s->artist);
    assert(t->album);
    assert(t->title);

    memcpy(t->title, s->title, sizeof(s->title));
    memcpy(t->album, s->album, sizeof(s->album));
    memcpy(t->artist, s->artist, sizeof(s->artist));
    memcpy(t->mb_track_id, s->mb_track_id, sizeof(s->mb_track_id));
    memcpy(t->mb_album_id, s->mb_album_id, sizeof(s->mb_album_id));
    memcpy(t->mb_artist_id, s->mb_artist_id, sizeof(s->mb_artist_id));
    memcpy(t->mb_album_artist_id, s->mb_album_artist_id, sizeof(s->mb_album_artist_id));
    memcpy(t->mb_spotify_id, s->mb_spotify_id, sizeof(s->mb_spotify_id));

    t->length = s->length;
    t->position = s->position;
    t->start_time = s->start_time;
    t->play_time = s->play_time;
    t->track_number = s->track_number;

    _trace("scrobbler::copied_scrobble:%p->%p", s, t);
}

static bool scrobbles_equal(const struct scrobble *s, const struct scrobble *p)
{
    if ((NULL == s) && (NULL == p)) { return true; }

    if (NULL == s) { return false; }
    if (NULL == p) { return false; }

    if (s == p) { return true; }

    assert(s->title);
    assert(p->title);
    assert(s->album);
    assert(p->album);
    assert(s->artist);
    assert(p->artist);
    int s_artist_len = arrlen(s->artist);
    int p_artist_len = arrlen(p->artist);
    bool result = (
        (strncmp(s->title, p->title, strlen(s->title)) == 0)
        && (strncmp(s->album, p->album, strlen(s->album)) == 0)
        && s_artist_len == p_artist_len
#if 1
        && (s->length == p->length)
        && (s->track_number == p->track_number)
#endif
    );
    for (int i = s_artist_len - 1; i >= 0 ; i--) {
        result &= (strncmp(s->artist[i], p->artist[i], MAX_PROPERTY_LENGTH) == 0);
    }

    _trace("scrobbler::check_scrobbles(%p:%p) %s", s, p, result ? "same" : "different");

    return result;
}

bool load_scrobble(struct scrobble *d, const struct mpris_properties *p)
{
    if (NULL == d) {
        _trace2("load_scrobble::invalid_destination_ptr");
        return false;
    }
    if (NULL == p) {
        _trace2("load_scrobble::invalid_source_ptr");
        return false;
    }

    if (strlen(p->metadata.title) == 0) {
        _trace2("load_scrobble::invalid_source_metadata_title");
        return false;
    }

    memcpy(d->title, p->metadata.title, sizeof(p->metadata.title));
    memcpy(d->album, p->metadata.album, sizeof(p->metadata.album));
    memcpy(d->artist, p->metadata.artist, sizeof(p->metadata.artist));

    d->length = 0u;
    if (p->metadata.length > 0) {
        d->length = p->metadata.length / 1000000lu;
    }
    if (p->position > 0) {
        d->position = p->position / 1000000lu;
    }
    d->scrobbled = false;
    d->track_number = p->metadata.track_number;
    d->start_time = p->metadata.timestamp;
    if (d->position > 0) {
        d->play_time = d->position;
    }

    // musicbrainz data
    memcpy(d->mb_track_id, p->metadata.mb_track_id, sizeof(p->metadata.mb_track_id));
    memcpy(d->mb_album_id, p->metadata.mb_album_id, sizeof(p->metadata.mb_album_id));
    memcpy(d->mb_artist_id, p->metadata.mb_artist_id, sizeof(p->metadata.mb_artist_id));
    memcpy(d->mb_album_artist_id, p->metadata.mb_album_artist_id, sizeof(p->metadata.mb_album_artist_id));
    // if this is spotify we add the track_id as the spotify_id
    int spotify_prefix_len = strlen(MPRIS_SPOTIFY_TRACK_ID_PREFIX);
    if (strncmp(p->metadata.track_id, MPRIS_SPOTIFY_TRACK_ID_PREFIX, spotify_prefix_len) == 0){
        memcpy(d->mb_spotify_id, p->metadata.track_id + spotify_prefix_len, sizeof(p->metadata.track_id)-spotify_prefix_len);
    }
    if (now_playing_is_valid(d)) {
        _trace("scrobbler::loaded_scrobble(%p)", d);
        _trace("  scrobble::title: %s", d->title);
        print_array(d->artist, array_count(d->artist), log_tracing, "  scrobble::artist");
        _trace("  scrobble::album: %s", d->album);
        _trace("  scrobble::length: %lu", d->length);
        _trace("  scrobble::position: %.2f", d->position);
        _trace("  scrobble::scrobbled: %s", d->scrobbled ? "yes" : "no");
        _trace("  scrobble::track_number: %u", d->track_number);
        _trace("  scrobble::start_time: %lu", d->start_time);
        _trace("  scrobble::play_time: %.2f", d->play_time);
        if (strlen(d->mb_track_id[0]) > 0) {
            _trace("  scrobble::musicbrainz::track_id: %s", d->mb_track_id[0]);
        }
        if (strlen(d->mb_artist_id[0]) > 0) {
            _trace("  scrobble::musicbrainz::artist_id: %s", d->mb_artist_id[0]);
        }
        if (strlen(d->mb_album_id[0]) > 0) {
            _trace("  scrobble::musicbrainz::album_id: %s", d->mb_album_id[0]);
        }
        if (strlen(d->mb_album_artist_id[0]) > 0) {
            _trace("  scrobble::musicbrainz::album_artist_id: %s", d->mb_album_artist_id[0]);
        }
        if (strlen(d->mb_spotify_id) > 0) {
            _trace("  scrobble::musicbrainz::spotify_id: %s", d->mb_spotify_id);
        }
    }
    return true;
}

bool scrobbles_append(struct mpris_player *player, const struct scrobble *track)
{
    if (NULL == player) { return false; }
    if (NULL == track) { return false; }

    bool result = false;

    struct scrobble *n = scrobble_new();
    scrobble_copy(n, track);

    // TODO(marius) this looks very fishy, usually current and properties are equal
    int queue_count = arrlen(player->queue);
    int one_past = arrlen(player->queue);
    assert(queue_count == one_past);
    if (queue_count > 0) {
        struct scrobble *current = player->queue[one_past-1];
        _debug("scrobbler::queue_top[%u]: %p", queue_count, current);
        if (scrobbles_equal(current, n)) {
            _debug("scrobbler::queue:skipping existing scrobble(%p): %s//%s//%s", n, n->title, n->artist[0], n->album);
            scrobble_free(n);
            goto _exit;
        }
        time_t now = time(0);
        current->play_time += difftime(now, current->start_time);
        if (current->position != 0) {
            current->play_time += current->position;
        }
        _debug("scrobbler::queue:setting_top_scrobble_playtime(%p:%zf): %s//%s//%s", current, current->play_time, current->title, current->artist[0], current->album);
    }

    arrput(player->queue, n);
    _debug("scrobbler::queue_push_scrobble(%p//%-4u) %s//%s//%s", n, queue_count, n->title, n->artist[0], n->album);
    _trace("scrobbler::new_queue_length: %zu", arrlen(player->queue));
#if 0
    _trace2("scrobbler::copied_current:(%p::%p)", player->properties, player->current);
#endif

    result = true;

_exit:
    mpris_properties_zero(&player->properties, true);

    return result;
}

size_t scrobbles_consume_queue(struct scrobbler *scrobbler, struct scrobble **inc_tracks)
{
    if (NULL == scrobbler) { return 0; }

    int queue_length = arrlen(inc_tracks);
    if (queue_length == 0) { return 0; }

    _trace("scrobbler::queue_length: %u", queue_length);

    size_t consumed = 0;

    struct scrobble **tracks = NULL;
    for (int pos = 0; pos < queue_length; pos++) {
        struct scrobble *current = inc_tracks[pos];
        if (scrobble_is_valid(current)) {
            _info("scrobbler::scrobble::valid:(%p//%zu) %s//%s//%s", current, pos, current->title, current->artist[0], current->album);
            arrput(tracks, current);
            current->scrobbled = true;
        } else {
            _warn("scrobbler::scrobble::invalid:(%p//%zu) %s//%s//%s", current, pos, current->title, current->artist[0], current->album);
        }
    }
    api_request_do(scrobbler, (const struct scrobble**)tracks, api_build_request_scrobble);
    arrfree(tracks);

    return consumed;
}

void scrobble_payload_free(struct scrobble_payload *);
void now_playing_payload_free(struct now_playing_payload *);
static bool add_event_now_playing(struct mpris_player *, struct scrobble *);
static bool add_event_scrobble(struct mpris_player *, struct scrobble *);
static void mpris_event_clear(struct mpris_event *);
void state_loaded_properties(DBusConnection *conn, struct mpris_player *player, struct mpris_properties *properties, const struct mpris_event *what_happened)
{
    if (NULL == properties) {
        goto _exit;
    }
    if (!mpris_event_happened(what_happened)) {
        _debug("events::loaded_properties: nothing found");
        goto _exit;
    }

    if (mpris_properties_equals(player->current, properties)) {
        _debug("events::invalid_mpris_properties[%p::%p]: already loaded", player->current, properties);
        goto _exit;
    }

    if (what_happened->player_state == 0) {
        return;
    }

    debug_event(what_happened);
    struct scrobble scrobble = {0};
    if (!load_scrobble(&scrobble, properties)) {
        // TODO(marius) add fallback dbus call to load properties
        get_mpris_properties(conn, player);
        if (!load_scrobble(&scrobble, &player->properties)) {
            _warn("events::unable_to_load_scrobble[%p]", &scrobble);
        }
    }
    if (!now_playing_is_valid(&scrobble)) {
        _warn("events::invalid_now_playing[%p]", &scrobble);
        return;
    }
    mpris_properties_copy(player->current, properties);

    if (NULL != player->events.now_playing_payload) {
        now_playing_payload_free(player->events.now_playing_payload);
    }
    if (NULL != player->events.scrobble_payload) {
        scrobble_payload_free(player->events.scrobble_payload);
    }

    bool scrobble_added = false;
    bool now_playing_added = false;
    if(what_happened->playback_status_changed && !what_happened->track_changed) {
        if (what_happened->player_state == playing) {
            now_playing_added = add_event_now_playing(player, &scrobble);
            scrobble_added = scrobbles_append(player, &scrobble);
        }
    }
    if(what_happened->track_changed) {
        if (what_happened->player_state == playing) {
            if (!now_playing_added) {
                add_event_now_playing(player, &scrobble);
            }
            if (!scrobble_added) {
                scrobbles_append(player, &scrobble);
            }
            add_event_scrobble(player, &scrobble);
        }
    }
    if (what_happened->volume_changed) {
        // trigger volume_changed event
    }
    if (what_happened->position_changed) {
        // trigger position event
    }

_exit:
    mpris_event_clear(&player->changed);
    mpris_properties_zero(&player->properties, true);
}

struct scrobbler *scrobbler_new(void);
struct events *events_new(void);
void events_init(struct events*, struct state*);
void scrobbler_init(struct scrobbler*, struct configuration*, struct events*);
bool state_init(struct state *s, struct configuration *config)
{
    _trace2("mem::initing_state(%p)", s);
    if (NULL == config) { return false; }

    s->scrobbler = scrobbler_new();
    if (NULL == s->scrobbler) { return false; }

    s->config = config;

    events_init(&s->events, s);

    s->dbus = dbus_connection_init(s);
    if (NULL == s->dbus) { return false; }

    if (NULL == s->events.base) { return false; }
    scrobbler_init(s->scrobbler, s->config, &s->events);

    s->player_count = mpris_players_init(s->dbus, s->players, &s->events, s->scrobbler,
        s->config->ignore_players, s->config->ignore_players_count);

    _trace2("mem::inited_state(%p)", s);
    return true;
}

#endif // MPRIS_SCROBBLER_SCROBBLE_H
