/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SMPRIS_H
#define MPRIS_SCROBBLER_SMPRIS_H

#define MPRIS_PLAYBACK_STATUS_PLAYING  "Playing"
#define MPRIS_PLAYBACK_STATUS_PAUSED   "Paused"
#define MPRIS_PLAYBACK_STATUS_STOPPED  "Stopped"

static void mpris_metadata_zero(struct mpris_metadata *metadata)
{
    metadata->track_number = 0;
    metadata->bitrate = 0;
    metadata->disc_number = 0;
    metadata->length = 0;
    metadata->timestamp = 0;

    memset(metadata->genre, 0, sizeof(metadata->genre));
    memset(metadata->artist, 0, sizeof(metadata->artist));
    memset(metadata->album_artist, 0, sizeof(metadata->album_artist));
    memset(metadata->comment, 0, sizeof(metadata->comment));

    memset(metadata->content_created, 0, MAX_PROPERTY_LENGTH);
    memset(metadata->composer, 0, MAX_PROPERTY_LENGTH);
    memset(metadata->album, 0, MAX_PROPERTY_LENGTH);
    memset(metadata->title, 0, MAX_PROPERTY_LENGTH);
    memset(metadata->url, 0, MAX_PROPERTY_LENGTH);
    memset(metadata->art_url, 0, MAX_PROPERTY_LENGTH);
    memset(metadata->track_id, 0, MAX_PROPERTY_LENGTH);
    memset(metadata->mb_track_id, 0, MAX_PROPERTY_LENGTH);
    memset(metadata->mb_artist_id, 0, MAX_PROPERTY_LENGTH);
    memset(metadata->mb_album_id, 0, MAX_PROPERTY_LENGTH);
    memset(metadata->mb_album_artist_id, 0, MAX_PROPERTY_LENGTH);
    _trace2("mem::zeroed_metadata(%p)", metadata);
}

void mpris_properties_zero(struct mpris_properties *properties, bool metadata_too)
{
    properties->volume = 0;
    properties->position = 0;
    if (NULL != properties->player_name) {
        memset(properties->player_name, 0, strlen(properties->player_name));
    }
    if (NULL != properties->loop_status) {
        memset(properties->loop_status, 0, strlen(properties->loop_status));
    }
    if (NULL != properties->playback_status) {
        memset(properties->playback_status, 0, strlen(properties->playback_status));
    }
    properties->can_control = false;
    properties->can_go_next = false;
    properties->can_go_previous = false;
    properties->can_play = false;
    properties->can_pause = false;
    properties->can_seek = false;
    properties->shuffle = false;
    if (metadata_too) { mpris_metadata_zero(&properties->metadata); }
    _trace2("mem::zeroed_properties(%p)", properties);
}

struct mpris_properties *mpris_properties_new(void)
{
    struct mpris_properties *properties = calloc(1, sizeof(struct mpris_properties));
    return properties;
}

void mpris_properties_free(struct mpris_properties *properties)
{
    if (NULL == properties) { return; }
    _trace2("mem::properties::free(%p)", properties);
    free(properties);
}

static void mpris_metadata_copy(struct mpris_metadata  *d, const struct mpris_metadata s)
{
    if (NULL == d) { return; }

    mpris_metadata_zero(d);

    memcpy(d->composer, s.composer, MAX_PROPERTY_LENGTH);
    memcpy(d->track_id, s.track_id, MAX_PROPERTY_LENGTH);
    memcpy(d->album, s.album, MAX_PROPERTY_LENGTH);
    memcpy(d->content_created, s.content_created, MAX_PROPERTY_LENGTH);
    memcpy(d->title, s.title, MAX_PROPERTY_LENGTH);
    memcpy(d->url, s.url, MAX_PROPERTY_LENGTH);
    memcpy(d->art_url, s.art_url, MAX_PROPERTY_LENGTH);

    for (int i = 0; i < MAX_PROPERTY_COUNT; i++) {
        memcpy(d->album_artist[i], s.album_artist[i], MAX_PROPERTY_COUNT);
    }
    for (int i = 0; i < MAX_PROPERTY_COUNT; i++) {
        memcpy(d->genre[i], s.genre[i], MAX_PROPERTY_LENGTH);
    }
    for (int i = 0; i < MAX_PROPERTY_COUNT; i++) {
        memcpy(d->artist[i], s.artist[i], MAX_PROPERTY_LENGTH);
    }
    memcpy(d->comment, s.comment, sizeof(s.comment));

    // musicbrainz
    memcpy(d->mb_track_id, s.mb_track_id, MAX_PROPERTY_LENGTH);
    memcpy(d->mb_artist_id, s.mb_artist_id, MAX_PROPERTY_LENGTH);
    memcpy(d->mb_album_id, s.mb_album_id, MAX_PROPERTY_LENGTH);
    memcpy(d->mb_album_artist_id, s.mb_album_artist_id, MAX_PROPERTY_LENGTH);

    d->length = s.length;
    d->track_number = s.track_number;
    d->bitrate = s.bitrate;
    d->disc_number = s.disc_number;
    d->timestamp = s.timestamp;
}

