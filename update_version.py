import os
import subprocess
from datetime import datetime, timezone
Import("env")

# Resolve PROJECT_SRC_DIR (absolute path)
src_dir = env.subst("$PROJECT_SRC_DIR")
print(f"[update_version] PROJECT_SRC_DIR = {src_dir}")

# --- Git metadata ---
try:
    commit_count = subprocess.check_output(
        ["git", "rev-list", "--count", "HEAD"]
    ).strip().decode("utf-8")
except Exception as e:
    print(f"[update_version] Could not get commit count: {e}")
    commit_count = "0"

try:
    commit_hash = subprocess.check_output(
        ["git", "rev-parse", "--short", "HEAD"]
    ).strip().decode("utf-8")
except Exception as e:
    print(f"[update_version] Could not get commit hash: {e}")
    commit_hash = "unknown"

# --- Build date in UTC ---
build_date = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

version_content = f"""#ifndef VERSION_H
#define VERSION_H

#define APP_VERSION     "{commit_count}"
#define APP_COMMIT_HASH "{commit_hash}"
#define APP_BUILD_DATE  "{build_date}"

#endif // VERSION_H
"""

version_file_path = os.path.join(src_dir, "version.h")
print(f"[update_version] Writing version.h to {version_file_path}")

try:
    with open(version_file_path, "w") as version_file:
        version_file.write(version_content)
except Exception as e:
    print(f"[update_version] ERROR writing version.h: {e}")
