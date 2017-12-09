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

    if (NULL != metadata->content_created) {
        memset(metadata->content_created, 0, strlen(metadata->content_created));
    }
    if (NULL != metadata->album_artist) {
        memset(metadata->album_artist, 0, strlen(metadata->album_artist));
    }
    if (NULL != metadata->composer) {
        memset(metadata->composer, 0, strlen(metadata->composer));
    }
    if (NULL != metadata->genre) {
        memset(metadata->genre, 0, strlen(metadata->genre));
    }
    if (NULL != metadata->artist) {
        memset(metadata->artist, 0, strlen(metadata->artist));
    }
    if (NULL != metadata->comment) {
        memset(metadata->comment, 0, strlen(metadata->comment));
    }
    if (NULL != metadata->album) {
        memset(metadata->album, 0, strlen(metadata->album));
    }
    if (NULL != metadata->title) {
        memset(metadata->title, 0, strlen(metadata->title));
    }
    if (NULL != metadata->url) {
        memset(metadata->url, 0, strlen(metadata->url));
    }
    if (NULL != metadata->art_url) {
        memset(metadata->art_url, 0, strlen(metadata->art_url));
    }
    if (NULL != metadata->track_id) {
        memset(metadata->track_id, 0, strlen(metadata->track_id));
    }
    _trace("mem::zeroed_metadata(%p)", metadata);
}

static void mpris_metadata_init(struct mpris_metadata *metadata)
{
    metadata->track_number = 0;
    metadata->bitrate = 0;
    metadata->disc_number = 0;
    metadata->length = 0;
    metadata->content_created = get_zero_string(MAX_PROPERTY_LENGTH);
    //_trace("mem::metadata::alloced:content_created:%p - %p", metadata->content_created, metadata->content_created + MAX_PROPERTY_LENGTH + 1);
    metadata->album_artist = get_zero_string(MAX_PROPERTY_LENGTH);
    //_trace("mem::metadata::alloced:album_artist:%p - %p", metadata->album_artist, metadata->album_artist + MAX_PROPERTY_LENGTH + 1);
    metadata->composer = get_zero_string(MAX_PROPERTY_LENGTH);
    //_trace("mem::metadata::alloced:composer:%p - %p", metadata->composer, metadata->composer + MAX_PROPERTY_LENGTH + 1);
    metadata->genre = get_zero_string(MAX_PROPERTY_LENGTH);
    //_trace("mem::metadata::alloced:genre:%p - %p", metadata->genre, metadata->genre + MAX_PROPERTY_LENGTH + 1);
    metadata->artist = get_zero_string(MAX_PROPERTY_LENGTH);
    //_trace("mem::metadata::alloced:artist:%p - %p", metadata->artist, metadata->artist + MAX_PROPERTY_LENGTH + 1);
    metadata->comment = get_zero_string(MAX_PROPERTY_LENGTH);
    //_trace("mem::metadata::alloced:comment:%p - %p", metadata->comment, metadata->comment + MAX_PROPERTY_LENGTH + 1);
    metadata->track_id = get_zero_string(MAX_PROPERTY_LENGTH);
    //_trace("mem::metadata::alloced:track_id:%p - %p", metadata->track_id, metadata->track_id + MAX_PROPERTY_LENGTH + 1);
    metadata->album = get_zero_string(MAX_PROPERTY_LENGTH);
    //_trace("mem::metadata::alloced:album:%p - %p", metadata->album, metadata->album + MAX_PROPERTY_LENGTH + 1);
    metadata->title = get_zero_string(MAX_PROPERTY_LENGTH);
    //_trace("mem::metadata::alloced:title:%p - %p", metadata->title, metadata->title + MAX_PROPERTY_LENGTH + 1);
    metadata->url = get_zero_string(MAX_PROPERTY_LENGTH);
    //_trace("mem::metadata::alloced:url:%p - %p", metadata->url, metadata->url + MAX_PROPERTY_LENGTH + 1);
    metadata->art_url = get_zero_string(MAX_PROPERTY_LENGTH);
    //_trace("mem::metadata::alloced:album_artist:%p - %p", metadata->album_artist, metadata->album_artist + MAX_PROPERTY_LENGTH + 1);
    //_trace("mem::inited_metadata(%p)", metadata);
}

