/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SMPRIS_H
#define MPRIS_SCROBBLER_SMPRIS_H

#define MPRIS_PLAYBACK_STATUS_PLAYING  "Playing"
#define MPRIS_PLAYBACK_STATUS_PAUSED   "Paused"
#define MPRIS_PLAYBACK_STATUS_STOPPED  "Stopped"

static void mpris_sb_arr_free(char ***elements)
{
    if (NULL == elements) { return; }
    if (NULL == *elements) { return; }

    int elements_count = sb_count(*elements);
    if (elements_count > 0) {
        for (int i = 0; i < elements_count; i++) {
            if (NULL != *elements[i]) { free(*elements[i]); }
            (void)sb_add(*elements, -1);
        }
    }
    sb_free(*elements);
    *elements = NULL;
}

static void mpris_metadata_zero(struct mpris_metadata *metadata)
{
    metadata->track_number = 0;
    metadata->bitrate = 0;
    metadata->disc_number = 0;
    metadata->length = 0;
    metadata->timestamp = 0;

    mpris_sb_arr_free(&metadata->genre);
    mpris_sb_arr_free(&metadata->artist);
    mpris_sb_arr_free(&metadata->album_artist);

    if (NULL != metadata->content_created) {
        memset(metadata->content_created, 0, strlen(metadata->content_created));
    }
    if (NULL != metadata->composer) {
        memset(metadata->composer, 0, strlen(metadata->composer));
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
    if (NULL != metadata->mb_track_id) {
        memset(metadata->mb_track_id, 0, strlen(metadata->mb_track_id));
    }
    if (NULL != metadata->mb_artist_id) {
        memset(metadata->mb_artist_id, 0, strlen(metadata->mb_artist_id));
    }
    if (NULL != metadata->mb_album_id) {
        memset(metadata->mb_album_id, 0, strlen(metadata->mb_album_id));
    }
    if (NULL != metadata->mb_album_artist_id) {
        memset(metadata->mb_album_artist_id, 0, strlen(metadata->mb_album_artist_id));
    }
    _trace2("mem::zeroed_metadata(%p)", metadata);
}

static void mpris_metadata_init(struct mpris_metadata *metadata)
{
    metadata->track_number = 0;
    metadata->bitrate = 0;
    metadata->disc_number = 0;
    metadata->length = 0;
    metadata->timestamp = 0;
    metadata->content_created = get_zero_string(MAX_PROPERTY_LENGTH);
    _trace2("mem::metadata::allocated:content_created:%p - %p", metadata->content_created, metadata->content_created + MAX_PROPERTY_LENGTH + 1);
    metadata->composer = get_zero_string(MAX_PROPERTY_LENGTH);
    _trace2("mem::metadata::allocated:composer:%p - %p", metadata->composer, metadata->composer + MAX_PROPERTY_LENGTH + 1);

    metadata->genre = NULL;
    _trace2("mem::metadata::allocated:genre[%zu:%p]", sb_count(metadata->genre), metadata->genre);

    metadata->artist = NULL;
    _trace2("mem::metadata::allocated:artist[%zu:%p]", sb_count(metadata->artist), metadata->artist);

    metadata->album_artist = NULL;
    _trace2("mem::metadata::allocated:album_artist[%zu:%p]", sb_count(metadata->album_artist), metadata->album_artist);

    metadata->comment = get_zero_string(MAX_PROPERTY_LENGTH);
    _trace2("mem::metadata::allocated:comment:%p - %p", metadata->comment, metadata->comment + MAX_PROPERTY_LENGTH + 1);
    metadata->track_id = get_zero_string(MAX_PROPERTY_LENGTH);
    _trace2("mem::metadata::allocated:track_id:%p - %p", metadata->track_id, metadata->track_id + MAX_PROPERTY_LENGTH + 1);
    metadata->album = get_zero_string(MAX_PROPERTY_LENGTH);
    _trace2("mem::metadata::allocated:album:%p - %p", metadata->album, metadata->album + MAX_PROPERTY_LENGTH + 1);
    metadata->title = get_zero_string(MAX_PROPERTY_LENGTH);
    _trace2("mem::metadata::allocated:title:%p - %p", metadata->title, metadata->title + MAX_PROPERTY_LENGTH + 1);
    metadata->url = get_zero_string(MAX_PROPERTY_LENGTH);
    _trace2("mem::metadata::allocated:url:%p - %p", metadata->url, metadata->url + MAX_PROPERTY_LENGTH + 1);
    metadata->art_url = get_zero_string(MAX_PROPERTY_LENGTH);
    _trace2("mem::inited_metadata(%p)", metadata);
    metadata->mb_track_id = get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->mb_album_id = get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->mb_artist_id = get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->mb_album_artist_id = get_zero_string(MAX_PROPERTY_LENGTH);
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

    _trace2("mem::metadata::free(%p)", metadata);
    if (NULL != metadata->content_created) {
        if (strlen(metadata->content_created) > 0) {
            _trace2("mem::metadata::free:content_created(%p): %s", metadata->content_created, metadata->content_created);
        }
        free(metadata->content_created);
        metadata->content_created = NULL;
    }
    if (NULL != metadata->composer) {
        if (strlen(metadata->composer) > 0) {
            _trace2("mem::metadata::free:composer(%p): %s", metadata->composer, metadata->composer);
        }
        free(metadata->composer);
        metadata->composer = NULL;
    }
    if (NULL != metadata->genre) {
        _trace2("mem::metadata::free::genre(%zu:%p): %s", sb_count(metadata->genre), metadata->genre[0], metadata->genre[0]);
        mpris_sb_arr_free(&metadata->genre);
    }
    if (NULL !=  metadata->artist) {
        _trace2("mem::metadata::free:artist(%zu:%p): %s...", sb_count(metadata->artist), metadata->artist[0], metadata->artist[0]);
        mpris_sb_arr_free(&metadata->artist);
    }
    if (NULL != metadata->album_artist) {
        _trace2("mem::metadata::free:album_artist(%zu:%p): %s...", sb_count(metadata->album_artist), metadata->album_artist[0], metadata->album_artist[0]);
        mpris_sb_arr_free(&metadata->album_artist);
    }
    if (NULL != metadata->comment) {
        if (strlen(metadata->comment) > 0) {
            _trace2("mem::metadata::free:comment(%p): %s", metadata->comment, metadata->comment);
        }
        free(metadata->comment);
        metadata->comment = NULL;
    }
    if (NULL != metadata->track_id) {
        if (strlen(metadata->track_id) > 0) {
            _trace2("mem::metadata::free:track_id(%p): %s", metadata->track_id, metadata->track_id);
        }
        free(metadata->track_id);
        metadata->track_id = NULL;
    }
    if (NULL != metadata->album) {
        if (strlen(metadata->album) > 0) {
            _trace2("mem::metadata::free:album(%p): %s", metadata->album, metadata->album);
        }
        free(metadata->album);
        metadata->album = NULL;
    }
    if (NULL != metadata->title) {
        if (strlen(metadata->title) > 0) {
            _trace2("mem::metadata::free:title(%p): %s", metadata->title, metadata->title);
        }
        free(metadata->title);
        metadata->title = NULL;
    }
    if (NULL != metadata->url) {
        if (strlen(metadata->url) > 0) {
            _trace2("mem::metadata::free:url(%p): %s", metadata->url, metadata->url);
        }
        free(metadata->url);
        metadata->url = NULL;
    }
    if (NULL != metadata->art_url) {
        if (strlen(metadata->art_url) > 0) {
            _trace2("mem::metadata::free:art_url(%p): %s", metadata->art_url, metadata->art_url);
        }
        free(metadata->art_url);
        metadata->art_url = NULL;
    }
    if (NULL != metadata->mb_track_id) {
        if (strlen(metadata->mb_track_id) > 0) {
            _trace2("mem::metadata::musicbrainz::free:track_id(%p): %s", metadata->mb_track_id, metadata->mb_track_id);
        }
        free(metadata->mb_track_id);
        metadata->mb_track_id = NULL;
    }
    if (NULL != metadata->mb_artist_id) {
        if (strlen(metadata->mb_artist_id) > 0) {
            _trace2("mem::metadata::musicbrainz::free:artist_id(%p): %s", metadata->mb_artist_id, metadata->mb_artist_id);
        }
        free(metadata->mb_artist_id);
        metadata->mb_artist_id = NULL;
    }
    if (NULL != metadata->mb_album_id) {
        if (strlen(metadata->mb_album_id) > 0) {
            _trace2("mem::metadata::musicbrainz::free:album_id(%p): %s", metadata->mb_album_id, metadata->mb_album_id);
        }
        free(metadata->mb_album_id);
        metadata->mb_album_id = NULL;
    }
    if (NULL != metadata->mb_album_artist_id) {
        if (strlen(metadata->mb_album_artist_id) > 0) {
            _trace2("mem::metadata::musicbrainz::free:album_artist_id(%p): %s", metadata->mb_album_artist_id, metadata->mb_album_artist_id);
        }
        free(metadata->mb_album_artist_id);
        metadata->mb_album_artist_id = NULL;
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
    _trace2("mem::zeroed_properties(%p)", properties);
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
    _trace2("mem::inited_properties(%p)", properties);
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
    _trace2("mem::properties::free(%p)", properties);
    mpris_metadata_free(properties->metadata);
    if (NULL != properties->player_name) {
        if (strlen(properties->player_name)) {
            _trace2("mem::properties::free:player_name(%p): %s", properties->player_name, properties->player_name);
        }
        free(properties->player_name);
        properties->player_name = NULL;
    }
    if (NULL != properties->loop_status) {
        if (strlen(properties->loop_status)) {
            _trace2("mem::properties::free:loop_status(%p): %s", properties->loop_status, properties->loop_status);
        }
        free(properties->loop_status);
        properties->loop_status = NULL;
    }
    if (NULL != properties->playback_status) {
        if (strlen(properties->playback_status)) {
            _trace2("mem::properties::free:playback_status(%p): %s", properties->playback_status, properties->playback_status);
        }
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

    strncpy(d->composer, s->composer, MAX_PROPERTY_LENGTH);
    strncpy(d->comment, s->comment, MAX_PROPERTY_LENGTH);
    strncpy(d->track_id, s->track_id, MAX_PROPERTY_LENGTH);
    strncpy(d->album, s->album, MAX_PROPERTY_LENGTH);
    strncpy(d->content_created, s->content_created, MAX_PROPERTY_LENGTH);
    strncpy(d->title, s->title, MAX_PROPERTY_LENGTH);
    strncpy(d->url, s->url, MAX_PROPERTY_LENGTH);
    strncpy(d->art_url, s->art_url, MAX_PROPERTY_LENGTH);

    for (int i = 0; i < sb_count(s->album_artist); i++) {
        char *elem = get_zero_string(MAX_PROPERTY_LENGTH);
        strncpy(elem, s->album_artist[i], MAX_PROPERTY_LENGTH);
        sb_push(d->album_artist, elem);
    }

    for (int i = 0; i < sb_count(s->genre); i++) {
        char *elem = get_zero_string(MAX_PROPERTY_LENGTH);
        strncpy(elem, s->genre[i], MAX_PROPERTY_LENGTH);
        sb_push(d->genre, elem);
    }

    for (int i = 0; i < sb_count(s->artist); i++) {
        char *elem = get_zero_string(MAX_PROPERTY_LENGTH);
        strncpy(elem, s->artist[i], MAX_PROPERTY_LENGTH);
        sb_push(d->artist, elem);
    }

    // musicbrainz
    if (NULL != s->mb_track_id) {
        strncpy(d->mb_track_id, s->mb_track_id, MAX_PROPERTY_LENGTH);
    }
    if (NULL != s->mb_artist_id) {
        strncpy(d->mb_artist_id, s->mb_artist_id, MAX_PROPERTY_LENGTH);
    }
    if (NULL != s->mb_album_id) {
        strncpy(d->mb_album_id, s->mb_album_id, MAX_PROPERTY_LENGTH);
    }
    if (NULL != s->mb_album_artist_id) {
        strncpy(d->mb_album_artist_id, s->mb_album_artist_id, MAX_PROPERTY_LENGTH);
    }
    d->length = s->length;
    d->track_number = s->track_number;
    d->bitrate = s->bitrate;
    d->disc_number = s->disc_number;
    d->timestamp = s->timestamp;
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
    bool result = false;
    if (NULL == sp) { goto _exit; }
    if (NULL == pp) { goto _exit; }
    if (sp == pp) { result = true; goto _exit; }

    result = mpris_metadata_equals(sp->metadata, pp->metadata);
_exit:
    _trace("mpris::check_properties(%p:%p) %s", sp, pp, result ? "same" : "different");
    return result;
}
#endif

void mpris_properties_print(struct mpris_metadata *s) {
    _trace("print metadata: \n" \
        "\ttitle: %s\n" \
        "\tartist: %s\n" \
        "\talbum: %s",
    s->title, s->artist, s->album);
}

#endif // MPRIS_SCROBBLER_SMPRIS_H
