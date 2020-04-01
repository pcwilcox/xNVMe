#!/usr/bin/env python
"""
    Produce 3p:ver strings from xNVMe repos

    When running from shell, return 0 on success, some other value otherwise

"""
import subprocess
import argparse
import glob
import sys
import os

def expand_path(path):
    """Expands variables from the given path and turns it into absolute path"""

    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))

def setup():
    """Parse command-line arguments"""

    prsr = argparse.ArgumentParser(
        description="Update 3p version-strings in the given xNVMe repos",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    prsr.add_argument(
        "--repos",
        help="Path to root of the xNVMe repository",
        required=True
    )
    args = prsr.parse_args()
    args.repos = expand_path(args.repos)

    return args

def run(cmd):
    """Execute the given command"""

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        encoding="UTF-8"
    )

    out, err = proc.communicate()

    return out, err, proc.returncode

def git_head_rev_name(repos):
    """Return the git branch of given repository"""

    return run([
        "git", "-C", str(repos), "rev-parse", "--abbrev-ref", "HEAD"
    ])

def git_head_rev_short(repos):
    """Return the git branch of given repository"""

    return run([
        "git", "-C", str(repos), "rev-parse", "--short", "HEAD"
    ])

def git_describe(repos):
    """Try running git describe on the given repos"""

    return run([
        "git", "-C", str(repos), "describe", "--tags", "--abbrev=8"
    ])

def gen_description(project):
    """Produce a string which identifies the given project and its version"""

    if not os.path.exists(project["path"]["repos"]):    # Info from repos
        return None

    out, _, rcode = git_describe(project["path"]["repos"])
    if not rcode:
        return "git-describe:%s" % out.strip()

    descr = []

    out, _, rcode = git_head_rev_name(project["path"]["repos"])
    if not rcode:
        descr.append("%s" % out.strip())

    out, _, rcode = git_head_rev_short(project["path"]["repos"])
    if not rcode:
        descr.append("%s" % out.strip())

    return "git-rev:%s" % "/".join(descr) if descr else None

def traverse_projects(args):
    """Traverse third-party projects / repositories"""

    dirname = os.path.join(args.repos, "third-party")
    for path in sorted(f.path for f in os.scandir(dirname) if f.is_dir()):
        vfields = ["name", "descr", "patches"]

        project = dict.fromkeys(vfields, "unknown")
        project["name"] = os.path.basename(path)
        project["path"] = {
            "repos": os.path.join(path, "repos"),
            "patches": os.path.join(path, "patches"),
        }
        project["patches"] = "+patches" if len(glob.glob(
            os.path.join(project["path"]["patches"], "*.patch")
        )) > 0 else ""

        descr = gen_description(project)
        if descr:
            project["descr"] = descr

        project["ver"] = ";".join((
            project[field] for field in vfields if project[field]
        ))

        yield project, descr is None

def ver_to_file(args, project):
    """Dumps the 3p-versions-string of the given project to file"""

    symb = "xnvme_3p_%s" % project["name"]
    fname = "%s.c" % symb
    fpath = os.path.join(args.repos, "src", "xnvme_3p", fname)

    code = 'static const char %s[] = "%s";' % (symb, project["ver"])

    with open(fpath, "wt") as vfd:
        vfd.write(code)

    return True

def update(args):
    """Traverse third-party repos / submodules and emit version-strings"""

    failures = []

    for project, err in traverse_projects(args):
        print(project["name"], err)
        if err:
            failures.append(project)
            continue

        if not ver_to_file(args, project):
            print("Failed")

    return 0

def main(args):
    """Entry point"""

    try:
        update(args)
    except FileNotFoundError:
        return 1

    return 0

if __name__ == "__main__":
    sys.exit(main(setup()))