static void mpris_properties_copy(struct mpris_properties *d, const struct mpris_properties *s)
{
    if (NULL == s) { return; }
    if (NULL == d) { return; }

    mpris_properties_zero(d, false);
    mpris_metadata_copy(&d->metadata, s->metadata);

    strncpy(d->player_name, s->player_name, MAX_PROPERTY_LENGTH);
    strncpy(d->loop_status, s->loop_status, MAX_PROPERTY_LENGTH);
    strncpy(d->playback_status, s->playback_status, MAX_PROPERTY_LENGTH);

    d->can_control = s->can_control;
    d->can_go_next = s->can_go_next;
    d->can_go_previous = s->can_go_previous;
    d->can_play = s->can_play;
    d->can_pause = s->can_pause;
    d->can_seek = s->can_seek;
    d->shuffle = s->shuffle;
}

static bool mpris_properties_is_playing(const struct mpris_properties *s)
{
    return (
        (NULL != s) &&
        (NULL != s->playback_status) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_PLAYING, strlen(MPRIS_PLAYBACK_STATUS_PLAYING)) == 0
    );
}

static bool mpris_properties_is_paused(const struct mpris_properties *s)
{
    return (
        (NULL != s) &&
        (NULL != s->playback_status) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_PAUSED, strlen(MPRIS_PLAYBACK_STATUS_PAUSED)) == 0
    );
}

static bool mpris_properties_is_stopped(const struct mpris_properties *s)
{
    return (
        (NULL != s) &&
        (NULL != s->playback_status) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_STOPPED, strlen(MPRIS_PLAYBACK_STATUS_STOPPED)) == 0
    );
}

enum playback_state get_mpris_playback_status(const struct mpris_properties *p)
{
    enum playback_state state = stopped;
    if (NULL != p->playback_status) {
        if (mpris_properties_is_playing(p)) {
            state = playing;
        }
        if (mpris_properties_is_paused(p)) {
            state = paused;
        }
        if (mpris_properties_is_stopped(p)) {
            state = stopped;
        }
    }
    return state;
}

bool mpris_player_is_valid(char **name)
{
    return (NULL != name && NULL != *name && strlen(*name) > 0);
}

static bool mpris_metadata_equals(const struct mpris_metadata s, const struct mpris_metadata p)
{
    bool result = (
        (memcmp(s.title, p.title, sizeof(s.title)) == 0) &&
        (memcmp(s.album, p.album, sizeof(s.album)) == 0) &&
        (s.length == p.length) &&
        (s.track_number == p.track_number) /*&&
        (s.start_time == p.start_time)*/
    );
    //_trace("mpris::check_metadata(%p:%p) %s", s, p, result ? "same" : "different");

    return result;
}

static bool mpris_properties_equals(const struct mpris_properties *sp, const struct mpris_properties *pp)
{
    bool result = false;
    if (NULL == sp) { goto _exit; }
    if (NULL == pp) { goto _exit; }
    if (sp == pp) { result = true; goto _exit; }

    result = mpris_metadata_equals(sp->metadata, pp->metadata);
    result &= (memcmp(sp->playback_status, pp->playback_status, MAX_PROPERTY_LENGTH) == 0);
    result &= (sp->position == pp->position);

_exit:
    _trace2("mpris::check_properties(%p:%p) %s", sp, pp, result ? "same" : "different");
    return result;
}

#if 0
static bool mpris_properties_are_loaded(const struct mpris_properties *p)
{
    bool result = false;

    result = (NULL != p->metadata->title && NULL != p->metadata->artist && NULL != p->metadata->album);

    return result;
}

void mpris_properties_print(struct mpris_metadata *s, enum log_levels log) {
    _log(log, "mpris::metadata: \n" \
        "\ttitle: %s\n" \
        "\tartist: %s\n" \
        "\talbum: %s",
    s->title, s->artist, s->album);
}
#endif

#endif // MPRIS_SCROBBLER_SMPRIS_H