static struct mpris_metadata *mpris_metadata_new(void)
{
    struct mpris_metadata *metadata = malloc(sizeof(struct mpris_metadata));
    mpris_metadata_init(metadata);

    return metadata;
}

static void mpris_metadata_free(struct mpris_metadata *metadata)
{
    if (NULL == metadata) { return; }

    _trace("mem::metadata::free(%p)", metadata);
    if (NULL != metadata->content_created) {
        _trace("mem::metadata::free:content_created(%p): %s", metadata->content_created, metadata->content_created);
        free(metadata->content_created);
        metadata->content_created = NULL;
    }
    if (NULL != metadata->album_artist) {
        _trace("mem::metadata::free:album_artist(%p): %s", metadata->album_artist, metadata->album_artist);
        free(metadata->album_artist);
        metadata->album_artist = NULL;
    }
    if (NULL != metadata->composer) {
        _trace("mem::metadata::free:composer(%p): %s", metadata->composer, metadata->composer);
        free(metadata->composer);
        metadata->composer = NULL;
    }
    if (NULL != metadata->genre) {
        _trace("mem::metadata::free:genre(%p): %s", metadata->genre, metadata->genre);
        free(metadata->genre);
        metadata->genre = NULL;
    }
    if (NULL != metadata->artist) {
        _trace("mem::metadata::free:artist(%p): %s", metadata->artist, metadata->artist);
        free(metadata->artist);
        metadata->artist = NULL;
    }
    if (NULL != metadata->comment) {
        _trace("mem::metadata::free:comment(%p): %s", metadata->comment, metadata->comment);
        free(metadata->comment);
        metadata->comment = NULL;
    }
    if (NULL != metadata->track_id) {
        _trace("mem::metadata::free:track_id(%p): %s", metadata->track_id, metadata->track_id);
        free(metadata->track_id);
        metadata->track_id = NULL;
    }
    if (NULL != metadata->album) {
        _trace("mem::metadata::free:album(%p): %s", metadata->album, metadata->album);
        free(metadata->album);
        metadata->album = NULL;
    }
    if (NULL != metadata->title) {
        _trace("mem::metadata::free:title(%p): %s", metadata->title, metadata->title);
        free(metadata->title);
        metadata->title = NULL;
    }
    if (NULL != metadata->url) {
        _trace("mem::metadata::free:url(%p): %s", metadata->url, metadata->url);
        free(metadata->url);
        metadata->url = NULL;
    }
    if (NULL != metadata->art_url) {
        _trace("mem::metadata::free:art_url(%p): %s", metadata->art_url, metadata->art_url);
        free(metadata->art_url);
        metadata->art_url = NULL;
    }
    free(metadata);
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
    if (metadata_too) { mpris_metadata_zero(properties->metadata); }
    _trace("mem::zeroed_properties(%p)", properties);
}

static void mpris_properties_init(struct mpris_properties *properties)
{
    properties->metadata = mpris_metadata_new();
    properties->volume = 0;
    properties->position = 0;
    properties->player_name = get_zero_string(MAX_PROPERTY_LENGTH);
    properties->loop_status = get_zero_string(MAX_PROPERTY_LENGTH);
    properties->playback_status = get_zero_string(MAX_PROPERTY_LENGTH);
    properties->can_control = false;
    properties->can_go_next = false;
    properties->can_go_previous = false;
    properties->can_play = false;
    properties->can_pause = false;
    properties->can_seek = false;
    properties->shuffle = false;
    _trace("mem::inited_properties(%p)", properties);
}

struct mpris_properties *mpris_properties_new(void)
{
    struct mpris_properties *properties = malloc(sizeof(struct mpris_properties));
    mpris_properties_init(properties);

