/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef SMPRIS_H
#define SMPRIS_H
#define MPRIS_PLAYBACK_STATUS_PLAYING  "Playing"
#define MPRIS_PLAYBACK_STATUS_PAUSED   "Paused"
#define MPRIS_PLAYBACK_STATUS_STOPPED  "Stopped"

#if 1
static void mpris_metadata_zero(mpris_metadata* metadata)
{
    metadata->track_number = 0;
    metadata->bitrate = 0;
    metadata->disc_number = 0;
    metadata->length = 0;
    metadata->content_created = 0;
    if (NULL != metadata->album_artist) { free(metadata->album_artist); metadata->album_artist = NULL; }
    if (NULL != metadata->composer) { free(metadata->composer); metadata->composer = NULL; }
    if (NULL != metadata->genre) { free(metadata->genre); metadata->genre = NULL; }
    if (NULL != metadata->artist) { free(metadata->artist); metadata->artist = NULL; }
    if (NULL != metadata->comment) { free(metadata->comment); metadata->comment = NULL; }
    if (NULL != metadata->album) { free(metadata->album); metadata->album = NULL; }
    if (NULL != metadata->title) { free(metadata->title); metadata->title = NULL; }
    if (NULL != metadata->url) { free(metadata->url); metadata->url = NULL; }
    if (NULL != metadata->art_url) { free(metadata->art_url); metadata->art_url = NULL; }
    if (NULL != metadata->track_id) { free(metadata->track_id); metadata->track_id = NULL; }
    _trace("mem::zeroed_metadata(%p)", metadata);
}
#endif

void mpris_metadata_init(mpris_metadata* metadata)
{
    metadata->track_number = 0;
    metadata->bitrate = 0;
    metadata->disc_number = 0;
    metadata->length = 0;
    metadata->content_created = NULL;
    metadata->album_artist = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->composer = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->genre = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->artist = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->comment = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->track_id = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->album = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->title = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->url = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->art_url = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    _trace("mem::inited_metadata(%p)", metadata);
}

mpris_metadata *mpris_metadata_new()
{
    mpris_metadata *metadata = malloc(sizeof(mpris_metadata));
    mpris_metadata_init(metadata);

    return metadata;
}

void mpris_metadata_unref(mpris_metadata *metadata)
{
    if (NULL == metadata) { return; }

    _trace("mem::metadata::free(%p)", metadata);
    if (NULL != metadata->album_artist) {
        _trace("mem::metadata::free:album_artist(%p): %s", metadata->album_artist, metadata->album_artist);
        free(metadata->album_artist);
        //metadata->album_artist = NULL;
    }
    if (NULL != metadata->composer) {
        _trace("mem::metadata::free:composer(%p): %s", metadata->composer, metadata->composer);
        free(metadata->composer);
        //metadata->composer = NULL;
    }
    if (NULL != metadata->genre) {
        _trace("mem::metadata::free:genre(%p): %s", metadata->genre, metadata->genre);
        free(metadata->genre);
        //metadata->genre = NULL;
    }
    if (NULL != metadata->artist) {
        _trace("mem::metadata::free:artist(%p): %s", metadata->artist, metadata->artist);
        free(metadata->artist);
        //metadata->artist = NULL;
    }
    if (NULL != metadata->comment) {
        _trace("mem::metadata::free:comment(%p): %s", metadata->comment, metadata->comment);
        free(metadata->comment);
        //metadata->comment = NULL;
    }
    if (NULL != metadata->track_id) {
        _trace("mem::metadata::free:track_id(%p): %s", metadata->track_id, metadata->track_id);
        free(metadata->track_id);
        //metadata->track_id = NULL;
    }
    if (NULL != metadata->album) {
        _trace("mem::metadata::free:album(%p): %s", metadata->album, metadata->album);
        free(metadata->album);
        //metadata->album = NULL;
    }
    if (NULL != metadata->title) {
        _trace("mem::metadata::free:title(%p): %s", metadata->title, metadata->title);
        free(metadata->title);
        //metadata->title = NULL;
    }
    if (NULL != metadata->url) {
        _trace("mem::metadata::free:url(%p): %s", metadata->url, metadata->url);
        free(metadata->url);
        //metadata->url = NULL;
    }
    if (NULL != metadata->art_url) {
        _trace("mem::metadata::free:art_url(%p): %s", metadata->art_url, metadata->art_url);
        free(metadata->art_url);
        //metadata->art_url = NULL;
    }
    free(metadata);
}

