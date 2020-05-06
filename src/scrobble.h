/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SCROBBLE_H
#define MPRIS_SCROBBLER_SCROBBLE_H

#include <assert.h>
#include <time.h>

#define NOW_PLAYING_DELAY 65.0L //seconds
#define MIN_TRACK_LENGTH  30.0F // seconds
#define MPRIS_SPOTIFY_TRACK_ID_PREFIX                          "spotify:track:"

int load_player_namespaces(DBusConnection *, struct mpris_player *, int);
void load_player_mpris_properties(DBusConnection*, struct mpris_player*);

struct dbus *dbus_connection_init(struct state*);

static inline bool mpris_event_changed_playback_status(const struct mpris_event *);
static inline bool mpris_event_changed_track(const struct mpris_event *);
static inline bool mpris_event_changed_volume(const struct mpris_event *);
static inline bool mpris_event_changed_position(const struct mpris_event *);

static void debug_event(const struct mpris_event *e)
{
    enum log_levels level = log_debug;
    _log(log_tracing2, "scrobbler::player:                           %7s", e->sender_bus_id);
    _log(level, "changed::volume:          %7s", _to_bool(mpris_event_changed_volume(e)));
    _log(level, "changed::position:        %7s", _to_bool(mpris_event_changed_position(e)));
    _log(level, "changed::playback_status: %7s", _to_bool(mpris_event_changed_playback_status(e)));
    _log(level, "changed::track:           %7s", _to_bool(mpris_event_changed_track(e)));

#ifdef DEBUG
    unsigned whats_loaded = e->loaded_state;
    level = level << 2U;
    if (whats_loaded & mpris_load_property_can_control) {
        _log(level, "changed::can_control:              %s", "yes");
    }
    if (whats_loaded & mpris_load_property_can_go_next) {
        _log(level, "changed::can_go_next:              %s", "yes");
    }
    if (whats_loaded & mpris_load_property_can_go_previous) {
        _log(level, "changed::can_go_previous:          %s", "yes");
    }
    if (whats_loaded & mpris_load_property_can_pause) {
        _log(level, "changed::can_pause:                %s", "yes");
    }
    if (whats_loaded & mpris_load_property_can_play) {
        _log(level, "changed::can_play:                 %s", "yes");
    }
    if (whats_loaded & mpris_load_property_can_seek) {
        _log(level, "changed::can_seek:                 %s", "yes");
    }
    if (whats_loaded & mpris_load_property_loop_status) {
        _log(level, "changed::loop_status:              %s", "yes");
    }
    if (whats_loaded & mpris_load_property_shuffle) {
        _log(level, "changed::shuffle:                  %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_bitrate) {
        _log(level, "changed::metadata::bitrate:        %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_art_url) {
        _log(level, "changed::metadata::art_url:        %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_length) {
        _log(level, "changed::metadata::length:         %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_track_id) {
        _log(level, "changed::metadata::track_id:       %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_album) {
        _log(level, "changed::metadata::album:          %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_album_artist) {
        _log(level, "changed::metadata::album_artist:   %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_artist) {
        _log(level, "changed::metadata::artist:         %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_comment) {
        _log(level, "changed::metadata::comment:        %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_title) {
        _log(level, "changed::metadata::title:          %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_track_number) {
        _log(level, "changed::metadata::track_number:   %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_url) {
        _log(level, "changed::metadata::url:            %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_genre) {
        _log(level, "changed::metadata::genre:          %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_mb_track_id) {
        _log(level, "changed::metadata::mb_track_id:        %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_mb_album_id) {
        _log(level, "changed::metadata::mb_album_id:        %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_mb_artist_id) {
        _log(level, "changed::metadata::mb_artist_id:       %s", "yes");
    }
    if (whats_loaded & mpris_load_metadata_mb_album_artist_id) {
        _log(level, "changed::metadata::mb_album_artist_id: %s", "yes");
    }
#endif
}

static inline bool mpris_event_happened(const struct mpris_event *what_happened)
{
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
    return result - s->play_time + 1;
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

static void scrobble_init(struct scrobble*);
static void mpris_player_free(struct mpris_player *player)
{
    if (NULL == player) { return; }

    if (NULL != player->history) {
        int hist_size = arrlen(player->history);
        for(int i = 0; i < hist_size; i++) {
            free(player->history[i]);
        }
    }
    if (NULL != player->now_playing.event) {
        event_free(player->now_playing.event);
    }
    if (NULL != player->queue.event) {
        event_free(player->queue.event);
    }
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
    if (strlen(identity) == 0) {
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
    player->evbase = events.base;
    player->now_playing.event = NULL;
    player->queue.event = NULL;
    player->now_playing.parent = player;
    player->queue.parent = player;

    // FIXME(marius): this somehow seems to prevent open/close events from propagating
    load_player_mpris_properties(dbus->conn, player);
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
    time_t now = time(0);
    double d = difftime(now, s->start_time);

    _log(log, "scrobbler::loaded_scrobble(%p)", d);
    _log(log, "  scrobble::title: %s", s->title);
    print_array(s->artist, array_count(s->artist), log, "  scrobble::artist");
    _log(log, "  scrobble::album: %s", s->album);
    _log(log, "  scrobble::length: %lu", s->length);
    _log(log, "  scrobble::position: %.2f", s->position);
    _log(log, "  scrobble::scrobbled: %s", s->scrobbled ? "yes" : "no");
    _log(log, "  scrobble::track_number: %u", s->track_number);
    _log(log, "  scrobble::start_time: %lu", s->start_time);
    _log(log, "  scrobble::play_time[%.3lf]: %.3lf", d, s->play_time);
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
    const struct scrobble z = {0};
    return memcmp(s, &z, sizeof(z)) == 0;
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
    return result;
}

bool now_playing_is_valid(const struct scrobble *m/*, const time_t current_time, const time_t last_playing_time*/) {
    assert (NULL != m);

    if (array_count(m->artist) == 0 || NULL == m->artist[0]) { return false; }
    bool result = (
        strlen(m->title) > 0 &&
        strlen(m->artist[0]) > 0 &&
        strlen(m->album) > 0 &&
//        last_playing_time > 0 &&
//        difftime(current_time, last_playing_time) >= LASTFM_NOW_PLAYING_DELAY &&
        m->length > 0.0
    );

    return result;
}

static void scrobble_copy (struct scrobble *t, const struct scrobble *s)
{
    assert(NULL != t);
    assert(NULL != s);
    memcpy(t, s, sizeof(*t));
    _trace2("scrobbler::copied_scrobble: %p->%p", s, t);
}

static bool scrobbles_equal(const struct scrobble *s, const struct scrobble *p)
{
    if ((NULL == s) && (NULL == p)) { return true; }

    assert(NULL != s);
    assert(NULL != p);

    if (s == p) { return true; }

    bool result = (memcmp(s, p, sizeof(*s)) == 0);
    _trace("scrobbler::check_scrobbles(%p:%p) %s", s, p, result ? "same" : "different");
    return result;
}

bool load_scrobble(struct scrobble *d, const struct mpris_properties *p)
{
    assert (NULL != d);
    assert (NULL != p);

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
    memcpy(d->mb_track_id, p->metadata.mb_track_id, sizeof(d->mb_track_id));
    memcpy(d->mb_album_id, p->metadata.mb_album_id, sizeof(d->mb_album_id));
    memcpy(d->mb_artist_id, p->metadata.mb_artist_id, sizeof(d->mb_artist_id));
    memcpy(d->mb_album_artist_id, p->metadata.mb_album_artist_id, sizeof(d->mb_album_artist_id));
    // if this is spotify we add the track_id as the spotify_id
    int spotify_prefix_len = strlen(MPRIS_SPOTIFY_TRACK_ID_PREFIX);
    if (strncmp(p->metadata.track_id, MPRIS_SPOTIFY_TRACK_ID_PREFIX, spotify_prefix_len) == 0){
        memcpy(d->mb_spotify_id, p->metadata.track_id + spotify_prefix_len, sizeof(p->metadata.track_id)-spotify_prefix_len);
    }
    return true;
}

bool scrobbles_append(struct scrobbler *scrobbler, const struct scrobble *track)
{
    assert(NULL != scrobbler);
    assert(NULL != track);

    int queue_length = scrobbler->queue_length;

    struct scrobble *top = &scrobbler->queue[queue_length];
    scrobble_copy(top, track);

    top->play_time = difftime(time(0), top->start_time);
#if 0
    if (top->play_time == 0) {
        // TODO(marius): we need to be able to load the current playing mpris_properties from the track
        //  This would help with computing current play time based on position
    }
    _debug("scrobbler::queue:setting_top_scrobble_playtime(%.3f): %s//%s//%s", top->play_time, top->title, top->artist[0], top->album);
#endif

    scrobbler->queue_length++;

    _debug("scrobbler::queue_push(%-4zu) %s//%s//%s", queue_length, track->title, track->artist[0], track->album);
    for (int pos = scrobbler->queue_length-2; pos >= 0; pos--) {
        struct scrobble *current = &scrobbler->queue[pos];
        if (scrobble_is_empty (current)) {
            continue;
        }
        _debug("scrobbler::%5svalid(%-4zu) %s//%s//%s", scrobble_is_valid(current) ? "" : "in", pos, current->title, current->artist[0], current->album);
    }
    _trace("scrobbler::new_queue_length: %zu", scrobbler->queue_length);

    return true;
}

size_t scrobbles_consume_queue(struct scrobbler *scrobbler)
{
    assert (NULL != scrobbler);

    int queue_length = scrobbler->queue_length;
    _trace("scrobbler::queue_length: %u", queue_length);

    size_t consumed = 0;
    int top_pos = scrobbler->queue_length - 1;
    bool top_scrobble_invalid = false;

    struct scrobble *tracks[queue_length];
    for (int pos = top_pos; pos >= 0; pos--) {
        struct scrobble *current = &scrobbler->queue[pos];
        bool valid = scrobble_is_valid(current);

        _trace("scrobbler::scrobble::%svalid:(%p//%zu) %s//%s//%s", (valid ? "" : "in"), current, pos, current->title, current->artist[0], current->album);
        if (valid) {
            tracks[pos] = current;
            current->scrobbled = true;
            consumed++;
        } else if (pos == top_pos) {
            top_scrobble_invalid = true;
            // skip memory zeroing for top scrobble
            continue;
        }
        memset(&scrobbler->queue[pos], 0x0, sizeof(*current));
    }
    if (consumed > 0) {
        api_request_do(scrobbler, (const struct scrobble**)tracks, consumed, api_build_request_scrobble);
    }
    if (top_scrobble_invalid) {
        struct scrobble *first = &scrobbler->queue[0];
        struct scrobble *top = &scrobbler->queue[top_pos];
        // leave the former top scrobble (which might still be playing) as the only one in the queue
        memcpy(first, top, sizeof(*first));
        memset(top, 0x0, sizeof(*top));
    }

    return consumed;
}

static bool add_event_now_playing(struct mpris_player *, struct scrobble *, time_t);
//static bool add_event_scrobble(struct mpris_player *, struct scrobble *);
static bool add_event_queue(struct mpris_player*, struct scrobble*, struct event_base*);
static void mpris_event_clear(struct mpris_event *);
static void print_properties_if_changed(struct mpris_properties*, const struct mpris_properties*, struct mpris_event*, enum log_levels);
void state_loaded_properties(DBusConnection *conn, struct mpris_player *player, struct mpris_properties *properties, const struct mpris_event *what_happened)
{
    assert(conn);
    assert(player);
    assert(properties);

    if (!mpris_event_happened(what_happened)) {
        _trace2("events::skipping: nothing happened");
        return;
    }
    debug_event(&player->changed);

    struct scrobble scrobble = {0};
    load_scrobble(&scrobble, properties);

    if (scrobble_is_empty(&scrobble)) {
        _warn("events::invalid_scrobble");
        return;
    }

    if (mpris_player_is_playing(player)) {
        if(mpris_event_changed_track(what_happened) || mpris_event_changed_playback_status(what_happened)) {
            add_event_now_playing(player, &scrobble, 0);
            add_event_queue(player, &scrobble, player->evbase);
        }
    } else {
        // remove add_now_event
        // compute current play_time for properties.metadata
        if (NULL != player->now_playing.event) {
            _trace("events::removing::now_loading");
            event_del(player->now_playing.event);
        }
        if (NULL != player->queue.event) {
            _trace("events::removing::queue");
            event_del(player->queue.event);
        }
    }
    if (mpris_event_changed_volume(what_happened)) {
        // trigger volume_changed event
    }
    if (mpris_event_changed_position(what_happened)) {
        // trigger position event
        // compute current play_time for properties.metadata
    }

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
