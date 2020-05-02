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
    memset(properties->player_name, 0, strlen(properties->player_name));
    memset(properties->loop_status, 0, strlen(properties->loop_status));
    memset(properties->playback_status, 0, strlen(properties->playback_status));
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

static bool mpris_properties_is_playing(const struct mpris_properties *s)
{
    return (
        (NULL != s) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_PLAYING, strlen(MPRIS_PLAYBACK_STATUS_PLAYING)) == 0
    );
}

static bool mpris_properties_is_paused(const struct mpris_properties *s)
{
    return (
        (NULL != s) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_PAUSED, strlen(MPRIS_PLAYBACK_STATUS_PAUSED)) == 0
    );
}

static bool mpris_properties_is_stopped(const struct mpris_properties *s)
{
    return (
        (NULL != s) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_STOPPED, strlen(MPRIS_PLAYBACK_STATUS_STOPPED)) == 0
    );
}

enum playback_state get_mpris_playback_status(const struct mpris_properties *p)
{
    enum playback_state state = stopped;
    if (mpris_properties_is_playing(p)) {
        state = playing;
    }
    if (mpris_properties_is_paused(p)) {
        state = paused;
    }
    if (mpris_properties_is_stopped(p)) {
        state = stopped;
    }
    return state;
}

bool mpris_player_is_valid(char **name)
{
    return (NULL != name && NULL != *name && strlen(*name) > 0);
}

static bool mpris_metadata_equals(const struct mpris_metadata *s, const struct mpris_metadata *p)
{
    bool result = (
        (strlen(s->title)+strlen(p->title) > 0 && memcmp(s->title, p->title, sizeof(s->title)) == 0) &&
        (strlen(s->album)+strlen(p->album) > 0 && memcmp(s->album, p->album, sizeof(s->album)) == 0) &&
        (strlen(s->artist[0])+strlen(p->artist[0]) > 0 && _eq(s->artist, p->artist)) &&
        (s->length == p->length) &&
        (s->track_number == p->track_number) /*&&
        (s->start_time == p->start_time)*/
    );
    _trace2("mpris::check_metadata(%p:%p) %s", s, p, result ? "same" : "different");

    return result;
}

static bool mpris_properties_equals(const struct mpris_properties *sp, const struct mpris_properties *pp)
{
    if (NULL == sp) { return false; }
    if (NULL == pp) { return false; }
    if (sp == pp) { return true; }

    bool result = mpris_metadata_equals(&sp->metadata, &pp->metadata) &&
        strlen(sp->playback_status)+strlen(pp->playback_status) > 0 &&
        _eq(sp->playback_status, pp->playback_status);

    _trace2("mpris::check_properties(%p:%p) %s", sp, pp, result ? "same" : "different");
    return result;
}

#endif // MPRIS_SCROBBLER_SMPRIS_H
