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
#if 0
    zero_string(&metadata->album_artist, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->composer, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->genre, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->artist, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->comment, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->track_id, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->album, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->title, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->url, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->art_url, MAX_PROPERTY_LENGTH);
#endif
    _log(tracing, "mem::zeroed_metadata(%p)", metadata);
}
#endif

void mpris_metadata_init(mpris_metadata* metadata)
{
    metadata->track_number = 0;
    metadata->bitrate = 0;
    metadata->disc_number = 0;
    metadata->length = 0;
    metadata->content_created = 0;
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
    _log(tracing, "mem::inited_metadata(%p)", metadata);
}

mpris_metadata *mpris_metadata_new()
{
    mpris_metadata *metadata = malloc(sizeof(mpris_metadata));
    mpris_metadata_init(metadata);

    return metadata;
}

void mpris_metadata_unref(void *data)
{
    mpris_metadata *metadata = data;
    _log(tracing, "mem::freeing_metadata(%p)", metadata);
#if 0
    if (NULL != metadata->album_artist) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->album_artist, metadata->album_artist);
        free(metadata->album_artist);
    }
    if (NULL != metadata->composer) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->composer, metadata->composer);
        free(metadata->composer);
    }
    if (NULL != metadata->genre) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->genre, metadata->genre);
        free(metadata->genre);
    }
    if (NULL != metadata->artist) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->artist, metadata->artist);
        free(metadata->artist);
    }
    if (NULL != metadata->comment) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->comment, metadata->comment);
        free(metadata->comment);
    }
    if (NULL != metadata->track_id) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->track_id, metadata->track_id);
        free(metadata->track_id);
    }
    if (NULL != metadata->album) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->album, metadata->album);
        free(metadata->album);
    }
    if (NULL != metadata->title) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->title, metadata->title);
        free(metadata->title);
    }
    if (NULL != metadata->url) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->url, metadata->url);
        free(metadata->url);
    }
    if (NULL != metadata->art_url) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->art_url, metadata->art_url);
        free(metadata->art_url);
    }
#endif
    if (NULL != metadata) {  free(metadata); }
}

#if 1
static void mpris_properties_zero(mpris_properties *properties)
{
    mpris_metadata_zero(properties->metadata);
    properties->volume = 0;
    properties->position = 0;
#if 0
    zero_string(&properties->player_name,MAX_PROPERTY_LENGTH);
    zero_string(&properties->loop_status, 20);
    zero_string(&properties->playback_status, 20);
#endif
    properties->can_control = false;
    properties->can_go_next = false;
    properties->can_go_previous = false;
    properties->can_play = false;
    properties->can_pause = false;
    properties->can_seek = false;
    properties->shuffle = false;
    _log(tracing, "mem::zeroed_properties(%p)", properties);
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
    _log(tracing, "mem::inited_properties(%p)", properties);
}

mpris_properties *mpris_properties_new()
{
    mpris_properties* properties = malloc(sizeof(mpris_properties));
    mpris_properties_init(properties);

    return properties;
}

void mpris_properties_unref(void *data)
{
    mpris_properties *properties = data;
    _log(tracing, "mem::freeing_properties(%p)", properties);
    mpris_metadata_unref(properties->metadata);
#if 0
    if (NULL != properties->player_name) {
        _log(tracing, "mem:freeing_str(%p): %s", properties->player_name, properties->player_name);
        free(properties->player_name);
    }
    if (NULL != properties->loop_status) {
        _log(tracing, "mem:freeing_str(%p): %s", properties->loop_status, properties->loop_status);
        free(properties->loop_status);
    }
    if (NULL != properties->playback_status) {
        _log(tracing, "mem:freeing_str(%p): %s", properties->playback_status, properties->playback_status);
        free(properties->playback_status);
    }
#endif
    if (NULL != properties) { free(properties); }
}

inline bool mpris_properties_is_playing(const mpris_properties *s)
{
    return (
        (NULL != s) &&
        (NULL != s->playback_status) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_PLAYING, strlen(MPRIS_PLAYBACK_STATUS_PLAYING)) == 0
    );
}

inline bool mpris_properties_is_paused(const mpris_properties *s)
{
    return (
        (NULL != s) &&
        (NULL != s->playback_status) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_STOPPED, strlen(MPRIS_PLAYBACK_STATUS_STOPPED)) == 0
    );
}

inline bool mpris_properties_is_stopped(const mpris_properties *s)
{
    return (
        (NULL != s) &&
        (NULL != s->playback_status) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_PAUSED, strlen(MPRIS_PLAYBACK_STATUS_PAUSED)) == 0
    );
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
