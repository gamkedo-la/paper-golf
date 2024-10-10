# Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

import zipfile
import sys
import os
import re

binary_paths = [
    "Binaries",
    "Plugins/MultiplayerSessions/Binaries"
]

include = [
    "UnrealEditor-PG\\w+(?!Debug)\\.dll"
    , "UnrealEditor-PaperGolf\\w*(?!Debug)\\.dll"
    , "UnrealEditor\.modules"
    , "PaperGolfEditor\.target"
    , "UnrealEditor-MultiplayerSessions.dll"
]

def zipdirs(ziph, paths, exclusions = None, inclusions = None):
    # Do not include the folder being zipped in the archive itself
    # If we want to do this we could use parent path below for zip_root instead of input path
    # zip_root = os.path.join(path, '..')
    zip_root = "."
    # Include multiple paths in the zip file
    for path in paths:
        for root, dirs, files in os.walk(path):
            for file in files:
                if (inclusions and any(re.match(includeRegex, file) for includeRegex in inclusions)) or (exclusions and not any(re.match(excludeRegex, file) for excludeRegex in exclusions)):
                    file_path = os.path.join(root, file)
                    zip_path = os.path.relpath(file_path, zip_root)
                    print("Writing " + zip_path)
                    ziph.write(file_path, zip_path)


######################### MAIN SCRIPT ##################################

if len(sys.argv) != 2:
    print("Usage: [Output Zip File]")
    sys.exit(1)

zip_file = sys.argv[1]

print("Zipping editor binaries to " + zip_file)

ziph = zipfile.ZipFile(zip_file, 'w', zipfile.ZIP_DEFLATED)

zipdirs(ziph, binary_paths, inclusions = include)

ziph.close()

print()
print("Zip created successfully: " + zip_file)
