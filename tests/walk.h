/* Shared input for native movement and persistence tests. */
static bool walk_to(double x, double z) {
    double dx = x - mountain_value(0), dz = z - mountain_value(1);
    mountain_look((float)(atan2(dx, -dz) - mountain_value(2)), 0);
    int budget = (int)ceil(hypot(dx, dz) / .8) + 4;
    for (int k = 0; k < budget; k++) {
        dx = x - mountain_value(0);
        dz = z - mountain_value(1);
        double left = hypot(dx, dz);
        if (left < .00015)
            return true;
        if (left < 1)
            mountain_look((float)(atan2(dx, -dz) - mountain_value(2)), 0);
        mountain_walk((float)(fmin(left, .8) / 5.3), 1, 0);
        if (mountain_wet(mountain_value(0), mountain_value(1)))
            break;
    }
    fprintf(stderr, "Trail input to %.2f, %.2f stopped at %.2f, %.2f in %s (footing %.0f).\n", x, z,
            mountain_value(0), mountain_value(1), mountain_label(0), mountain_value(33));
    return false;
}
static char test_notes[1024];
static void isolated_journal(void) {
    SDL_Time now = 0;
    SDL_GetCurrentTime(&now);
    char *dir = SDL_GetPrefPath("Mountain", "Playtests");
    SDL_snprintf(test_notes, sizeof test_notes, "%sfield-%lld.notes", dir ? dir : "",
                 (long long)now);
    SDL_free(dir);
    SDL_setenv_unsafe("MOUNTAIN_FIELD_NOTES", test_notes, 1);
}
