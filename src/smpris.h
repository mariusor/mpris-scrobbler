/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SMPRIS_H
#define MPRIS_SCROBBLER_SMPRIS_H

#define MPRIS_PLAYBACK_STATUS_PLAYING  "Playing"
#define MPRIS_PLAYBACK_STATUS_PAUSED   "Paused"
#define MPRIS_PLAYBACK_STATUS_STOPPED  "Stopped"

static struct mpris_properties *mpris_properties_new(void)
{
    struct mpris_properties *properties = calloc(1, sizeof(struct mpris_properties));
    return properties;
}

static bool mpris_properties_is_playing(const struct mpris_properties *s)
{
    return (
        (NULL != s) && _eq(MPRIS_PLAYBACK_STATUS_PLAYING, s->playback_status)
    );
}

static bool mpris_properties_is_paused(const struct mpris_properties *s)
{
    return (
        (NULL != s) && _eq(MPRIS_PLAYBACK_STATUS_PAUSED, s->playback_status)
    );
}

static bool mpris_properties_is_stopped(const struct mpris_properties *s)
{
    return (
        (NULL != s) && _eq(MPRIS_PLAYBACK_STATUS_STOPPED, s->playback_status)
    );
}

static bool mpris_player_is_playing (const struct mpris_player *player)
{
    return mpris_properties_is_playing(&player->properties);
}

static enum playback_state get_mpris_playback_status(const struct mpris_properties *p)
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

static bool mpris_player_is_valid_name(char *name)
{
    return (strlen(name) > 0);
}

static bool mpris_player_is_valid(const struct mpris_player *player)
{
    return strlen(player->mpris_name) > 1 && strlen(player->name) > 0 && NULL != player->scrobbler;
}

static bool mpris_metadata_equals(const struct mpris_metadata *s, const struct mpris_metadata *p)
{
    bool result = (
        (!_is_zero(s->title) && !_is_zero(p->title) && _eq(s->title, p->title)) &&
        (!_is_zero(s->album) && !_is_zero(p->album) && _eq(s->album, p->album)) &&
        (!_is_zero(s->artist) && !_is_zero(p->artist) && _eq(s->artist, p->artist)) &&
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

static inline bool mpris_event_changed_playback_status(const struct mpris_event *ev)
{
    return ev->loaded_state & mpris_load_property_playback_status;
}
static inline bool mpris_event_changed_track(const struct mpris_event *ev)
{
    return ev->loaded_state > mpris_load_property_position;
}
static inline bool mpris_event_changed_volume(const struct mpris_event *ev)
{
    return ev->loaded_state & mpris_load_property_volume;
}
static inline bool mpris_event_changed_position(const struct mpris_event *ev)
{
    return ev->loaded_state & mpris_load_property_position;
}

#endif // MPRIS_SCROBBLER_SMPRIS_H
