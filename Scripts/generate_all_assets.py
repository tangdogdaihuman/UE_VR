"""
generate_all_assets.py v2 — Phase 2: DataTable, Maps, Components, Interfaces
"""
import unreal

BP_ROOT = "/Game/Blueprints/"
UI = BP_ROOT + "UI/"
S1 = BP_ROOT + "Scene1/"
S2 = BP_ROOT + "Scene2/"
S3 = BP_ROOT + "Scene3/"
DATA_PATH = "/Game/Data/"
INPUT_PATH = "/Game/Input/Coriolis/"
MAP_PATH = "/Game/Maps/Coriolis/"

def ensure_dir(path):
    unreal.EditorAssetLibrary.make_directory(path)

def log(msg):
    unreal.log(msg)

def main():
    log("=" * 60)
    log("PHASE 2: DataTable, Maps, Components, Interfaces")
    log("=" * 60)

    # == 1. Create DataTable ==
    ensure_dir(DATA_PATH)
    dt_path = DATA_PATH + "DT_QuizData"
    log(f"Creating DataTable: {dt_path}")

    dt = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name="DT_QuizData",
        package_path=DATA_PATH.rstrip("/"),
        asset_class=unreal.DataTable,
        factory=unreal.DataTableFactory()
    )
    if dt:
        # Set the row struct via Python reflection
        quiz_row = unreal.load_object(None, "/Script/VR.QuizRow")
        if quiz_row:
            dt.set_editor_property("row_struct", quiz_row)

        # Populate rows
        try:
            row_names = []
            unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(dt, """[{"Name":"Scene1","Question":"地转偏向力何时最大？","Options":["低速慢转时","高速快转时"],"CorrectIndex":1,"SuccessMessage":"旋转越快、物体运动越快，地转偏向力越大","AutoAdvanceDelay":3.0},{"Name":"Scene2","Question":"地转偏向力最大的位置是？","Options":["赤道","中纬度","两极"],"CorrectIndex":2,"SuccessMessage":"北右南左赤道无，纬度越高，地转偏向力越强","AutoAdvanceDelay":3.0},{"Name":"Scene3","Question":"北半球台风逆时针、河流右岸侵蚀的核心原因是？","Options":["地球公转","地转偏向力向右作用"],"CorrectIndex":1,"SuccessMessage":"地转偏向力是地球自转引发的惯性力，塑造了诸多自然现象","AutoAdvanceDelay":3.0}]""")
            log(f"DataTable populated with 3 rows")
        except Exception as e:
            log(f"  WARNING: Could not auto-populate rows: {e}")
            log(f"  -> Populate DT_QuizData rows manually in the editor.")

        unreal.EditorAssetLibrary.save_loaded_asset(dt)
        log(f"CREATED: {dt_path}")
    else:
        log(f"FAILED: {dt_path}")

    # == 2. Create Map Levels ==
    ensure_dir(MAP_PATH)
    maps = [
        MAP_PATH + "MainMenu",
        MAP_PATH + "Scene1_DiskLevel",
        MAP_PATH + "Scene2_EarthLevel",
        MAP_PATH + "Scene3_NatureLevel",
    ]
    for m in maps:
        map_name = m.split("/")[-1]
        log(f"Creating map: {m}")
        try:
            # new_level creates and opens the level; save it immediately
            unreal.EditorLevelLibrary.new_level(m)
            unreal.EditorLevelLibrary.save_current_level()
            log(f"CREATED: {m}")
        except Exception as e:
            log(f"  WARNING: {m} - {e}")
            log(f"  -> Create manually: File > New Level > Save As '{map_name}'")

    # == 3. Implement IInteractableInterface on BP_FloatingUI_Base ==
    bp_path = UI + "BP_FloatingUI_Base"
    log(f"Adding interface to: {bp_path}")
    try:
        bp = unreal.EditorAssetLibrary.load_asset(bp_path)
        if bp:
            # Get the interface class
            iface = unreal.load_class(None, "/Script/VR.InteractableInterface")
            if iface:
                unreal.BlueprintEditorLibrary.add_interface(bp, iface)
                unreal.EditorAssetLibrary.save_loaded_asset(bp)
                log(f"  -> IInteractableInterface added to BP_FloatingUI_Base")
            else:
                log(f"  WARNING: Could not load InteractableInterface class")
        else:
            log(f"  WARNING: Could not load {bp_path}")
    except Exception as e:
        log(f"  WARNING: Interface attachment failed: {e}")
        log(f"  -> In editor: open BP_FloatingUI_Base > Class Settings > Add Interface > InteractableInterface")

    # == 4. Add WidgetComponent to BP_FloatingUI_Base ==
    log(f"Adding WidgetComponent to BP_FloatingUI_Base...")
    try:
        bp = unreal.EditorAssetLibrary.load_asset(bp_path)
        if bp:
            widget_comp = unreal.BlueprintEditorLibrary.add_component_to_blueprint(
                bp,
                unreal.WidgetComponent,
                "FloatingWidget"
            )
            if widget_comp:
                # Configure: Screen mode, draw size, collision preset
                try:
                    widget_comp.set_editor_property("widget_space", unreal.WidgetSpaceMode.SCREEN)
                    widget_comp.set_editor_property("draw_size", unreal.Vector2D(400, 200))
                    widget_comp.set_editor_property("collision_enabled", unreal.CollisionEnabled.QUERY_ONLY)
                    widget_comp.set_editor_property("collision_profile_name", "Custom_UI")
                    log(f"  -> WidgetComponent configured (Screen mode, 400x200)")
                except Exception as e2:
                    log(f"  WARNING: Component config failed: {e2}")
            unreal.EditorAssetLibrary.save_loaded_asset(bp)
            log(f"  -> WidgetComponent added")
        else:
            log(f"  WARNING: Could not load {bp_path}")
    except Exception as e:
        log(f"  WARNING: WidgetComponent failed: {e}")
        log(f"  -> In editor: Add WidgetComponent manually to BP_FloatingUI_Base")

    # == 5. Add StaticMeshComponent to scene objects ==
    mesh_targets = {
        S1 + "BP_Rotate_Disk":       "RotatingDisk",
        S1 + "BP_Scene1_Ball":       "BallMesh",
        S2 + "BP_Virtual_Earth":     "EarthMesh",
        S2 + "BP_Scene2_Ball":       "BallMesh",
    }
    for asset_path, comp_name in mesh_targets.items():
        log(f"Adding StaticMeshComponent to: {asset_path}")
        try:
            bp = unreal.EditorAssetLibrary.load_asset(asset_path)
            if bp:
                unreal.BlueprintEditorLibrary.add_component_to_blueprint(
                    bp, unreal.StaticMeshComponent, comp_name
                )
                unreal.EditorAssetLibrary.save_loaded_asset(bp)
                log(f"  -> {comp_name} added")
            else:
                log(f"  WARNING: Could not load {asset_path}")
        except Exception as e:
            log(f"  WARNING: {asset_path} - {e}")

    # == 6. Add CoriolisForceComponent to ball blueprints ==
    ball_targets = [S1 + "BP_Scene1_Ball", S2 + "BP_Scene2_Ball"]
    coriolis_comp = unreal.load_class(None, "/Script/VR.CoriolisForceComponent")
    for asset_path in ball_targets:
        log(f"Adding CoriolisForceComponent to: {asset_path}")
        try:
            bp = unreal.EditorAssetLibrary.load_asset(asset_path)
            if bp and coriolis_comp:
                # Add component by class
                unreal.BlueprintEditorLibrary.add_component_to_blueprint(
                    bp, coriolis_comp, "CoriolisForce"
                )
                unreal.EditorAssetLibrary.save_loaded_asset(bp)
                log(f"  -> CoriolisForceComponent added")
            else:
                log(f"  WARNING: Could not load {asset_path} or component class")
        except Exception as e:
            log(f"  WARNING: {asset_path} - {e}")

    log("=" * 60)
    log("PHASE 2 COMPLETE")
    log("=" * 60)

main()

