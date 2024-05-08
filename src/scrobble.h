/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SCROBBLE_H
#define MPRIS_SCROBBLER_SCROBBLE_H

#include <assert.h>
#include <time.h>

#define NOW_PLAYING_DELAY 65.0L //seconds
#define MIN_TRACK_LENGTH  30.0L // seconds
#define MPRIS_SPOTIFY_TRACK_ID_PREFIX                          "spotify:track:"

short load_player_namespaces(DBusConnection *, struct mpris_player *, short);
void load_player_mpris_properties(DBusConnection*, struct mpris_player*);

struct dbus *dbus_connection_init(struct state*);

static void debug_event(const struct mpris_event *e)
{
    enum log_levels level = log_debug;
    _log(log_tracing2, "scrobbler::player:                           %7s", e->sender_bus_id);
    _log(log_tracing2, "change ::at:          %11d", e->timestamp);
    _log(level, "changed::volume:          %7s", _to_bool(mpris_event_changed_volume(e)));
    _log(level, "changed::position:        %7s", _to_bool(mpris_event_changed_position(e)));
    _log(level, "changed::playback_status: %7s", _to_bool(mpris_event_changed_playback_status(e)));
    _log(level, "changed::track:           %7s", _to_bool(mpris_event_changed_track(e)));

#ifdef DEBUG
    const unsigned whats_loaded = (unsigned)e->loaded_state;
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

static double min_scrobble_delay_seconds(const struct scrobble *s)
{
    if (s->length <= 0.1L) {
        return 0L;
    }
    const double result = (double)min(MIN_SCROBBLE_DELAY_SECONDS, s->length / 2L) - s->play_time + 1L;
    return max(result, 0L);
}

static void scrobble_init(struct scrobble *s)
{
    if (NULL == s) { return; }
    memset(s, 0, sizeof(*s));
    time(&s->start_time);
    _trace2("mem::inited_scrobble(%p)", s);
}

static struct scrobble *scrobble_new(void)
{
    struct scrobble *result = malloc(sizeof(struct scrobble));
    scrobble_init(result);

    return result;
}

static void mpris_player_free(struct mpris_player *player)
{
    if (NULL == player) { return; }

    if (NULL != player->history) {
        const size_t hist_size = arrlen(player->history);
        for(size_t i = 0; i < hist_size; i++) {
            free(player->history[i]);
        }
    }
    if (event_initialized(&player->now_playing.event) && event_pending(&player->now_playing.event, EV_TIMEOUT, NULL)) {
        event_del(&player->now_playing.event);
    }
    if (event_initialized(&player->queue.event) && event_pending(&player->queue.event, EV_TIMEOUT, NULL)) {
        event_del(&player->queue.event);
    }
    memset(player, 0x0, sizeof(*player));
}

void dbus_close(struct state*);
void events_free(const struct events*);
static void state_destroy(struct state *s)
{
    if (NULL != s->dbus) { dbus_close(s); }
    for (int i = 0; i < s->player_count; i++) {
        mpris_player_free(&s->players[i]);
    }

    curl_multi_cleanup(s->scrobbler.handle);
    scrobbler_clean(&s->scrobbler);
    events_free(&s->events);
}

static struct mpris_player *mpris_player_new(void)
{
    struct mpris_player *result = calloc(1, sizeof(struct mpris_player));

    return (result);
}

void state_loaded_properties(const DBusConnection *, struct mpris_player *, struct mpris_properties *, const struct mpris_event *);
void get_player_identity(DBusConnection*, const char*, char*);
static bool mpris_player_init (const struct dbus *dbus, struct mpris_player *player, const struct events events, struct scrobbler *scrobbler, const char ignored[MAX_PLAYERS][MAX_PROPERTY_LENGTH+1], const short ignored_count)
{
    if (strlen(player->mpris_name) == 0 || strlen(player->bus_id) == 0) {
        return false;
    }
    const char *identity = player->mpris_name;
    if (strlen(identity) == 0) {
        identity = player->bus_id;
    }
    get_player_identity(dbus->conn, identity, player->name);

    for (short j = 0; j < ignored_count; j++) {
        char *ignored_id = (char*)ignored[j];
        const size_t len = strlen(ignored_id);
        player->ignored = (
            strncmp(player->mpris_name, ignored_id, len) == 0 ||
            strncmp(player->name, ignored_id, len) == 0
        );
        if (player->ignored) {
            _debug("mpris_player::ignored: %s on %s", player->name, ignored_id);
            return true;
        }
    }
    assert(scrobbler);
    player->scrobbler = scrobbler;
    assert(events.base);
    player->evbase = events.base;

    load_player_mpris_properties(dbus->conn, player);

    player->now_playing.parent = player;
    player->queue.parent = player;

    return true;
}

void print_mpris_player(struct mpris_player *, enum log_levels, bool);
static short mpris_players_init(const struct dbus *dbus, struct mpris_player *players, const struct events events, struct scrobbler *scrobbler, const char ignored[MAX_PLAYERS][MAX_PROPERTY_LENGTH+1], const short ignored_count)
{
    if (NULL == players){
        return -1;
    }
    if (NULL == dbus){
        _error("players::init: failed, unable to load from dbus");
        return -1;
    }
    const short player_count = load_player_namespaces(dbus->conn, players, MAX_PLAYERS);
    short loaded_player_count = 0;
    for (short i = 0; i < player_count; i++) {
        struct mpris_player *player = &players[i];
        _trace("mpris_player[%d]: %s%s", i, player->mpris_name, player->bus_id);
        if (!mpris_player_init(dbus, player, events, scrobbler, ignored, ignored_count)) {
            _trace("mpris_player[%d:%s]: failed to load properties", i, player->mpris_name);
            continue;
        }
        if (!player->ignored) {
            print_mpris_player(player, log_tracing2, false);
            loaded_player_count++;
        }
    }

    return loaded_player_count;
}

static void print_scrobble(struct scrobble *s, const enum log_levels log)
{
    const time_t now = time(NULL) + 1;
    double d = 1;
    if (s->start_time > 0) {
        d = difftime(now, s->start_time);
    }

    char start_time[20];
    const struct tm *timeinfo = localtime (&s->start_time);
    strftime(start_time, sizeof(start_time), "%Y-%m-%d %T %p", timeinfo);

    char temp[MAX_PROPERTY_LENGTH*MAX_PROPERTY_COUNT+10] = {0};
    array_log_with_label(temp, s->artist, array_count(s->artist));
    _log(log, "scrobbler::loaded_scrobble(%p)", s);
    _log(log, "  scrobble::title: %s", s->title);
    _log(log, "  scrobble::artist: %s", temp);
    _log(log, "  scrobble::album: %s", s->album);
    _log(log, "  scrobble::length: %lu", s->length);
    _log(log, "  scrobble::position: %.2f", s->position);
    _log(log, "  scrobble::scrobbled: %s", _to_bool(s->scrobbled));
    _log(log, "  scrobble::track_number: %u", s->track_number);
    _log(log, "  scrobble::start_time: %s", start_time);
    _log(log, "  scrobble::play_time[%.3lf]: %.3lf", d, s->play_time);
    if (strlen(s->mb_spotify_id) > 0) {
        _log(log, "  scrobble::spotify_id: %s", s->mb_spotify_id);
    }
    if (strlen(s->mb_track_id[0]) > 0) {
        array_log_with_label(temp, s->mb_track_id, array_count(s->mb_track_id));
        _log(log, "  scrobble::musicbrainz::track_id: %s", temp);
    }
    if (strlen(s->mb_artist_id[0]) > 0) {
        array_log_with_label(temp, s->mb_artist_id, array_count(s->mb_artist_id));
        _log(log, "  scrobble::musicbrainz::artist_id: %s", temp);
    }
    if (strlen(s->mb_album_id[0]) > 0) {
        array_log_with_label(temp, s->mb_album_id, array_count(s->mb_album_id));
        _log(log, "  scrobble::musicbrainz::album_id: %s", temp);
    }
    if (strlen(s->mb_album_artist_id[0]) > 0) {
        array_log_with_label(temp, s->mb_album_artist_id, array_count(s->mb_album_artist_id));
        _log(log, "  scrobble::musicbrainz::album_artist_id: %s", temp);
    }
}

static void print_scrobble_valid_check(const struct scrobble *s, const enum log_levels log)
{
    if (NULL == s) {
        return;
    }
    _log(log, "scrobble::valid::title[%s]: %s", s->title, _to_bool(strlen(s->title) > 0));
    _log(log, "scrobble::valid::album[%s]: %s", s->album, _to_bool(strlen(s->album) > 0));
    _log(log, "scrobble::valid::length[%.2f]: %s", s->length, _to_bool(s->length > MIN_TRACK_LENGTH));
    const double scrobble_interval = min_scrobble_delay_seconds(s);
    double d = 0;
    if (s->play_time > 0) {
        d = s->play_time + 1l;
    } else if (s->start_time > 0) {
        const time_t now = time(0);
        d = difftime(now, s->start_time) + 1l;
    }
    _log(log, "scrobble::valid::play_time[%.3lf:%.3lf]: %s", d, scrobble_interval, _to_bool(d >= scrobble_interval));
    if (!_is_zero(s->artist)) {
        _log(log, "scrobble::valid::artist[%s]: %s", s->artist[0], _to_bool(strlen(s->artist[0]) > 0));
    }
    _log(log, "scrobble::valid::scrobbled: %s", _to_bool(!s->scrobbled));
}

static bool scrobble_is_empty(const struct scrobble *s)
{
    return _is_zero(s);
}

static bool scrobble_is_valid(const struct scrobble *s)
{
    if (NULL == s) { return false; }
    if (_is_zero(s->artist)) { return false; }

    const double scrobble_interval = min_scrobble_delay_seconds(s);
    double d;
    if (s->play_time > 0) {
        d = s->play_time +1lu;
    } else {
        const time_t now = time(0);
        d = difftime(now, s->start_time) + 1lu;
    }

    const bool result = (
        s->length >= (double)MIN_TRACK_LENGTH &&
        d >= scrobble_interval &&
        s->scrobbled == false &&
        strlen(s->title) > 0 &&
        strlen(s->artist[0]) > 0 &&
        strlen(s->album) > 0
    );
    return result;
}

static bool now_playing_is_valid(const struct scrobble *m, const struct api_credentials *cur/*, const time_t current_time, const time_t last_playing_time*/) {
    if (NULL == m) {
        return false;
    }

    switch (cur->end_point) {
        case api_listenbrainz:
            return listenbrainz_now_playing_is_valid(m);
        case api_librefm:
        case api_lastfm:
        case api_unknown:
        default:
            return audioscrobbler_now_playing_is_valid(m);
    }
}

static void scrobble_copy (struct scrobble *t, const struct scrobble *s)
{
    assert(NULL != t);
    assert(NULL != s);
    memcpy(t, s, sizeof(*t));
}

static bool scrobbles_equal(const struct scrobble *s, const struct scrobble *p)
{
    if ((NULL == s) && (NULL == p)) { return true; }

    assert(NULL != s);
    assert(NULL != p);

    if (s == p) { return true; }

    const bool result = _eq(s, p);
    _trace("scrobbler::check_scrobbles(%p:%p) %s", s, p, result ? "same" : "different");
    return result;
}

static bool load_scrobble(struct scrobble *d, const struct mpris_properties *p, const struct mpris_event *e)
{
    assert (NULL != d);
    assert (NULL != p);

    memcpy(d->title, p->metadata.title, sizeof(p->metadata.title));
    memcpy(d->album, p->metadata.album, sizeof(p->metadata.album));
    memcpy(d->artist, p->metadata.artist, sizeof(p->metadata.artist));

    d->length = 0L;
    d->position = 0L;
    if (p->metadata.length > 0) {
        d->length = (double)p->metadata.length / (double)1000000L;
    }
    d->scrobbled = false;
    d->track_number = (unsigned short )p->metadata.track_number;
    if (mpris_event_changed_track(e) && mpris_properties_is_playing(p)) {
        if (mpris_event_changed_position(e)) {
            d->position = (double)p->position / (double)1000000L;
        }
        if (d->position > 0) {
            d->play_time = d->position;
        }
        // we're checking if it's a newly started track, in order to set the start_time accordingly
        if (d->play_time > 0) {
            d->start_time = time(NULL) - (time_t)(d->play_time/1);
        } else if (e->timestamp > 0) {
            d->start_time = e->timestamp;
        }
    }

    // musicbrainz data
    memcpy(d->mb_track_id, p->metadata.mb_track_id, sizeof(d->mb_track_id));
    memcpy(d->mb_album_id, p->metadata.mb_album_id, sizeof(d->mb_album_id));
    memcpy(d->mb_artist_id, p->metadata.mb_artist_id, sizeof(d->mb_artist_id));
    memcpy(d->mb_album_artist_id, p->metadata.mb_album_artist_id, sizeof(d->mb_album_artist_id));
    // if this is spotify we add the track_id as the spotify_id
    const size_t spotify_prefix_len = strlen(MPRIS_SPOTIFY_TRACK_ID_PREFIX);
    if (strncmp(p->metadata.track_id, MPRIS_SPOTIFY_TRACK_ID_PREFIX, spotify_prefix_len) == 0){
        memcpy(d->mb_spotify_id, p->metadata.track_id + spotify_prefix_len, sizeof(p->metadata.track_id)-spotify_prefix_len);
    }
    return true;
}

bool queue_append(struct scrobble_queue *queue, const struct scrobble *track)
{
    const int queue_length = queue->length;

    struct scrobble *top = &queue->entries[queue_length];
    scrobble_copy(top, track);

    top->play_time = difftime(time(0), top->start_time);
#if 0
    if (top->play_time == 0) {
        // TODO(marius): we need to be able to load the current playing mpris_properties from the track
        //  This would help with computing current play time based on position
    }
    _debug("scrobbler::queue:setting_top_scrobble_playtime(%.3f): %s//%s//%s", top->play_time, top->title, top->artist[0], top->album);
#endif

    queue->length++;

    for (int pos = queue->length-2; pos >= 0; pos--) {
        struct scrobble *current = &queue->entries[pos];
        if (!scrobble_is_valid (current)) {
            _debug("scrobbler::   invalid(%4zu) %s//%s//%s", pos, current->title, current->artist[0], current->album);
            continue;
        }
        _trace("scrobbler::     valid(%4zu) %s//%s//%s", pos, current->title, current->artist[0], current->album);
    }

    return true;
}

static bool scrobbles_append(struct scrobbler *scrobbler, const struct scrobble *track)
{
    assert(NULL != scrobbler);
    assert(NULL != track);

    struct scrobble_queue *queue = &scrobbler->queue;
    _trace("scrobbler::queue_push(%4zu) %s//%s//%s", queue->length, track->title, track->artist[0], track->album);
    const bool result = queue_append(queue, track);
    _trace("scrobbler::new_queue_length: %zu", queue->length);
    return result;
}

static unsigned int scrobbler_consume_queue(struct scrobbler *scrobbler)
{
    assert (NULL != scrobbler);

    const int queue_length = scrobbler->queue.length;
    _trace("scrobbler::queue_length: %u", queue_length);

    unsigned int consumed = 0;
    const int top = scrobbler->queue.length - 1;
    bool top_scrobble_invalid = false;

    struct scrobble *tracks[MAX_QUEUE_LENGTH];
    for (int pos = top; pos >= 0; pos--) {
        struct scrobble *current = &scrobbler->queue.entries[pos];
        const bool valid = scrobble_is_valid(current);

        if (valid) {
            tracks[pos] = current;
            current->scrobbled = true;
            _info("scrobbler::scrobble:(%4zu) %s//%s//%s", pos, current->title, current->artist[0], current->album);
            consumed++;
        } else if (pos == top) {
            _trace("scrobbler::scrobble::invalid:(%p//%4zu) %s//%s//%s", current, pos, current->title, current->artist[0], current->album);
            print_scrobble_valid_check(current, log_tracing);
            top_scrobble_invalid = true;
            // skip memory zeroing for top scrobble
            continue;
        }
    }
    if (consumed > 0) {
        api_request_do(scrobbler, (const struct scrobble**)tracks, consumed, api_build_request_scrobble);
    }
    int min_scrobble_zero = 0;
    if (top_scrobble_invalid) {
        struct scrobble *first = &scrobbler->queue.entries[0];
        struct scrobble *last = &scrobbler->queue.entries[top];
        // leave the former top scrobble (which might still be playing) as the only one in the queue
        memcpy(first, last, sizeof(*first));
        memset(last, 0x0, sizeof(*last));
        min_scrobble_zero = 1;
    }
    for (int pos = top; pos >= min_scrobble_zero; pos--) {
        memset(&scrobbler->queue.entries[pos], 0x0, sizeof(scrobbler->queue.entries[pos]));
        scrobbler->queue.length--;
    }

    return consumed;
}

static bool add_event_now_playing(struct mpris_player *, const struct scrobble *, const time_t);
static bool add_event_queue(struct mpris_player*, const struct scrobble*);
static void mpris_event_clear(struct mpris_event *);
static void print_properties_if_changed(struct mpris_properties*, struct mpris_properties*, struct mpris_event*, enum log_levels);
void state_loaded_properties(const DBusConnection *conn, struct mpris_player *player, struct mpris_properties *properties, const struct mpris_event *what_happened)
{
    assert(conn);
    assert(player);
    assert(properties);
    if (player->ignored) {
        _trace("events::skipping: player %s is ignored", player->name);
        return;
    }

    if (!mpris_event_happened(what_happened)) {
        _trace("events::skipping: nothing happened");
        return;
    }
    debug_event(&player->changed);

    struct scrobble scrobble = {0};
    load_scrobble(&scrobble, properties, what_happened);

    if (scrobble_is_empty(&scrobble)) {
        _warn("events::invalid_scrobble");
        return;
    }

    if (mpris_player_is_playing(player)) {
        if(mpris_event_changed_track(what_happened) || mpris_event_changed_playback_status(what_happened)) {
            add_event_now_playing(player, &scrobble, 0);
            add_event_queue(player, &scrobble);
        }
        if (mpris_event_changed_track(what_happened) && !mpris_event_changed_position(what_happened)) {
            properties->position = 0;
        }
    } else {
        // remove add_now_event
        // compute current play_time for properties.metadata
        if (event_initialized(&player->now_playing.event)) {
            _trace("events::removing::now_loading(%p)", &player->now_playing.event);
            event_del(&player->now_playing.event);
            memset(&player->now_playing.event, 0x0, sizeof(player->now_playing.event));
        }
        if (event_initialized(&player->queue.event)) {
            _trace("events::removing::queue(%p)", &player->queue.event);
            event_del(&player->queue.event);
            memset(&player->queue.event, 0x0, sizeof(player->queue.event));
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

static void check_player(struct mpris_player* player)
{
    if (!mpris_player_is_valid(player) || !mpris_player_is_playing(player) || player->ignored) {
        return;
    }
    const struct mpris_event all = {.loaded_state = mpris_load_all };

    struct scrobble scrobble = {0};
    load_scrobble(&scrobble, &player->properties, &all);

    add_event_now_playing(player, &scrobble, 0);
    add_event_queue(player, &scrobble);
}

struct events *events_new(void);
void events_init(struct events*, struct state*);
static bool state_init(struct state *s, struct configuration *config)
{
    _trace2("mem::initing_state(%p)", s);
    if (NULL == config) { return false; }

    s->config = config;

    events_init(&s->events, s);

    s->dbus = dbus_connection_init(s);
    if (NULL == s->dbus) { return false; }

    if (NULL == s->events.base) { return false; }
    scrobbler_init(&s->scrobbler, s->config, s->events.base);

    s->player_count = mpris_players_init(s->dbus, s->players, s->events, &s->scrobbler, s->config->ignore_players, s->config->ignore_players_count);
    for (short i = 0; i < s->player_count; i++) {
        struct mpris_player *player = &s->players[i];
        check_player(player);
    }
    _trace2("mem::loaded %zd players", s->player_count);

    _trace2("mem::inited_state(%p)", s);
    return true;
}

#endif // MPRIS_SCROBBLER_SCROBBLE_H
