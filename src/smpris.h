/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SMPRIS_H
#define MPRIS_SCROBBLER_SMPRIS_H

#define MPRIS_PLAYBACK_STATUS_PLAYING  "Playing"
#define MPRIS_PLAYBACK_STATUS_PAUSED   "Paused"
#define MPRIS_PLAYBACK_STATUS_STOPPED  "Stopped"

struct mpris_properties *mpris_properties_new(void)
{
    struct mpris_properties *properties = calloc(1, sizeof(struct mpris_properties));
    return properties;
}

static bool mpris_properties_is_playing(const struct mpris_properties *s)
{
    return (
        (NULL != s) &&
        memcmp(s->playback_status, MPRIS_PLAYBACK_STATUS_PLAYING, strlen(MPRIS_PLAYBACK_STATUS_PLAYING)) == 0
    );
}

static bool mpris_properties_is_paused(const struct mpris_properties *s)
{
    return (
        (NULL != s) &&
        memcmp(s->playback_status, MPRIS_PLAYBACK_STATUS_PAUSED, strlen(MPRIS_PLAYBACK_STATUS_PAUSED)) == 0
    );
}

static bool mpris_properties_is_stopped(const struct mpris_properties *s)
{
    return (
        (NULL != s) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_STOPPED, strlen(MPRIS_PLAYBACK_STATUS_STOPPED)) == 0
    );
}

static bool mpris_player_is_playing (const struct mpris_player *player)
{
    return mpris_properties_is_playing(&player->properties);
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

bool mpris_player_is_valid_name(char *name)
{
    return (strlen(name) > 0);
}

bool mpris_player_is_valid(const struct mpris_player  *player)
{
    return strlen(player->mpris_name) > 1 && strlen(player->name) > 0 && NULL != player->scrobbler;
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
