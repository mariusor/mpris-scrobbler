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

static inline bool mpris_event_changed_playback_status(const struct mpris_event *);
static inline bool mpris_event_changed_track(const struct mpris_event *);
static inline bool mpris_event_changed_volume(const struct mpris_event *);
static inline bool mpris_event_changed_position(const struct mpris_event *);

static void debug_event(const struct mpris_event *e)
{
    enum log_levels level = log_debug;
    unsigned whats_loaded = e->loaded_state;
    _log(log_tracing2, "scrobbler::player:                           %7s", e->sender_bus_id);
    _log(level, "scrobbler::checking_volume_changed:          %7s", _to_bool(mpris_event_changed_volume(e)));
    _log(level, "scrobbler::checking_position_changed:        %7s", _to_bool(mpris_event_changed_position(e)));
    _log(level, "scrobbler::checking_playback_status_changed: %7s", _to_bool(mpris_event_changed_playback_status(e)));
    _log(level, "scrobbler::checking_track_changed:           %7s", _to_bool(mpris_event_changed_track(e)));

#ifdef DEBUG
    if (whats_loaded & mpris_load_property_can_control) {
        _log(level, "scrobbler::checking_can_control_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_property_can_go_next) {
        _log(level, "scrobbler::checking_can_go_next_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_property_can_go_previous) {
        _log(level, "scrobbler::checking_can_go_previous_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_property_can_pause) {
        _log(level, "scrobbler::checking_can_pause_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_property_can_play) {
        _log(level, "scrobbler::checking_can_play_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_property_can_seek) {
        _log(level, "scrobbler::checking_can_seek_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_property_loop_status) {
        _log(level, "scrobbler::checking_loop_status_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_property_shuffle) {
        _log(level, "scrobbler::checking_shuffle_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_bitrate) {
        _log(level, "scrobbler::checking_metadata::bitrate_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_art_url) {
        _log(level, "scrobbler::checking_metadata::art_url_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_length) {
        _log(level, "scrobbler::checking_metadata::length_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_track_id) {
        _log(level, "scrobbler::checking_metadata::track_id_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_album) {
        _log(level, "scrobbler::checking_metadata::album_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_album_artist) {
        _log(level, "scrobbler::checking_metadata::album_artist_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_artist) {
        _log(level, "scrobbler::checking_metadata::artist_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_comment) {
        _log(level, "scrobbler::checking_metadata::comment_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_title) {
        _log(level, "scrobbler::checking_metadata::title_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_track_number) {
        _log(level, "scrobbler::checking_metadata::track_number_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_url) {
        _log(level, "scrobbler::checking_metadata::url_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_genre) {
        _log(level, "scrobbler::checking_metadata::genre_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_mb_track_id) {
        _log(level, "scrobbler::checking_metadata::musicbrainz_track_id_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_mb_album_id) {
        _log(level, "scrobbler::checking_metadata::musicbrainz_album_id_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_mb_artist_id) {
        _log(level, "scrobbler::checking_metadata::musicbrainz_artist_id_changed: %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_mb_album_artist_id) {
        _log(level, "scrobbler::checking_metadata::musicbrainz_album_artist_id_changed: %s", "yes");
    }
#endif
}

static inline bool mpris_event_happened(const struct mpris_event *what_happened)
{
#if 0
    return (
        what_happened->playback_status_changed ||
        what_happened->volume_changed ||
        what_happened->track_changed ||
        what_happened->position_changed
    );
#endif
    return what_happened->loaded_state >= (unsigned)mpris_load_property_position;
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
    if (s->length > 0 && s->length / 2.0 < result) {
        result = s->length / 2.0;
    }
    return result - s->play_time;
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

    if (NULL != player->history) {
        int hist_size = arrlen(player->history);
        for(int i = 0; i < hist_size; i++) {
            free(player->history[i]);
        }
    }
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

void state_loaded_properties(DBusConnection *, struct mpris_player *, struct mpris_properties *, const struct mpris_event *);
static void get_player_identity(DBusConnection*, const char*, char*);
static int mpris_player_init (struct dbus *dbus, struct mpris_player *player, struct events events, struct scrobbler *scrobbler, const char ignored[MAX_PLAYERS][MAX_PROPERTY_LENGTH], int ignored_count)
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
    player->events.base = events.base;
    player->events.now_playing_payload = NULL;

    // FIXME(marius): this somehow seems to prevent open/close events from propagating
    get_mpris_properties(dbus->conn, player);
    state_loaded_properties(dbus->conn, player, &player->properties, &player->changed);
    return 1;
}

static int mpris_players_init(struct dbus *dbus, struct mpris_player *players, struct events events, struct scrobbler *scrobbler, const char ignored[MAX_PLAYERS][MAX_PROPERTY_LENGTH], int ignored_count)
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
        _debug("mpris_player::namespace[%d]: %s %s", i, player.mpris_name, player.bus_id);
    }

    return loaded_player_count;
}

static void print_scrobble(const struct scrobble *s, enum log_levels log)
{
    double d = 0;
    if (s->play_time > 0) {
        d = s->play_time;
    } else if (s->start_time > 0) {
        time_t now = time(0);
        d = difftime(now, s->start_time) + 1lu;
    }

    _log(log, "scrobbler::loaded_scrobble(%p)", d);
    _log(log, "  scrobble::title: %s", s->title);
    print_array(s->artist, array_count(s->artist), log, "  scrobble::artist");
    _log(log, "  scrobble::album: %s", s->album);
    _log(log, "  scrobble::length: %lu", s->length);
    _log(log, "  scrobble::position: %.2f", s->position);
    _log(log, "  scrobble::scrobbled: %s", s->scrobbled ? "yes" : "no");
    _log(log, "  scrobble::track_number: %u", s->track_number);
    _log(log, "  scrobble::start_time: %lu", s->start_time);
    _log(log, "  scrobble::play_time: %.3lf", d);
    if (strlen(s->mb_track_id[0]) > 0) {
        print_array(s->mb_track_id, array_count(s->mb_track_id), log, "  scrobble::mb_track_id");
    }
    if (strlen(s->mb_artist_id[0]) > 0) {
        print_array(s->mb_artist_id, array_count(s->mb_artist_id), log, "  scrobble::mb_artist_id");
    }
    if (strlen(s->mb_album_id[0]) > 0) {
        print_array(s->mb_album_id, array_count(s->mb_album_id), log, "  scrobble::mb_album_id");
    }
    if (strlen(s->mb_album_artist_id[0]) > 0) {
        print_array(s->mb_album_artist_id, array_count(s->mb_album_artist_id), log, "  scrobble::mb_album_artist_id");
    }
    if (strlen(s->mb_spotify_id) > 0) {
        _log(log, "  scrobble::musicbrainz::spotify_id: %s", s->mb_spotify_id);
    }
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
    _log(log, "scrobble::valid::play_time[%.3lf:%.3lf]: %s", d, scrobble_interval, d >= scrobble_interval ? "yes" : "no");
    if (NULL != s->artist[0]) {
        _log(log, "scrobble::valid::artist[%s]: %s", s->artist[0], strlen(s->artist[0]) > 0 ? "yes" : "no");
    }
    _log(log, "scrobble::valid::scrobbled: %s", !s->scrobbled ? "yes" : "no");
}

static bool scrobble_is_empty(const struct scrobble *s)
{
    return (strlen(s->artist[0]) == 0 && strlen(s->album) == 0 && strlen(s->title) == 0);
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

#if 0
    if (strlen(p->metadata.title) == 0) {
        _trace2("load_scrobble::invalid_source_metadata_title");
        return false;
    }
#endif

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
    if (!scrobble_is_empty(d)) {
        print_scrobble(d, log_tracing);
    }
    return true;
}

bool scrobbles_append(struct scrobbler *scrobbler, const struct scrobble *track)
{
    if (NULL == scrobbler) { return false; }
    if (NULL == track) { return false; }

    bool result = false;

    struct scrobble *n = scrobble_new();
    scrobble_copy(n, track);

    // TODO(marius) this looks very fishy, usually current and properties are equal
    int queue_count = arrlen(scrobbler->queue);
    int one_past = arrlen(scrobbler->queue);
    assert(queue_count == one_past);
    if (queue_count > 0) {
        struct scrobble *current = scrobbler->queue[one_past-1];
        _debug("scrobbler::queue_top[%u]: %p", queue_count, current);
        if (scrobbles_equal(current, n)) {
            _debug("scrobbler::queue:skipping existing scrobble(%p): %s//%s//%s", n, n->title, n->artist[0], n->album);
            scrobble_free(n);
            return result;
        }
        time_t now = time(0);
        current->play_time += difftime(now, current->start_time);
        if (current->position != 0) {
            current->play_time += current->position;
        }
        _debug("scrobbler::queue:setting_top_scrobble_playtime(%p:%.3f): %s//%s//%s", current, current->play_time, current->title, current->artist[0], current->album);
    }

    arrput(scrobbler->queue, n);
    _debug("scrobbler::queue_push_scrobble(%p//%-4u) %s//%s//%s", n, queue_count, n->title, n->artist[0], n->album);
    _trace("scrobbler::new_queue_length: %zu", arrlen(scrobbler->queue));

    result = true;

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
static bool mpris_player_is_playing (struct mpris_player *player)
{
    return memcmp(player->properties.playback_status, "Playing", 8) == 0;
}

void add_to_queue_payload_free(struct add_to_queue_payload *);
void now_playing_payload_free(struct now_playing_payload *);
static bool add_event_now_playing(struct mpris_player *, struct scrobble *);
//static bool add_event_scrobble(struct mpris_player *, struct scrobble *);
static bool add_event_add_to_queue(struct scrobbler*, struct scrobble*, struct event_base*);
static void mpris_event_clear(struct mpris_event *);
static void print_properties_if_changed(struct mpris_properties*, const struct mpris_properties*, struct mpris_event*, enum log_levels);
void state_loaded_properties(DBusConnection *conn, struct mpris_player *player, struct mpris_properties *properties, const struct mpris_event *what_happened)
{
    assert(conn);
    assert(player);
    assert(properties);

    if (!mpris_event_happened(what_happened)) {
        _debug("events::skipping: nothing happened");
        return;
    }

    struct scrobble scrobble = {0};
    if (!load_scrobble(&scrobble, properties)) {
        // TODO(marius) add fallback dbus call to load properties
        get_mpris_properties(conn, player);
        if (!load_scrobble(&scrobble, &player->properties)) {
            _warn("events::unable_to_load_scrobble[%p]", &scrobble);
        }
    }

    if (scrobble_is_empty(&scrobble)) {
        _warn("events::invalid_scrobble[%p]", &scrobble);
        print_scrobble(&scrobble, log_error);
        return;
    }

    if(mpris_event_changed_track(what_happened)) {
        if (mpris_player_is_playing(player)) {
            add_event_now_playing(player, &scrobble);
            add_event_add_to_queue(player->scrobbler, &scrobble, player->events.base);
            //add_event_scrobble(player, &scrobble);
        }
    }
    if(mpris_event_changed_playback_status(what_happened) && !mpris_event_changed_track(what_happened)) {
        if (mpris_player_is_playing(player)) {
            add_event_now_playing(player, &scrobble);
            //add_event_add_to_queue(player->scrobbler, &scrobble, player->events.base);
        } else {
            // remove add_now_event
            // compute current play_time for properties.metadata
        }
    }
    if (mpris_event_changed_volume(what_happened)) {
        // trigger volume_changed event
    }
    if (mpris_event_changed_position(what_happened)) {
        // trigger position event
        // compute current play_time for properties.metadata
    }

    debug_event(&player->changed);
    mpris_event_clear(&player->changed);
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

    s->player_count = mpris_players_init(s->dbus, s->players, s->events, s->scrobbler,
        s->config->ignore_players, s->config->ignore_players_count);

    _trace2("mem::inited_state(%p)", s);
    return true;
}

#endif // MPRIS_SCROBBLER_SCROBBLE_H
