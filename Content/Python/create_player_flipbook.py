import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

sprite_folder = "/Game/PlayerAnimations"
flipbook_path = "/Game/PlayerAnimations/FB_Player_Test"

sprites = []

for i in range(8):
    path = f"{sprite_folder}/pixel_character_dark_blue_Sprite_{i}"
    sprite = unreal.EditorAssetLibrary.load_asset(path)

    if sprite:
        sprites.append(sprite)
        unreal.log(f"Loaded sprite {i}")
    else:
        unreal.log_warning(f"Could not load sprite {i}")

if not sprites:
    unreal.log_error("No sprites were loaded.")
else:
    flipbook = unreal.EditorAssetLibrary.load_asset(flipbook_path)

    if not flipbook:
        factory = unreal.PaperFlipbookFactory()

        flipbook = asset_tools.create_asset(
            "FB_Player_Test",
            sprite_folder,
            unreal.PaperFlipbook,
            factory
        )

    key_frames = []

    for sprite in sprites:
        key_frame = unreal.PaperFlipbookKeyFrame()
        key_frame.set_editor_property("sprite", sprite)
        key_frame.set_editor_property("frame_run", 1)
        key_frames.append(key_frame)

    flipbook.set_editor_property("key_frames", key_frames)
    flipbook.set_editor_property("frames_per_second", 8.0)

    unreal.EditorAssetLibrary.save_asset(flipbook_path)

    unreal.log(f"Created flipbook with {len(key_frames)} frames.")