#if 1
static void mpris_properties_zero(mpris_properties *properties, bool metadata_too)
{
    properties->volume = 0;
    properties->position = 0;
    if (NULL != properties->player_name) { free(properties->player_name); properties->player_name = NULL; }
    if (NULL != properties->loop_status) { free(properties->loop_status); properties->loop_status = NULL; }
    if (NULL != properties->playback_status) { free(properties->playback_status); properties->playback_status = NULL; }
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
#endif

void mpris_properties_init(mpris_properties *properties)
{
    properties->metadata = mpris_metadata_new();
    properties->volume = 0;
    properties->position = 0;
    properties->player_name = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    properties->loop_status = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    properties->playback_status = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    properties->can_control = false;
    properties->can_go_next = false;
    properties->can_go_previous = false;
    properties->can_play = false;
    properties->can_pause = false;
    properties->can_seek = false;
    properties->shuffle = false;
    _trace("mem::inited_properties(%p)", properties);
}

mpris_properties *mpris_properties_new()
{
    mpris_properties* properties = malloc(sizeof(mpris_properties));
    mpris_properties_init(properties);

    return properties;
}

void mpris_properties_unref(mpris_properties *properties)
{
    if (NULL == properties) { return; }
    _trace("mem::properties::free(%p)", properties);
    mpris_metadata_unref(properties->metadata);
    if (NULL != properties->player_name) {
        _trace("mem::properties::free:player_name(%p): %s", properties->player_name, properties->player_name);
        free(properties->player_name);
        //properties->player_name = NULL;
    }
    if (NULL != properties->loop_status) {
        _trace("mem::properties::free:loop_status(%p): %s", properties->loop_status, properties->loop_status);
        free(properties->loop_status);
        //properties->loop_status = NULL;
    }
    if (NULL != properties->playback_status) {
        _trace("mem::properties::free:playback_status(%p): %s", properties->playback_status, properties->playback_status);
        free(properties->playback_status);
        //properties->playback_status = NULL;
    }
    free(properties);
}

void mpris_metadata_copy(mpris_metadata  *d, const mpris_metadata *s)
{
    if (NULL == s) { return; }
    if (NULL == d) { return; }

    mpris_metadata_zero(d);

    cpstr(&d->album_artist, s->album_artist, MAX_PROPERTY_LENGTH);
    cpstr(&d->composer, s->composer, MAX_PROPERTY_LENGTH);
    cpstr(&d->genre, s->genre, MAX_PROPERTY_LENGTH);
    cpstr(&d->artist, s->artist, MAX_PROPERTY_LENGTH);
    cpstr(&d->comment, s->comment, MAX_PROPERTY_LENGTH);
    cpstr(&d->track_id, s->track_id, MAX_PROPERTY_LENGTH);
    cpstr(&d->album, s->album, MAX_PROPERTY_LENGTH);
    cpstr(&d->content_created, s->content_created, MAX_PROPERTY_LENGTH);
    cpstr(&d->title, s->title, MAX_PROPERTY_LENGTH);
    cpstr(&d->url, s->url, MAX_PROPERTY_LENGTH);
    cpstr(&d->art_url, s->art_url, MAX_PROPERTY_LENGTH);
    d->length = s->length;
    d->track_number = s->track_number;
    d->bitrate = s->bitrate;
    d->disc_number = s->disc_number;
}

void mpris_properties_copy(mpris_properties *d, const mpris_properties* s)
{
    if (NULL == s) { return; }
    if (NULL == d) { return; }

    mpris_properties_zero(d, false);
    mpris_metadata_copy(d->metadata, s->metadata);

    cpstr(&d->player_name, s->player_name, MAX_PROPERTY_LENGTH);
    cpstr(&d->loop_status, s->loop_status, MAX_PROPERTY_LENGTH);
    cpstr(&d->playback_status, s->playback_status, MAX_PROPERTY_LENGTH);

    d->can_control = s->can_control;
    d->can_go_next = s->can_go_next;
    d->can_go_previous = s->can_go_previous;
    d->can_play = s->can_play;
    d->can_pause = s->can_pause;
    d->can_seek = s->can_seek;
    d->shuffle = s->shuffle;
}

bool mpris_properties_is_playing(const mpris_properties *s)
{
    return (
        (NULL != s) &&
        (NULL != s->playback_status) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_PLAYING, strlen(MPRIS_PLAYBACK_STATUS_PLAYING)) == 0
    );
}

bool mpris_properties_is_paused(const mpris_properties *s)
{
    return (
        (NULL != s) &&
        (NULL != s->playback_status) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_PAUSED, strlen(MPRIS_PLAYBACK_STATUS_PAUSED)) == 0
    );
}

bool mpris_properties_is_stopped(const mpris_properties *s)
{
    return (
        (NULL != s) &&
        (NULL != s->playback_status) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_STOPPED, strlen(MPRIS_PLAYBACK_STATUS_STOPPED)) == 0
    );
}

playback_state get_mpris_playback_status(const mpris_properties *p)
{
    playback_state state = stopped;
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

bool mpris_player_is_valid(char** name)
{
    return (NULL != name && NULL != *name && strlen(*name) > 0);
}

bool mpris_properties_are_loaded(const mpris_properties *p)
{
    bool result = false;

    result = (NULL != p->metadata->title && NULL != p->metadata->artist && NULL != p->metadata->album);

    return result;
}
#endif // SMPRIS_H
