import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

sprite_path = "/Game/PlayerAnimations"

animations = {
    "FB_Player_Walk_Down": range(0, 8),
    "FB_Player_Walk_Up": range(8, 16),
    "FB_Player_Walk_Left": range(16, 24),
    "FB_Player_Walk_Right": range(24, 32),
}

for flipbook_name, frame_numbers in animations.items():

    sprites = []

    for number in frame_numbers:
        path = f"{sprite_path}/pixel_character_dark_blue_Sprite_{number}"
        sprite = unreal.load_asset(path)

        if sprite:
            sprites.append(sprite)
        else:
            unreal.log_warning(f"Could not load Sprite_{number}")

    if not sprites:
        unreal.log_error(f"No sprites found for {flipbook_name}")
        continue

    existing = unreal.load_asset(f"{sprite_path}/{flipbook_name}")

    if existing:
        flipbook = existing
    else:
        factory = unreal.PaperFlipbookFactory()
        flipbook = asset_tools.create_asset(
            flipbook_name,
            sprite_path,
            unreal.PaperFlipbook,
            factory
        )

    keys = []

    for sprite in sprites:
        key = unreal.PaperFlipbookKeyFrame()
        key.set_editor_property("sprite", sprite)
        key.set_editor_property("frame_run", 1)
        keys.append(key)

    flipbook.set_editor_property("key_frames", keys)
    flipbook.set_editor_property("frames_per_second", 8.0)

    unreal.EditorAssetLibrary.save_asset(flipbook.get_path_name())

    unreal.log(f"Created {flipbook_name}")

unreal.log("WALKING FLIPBOOKS DONE")
