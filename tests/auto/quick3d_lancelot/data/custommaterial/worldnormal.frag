void MAIN()
{
    // If the area directly behind the monkey is not blue but rather something
    // dark, then WORLD_NORMAL is broken (not adjusted for double sidedness correctly)
    BASE_COLOR = vec4(WORLD_NORMAL, 1.0);
}
