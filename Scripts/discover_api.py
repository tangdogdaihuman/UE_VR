"""Discover UE5 Python APIs for Blueprint manipulation — full output."""
import unreal

def list_all(obj, name):
    methods = sorted([m for m in dir(obj) if not m.startswith("_")])
    unreal.log(f"\n=== {name} ({len(methods)} methods) ===")
    for m in methods:
        unreal.log(f"  {m}")

# Find all BP-related methods
list_all(unreal.BlueprintEditorLibrary, "BlueprintEditorLibrary")

# Check Subobject subsystem
try:
    sub = unreal.SubobjectDataSubsystem()
    list_all(unreal.SubobjectDataSubsystem, "SubobjectDataSubsystem")
except:
    unreal.log("SubobjectDataSubsystem not available")

# Try loading VR module explicitly
unreal.log("\n--- Loading VR module ---")
try:
    # Try to find the module via loaded packages
    for pkg in unreal.EditorAssetLibrary.list_assets("/Script/", recursive=False):
        if "VR" in str(pkg):
            unreal.log(f"  Found: {pkg}")
    unreal.log(f"  VR.h location: checking...")
except Exception as e:
    unreal.log(f"  List error: {e}")

# Try with different class loading approaches
unreal.log("\n--- Class loading attempts ---")
for prefix in ["", "U", "A", "F"]:
    for name in ["InteractableInterface", "CoriolisForceComponent", "QuizRow", "QuizComponent"]:
        path = f"/Script/VR.{prefix}{name}"
        obj = unreal.load_class(None, path)
        if obj:
            unreal.log(f"  FOUND: {path}")

# Check if find_object works
try:
    obj = unreal.find_object(None, "/Script/VR.InteractableInterface")
    unreal.log(f"  find_object: {obj}")
except:
    pass

# Component-related methods on Blueprint
unreal.log("\n--- Component methods on BlueprintEditorLibrary ---")
for m in dir(unreal.BlueprintEditorLibrary):
    ml = m.lower()
    if any(kw in ml for kw in ["component", "interface", "add", "remove", "implement"]):
        unreal.log(f"  {m}")

unreal.log("\nDISCOVERY COMPLETE")
