import os
import subprocess
Import("env")


def update_version(source, target, env):
    # Resolve PROJECTSRC_DIR and ensure it is an absolute path
    src_dir = env.subst("$PROJECTSRC_DIR")
    print(f"Resolved PROJECTSRC_DIR: {src_dir}")

    try:
        commit_count = subprocess.check_output(
            ['git', 'rev-list', '--count', 'HEAD']).strip()
        commit_count = commit_count.decode('utf-8')
    except:
        commit_count = '0'

    try:
        commit_hash = subprocess.check_output(
            ['git', 'rev-parse', '--short', 'HEAD']).strip()
        commit_hash = commit_hash.decode('utf-8')
    except:
        commit_hash = 'unknown'

    version_content = f'''
    #ifndef VERSION_H
    #define VERSION_H

    #define APP_VERSION "{commit_count}"
    #define APP_COMMIT_HASH "{commit_hash}"

    #endif // VERSION_H
    '''

    version_file_path = os.path.join(src_dir, 'version.h')
    print(f"Writing version.h to {version_file_path}")
    with open(version_file_path, 'w') as version_file:
        version_file.write(version_content)


env.AddPreAction("buildprog", update_version)