    return properties;
}

void mpris_properties_free(struct mpris_properties *properties)
{
    if (NULL == properties) { return; }
    _trace("mem::properties::free(%p)", properties);
    mpris_metadata_free(properties->metadata);
    if (NULL != properties->player_name) {
        _trace("mem::properties::free:player_name(%p): %s", properties->player_name, properties->player_name);
        free(properties->player_name);
        properties->player_name = NULL;
    }
    if (NULL != properties->loop_status) {
        _trace("mem::properties::free:loop_status(%p): %s", properties->loop_status, properties->loop_status);
        free(properties->loop_status);
        properties->loop_status = NULL;
    }
    if (NULL != properties->playback_status) {
        _trace("mem::properties::free:playback_status(%p): %s", properties->playback_status, properties->playback_status);
        free(properties->playback_status);
        properties->playback_status = NULL;
    }
    free(properties);
}

static void mpris_metadata_copy(struct mpris_metadata  *d, const struct mpris_metadata *s)
{
    if (NULL == s) { return; }
    if (NULL == d) { return; }

    mpris_metadata_zero(d);

    strncpy(d->album_artist, s->album_artist, MAX_PROPERTY_LENGTH);
    strncpy(d->composer, s->composer, MAX_PROPERTY_LENGTH);
    strncpy(d->genre, s->genre, MAX_PROPERTY_LENGTH);
    strncpy(d->artist, s->artist, MAX_PROPERTY_LENGTH);
    strncpy(d->comment, s->comment, MAX_PROPERTY_LENGTH);
    strncpy(d->track_id, s->track_id, MAX_PROPERTY_LENGTH);
    strncpy(d->album, s->album, MAX_PROPERTY_LENGTH);
    strncpy(d->content_created, s->content_created, MAX_PROPERTY_LENGTH);
    strncpy(d->title, s->title, MAX_PROPERTY_LENGTH);
    strncpy(d->url, s->url, MAX_PROPERTY_LENGTH);
    strncpy(d->art_url, s->art_url, MAX_PROPERTY_LENGTH);
    d->length = s->length;
    d->track_number = s->track_number;
    d->bitrate = s->bitrate;
    d->disc_number = s->disc_number;
}

static void mpris_properties_copy(struct mpris_properties *d, const struct mpris_properties *s)
{
    if (NULL == s) { return; }
    if (NULL == d) { return; }

    mpris_properties_zero(d, false);
    mpris_metadata_copy(d->metadata, s->metadata);

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

#if 0
static bool mpris_properties_are_loaded(const struct mpris_properties *p)
{
    bool result = false;

    result = (NULL != p->metadata->title && NULL != p->metadata->artist && NULL != p->metadata->album);

    return result;
}
#endif

static bool mpris_metadata_equals(const struct mpris_metadata *s, const struct mpris_metadata *p)
{
    if (NULL == s) { return false; }
    if (NULL == p) { return false; }
    if (s == p) { return true; }

    bool result = (
        (strncmp(s->title, p->title, strlen(s->title)) == 0) &&
        (strncmp(s->album, p->album, strlen(s->album)) == 0) &&
        (strncmp(s->artist, p->artist, strlen(s->artist)) == 0) &&
        (s->length == p->length) &&
        (s->track_number == p->track_number) /*&&
        (s->start_time == p->start_time)*/
    );
    _trace("mpris::check_metadata(%p:%p) %s", s, p, result ? "same" : "different");

    return result;
}

static bool mpris_properties_equals(const struct mpris_properties *sp, const struct mpris_properties *pp)
{
    if (NULL == sp) { return false; }
    if (NULL == pp) { return false; }
    if (sp == pp) { return true; }

    bool result = mpris_metadata_equals(sp->metadata, pp->metadata);
    _trace("mpris::check_properties(%p:%p) %s", sp, pp, result ? "same" : "different");

    return result;
}

#endif // MPRIS_SCROBBLER_SMPRIS_H
