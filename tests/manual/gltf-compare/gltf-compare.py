#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Runs balsam twice on the same glTF asset, once through the Assimp
importer and once through the native glTF importer, and diffs the generated
QML, keyframe, and mesh output. A bring-up and review aid, not a CI gate.

Usage: gltf-compare.py <path/to/balsam> <asset.gltf> [balsam options...]
"""

import filecmp
import os
import subprocess
import sys
import tempfile


def run_balsam(balsam, asset, outdir, options, native):
    env = os.environ.copy()
    if native:
        env.pop("QT_QUICK3D_DISABLE_NATIVE_GLTF", None)
    else:
        env["QT_QUICK3D_DISABLE_NATIVE_GLTF"] = "1"
    subprocess.run([balsam, *options, "-o", outdir, asset], env=env, check=True,
                   capture_output=True)


def compare_trees(a, b):
    differences = []
    for root, _, files in os.walk(a):
        rel = os.path.relpath(root, a)
        for name in files:
            pa = os.path.join(root, name)
            pb = os.path.join(b, rel, name)
            if not os.path.exists(pb):
                differences.append(f"only in assimp output: {os.path.join(rel, name)}")
            elif not filecmp.cmp(pa, pb, shallow=False):
                differences.append(f"differs: {os.path.join(rel, name)}"
                                   f" ({os.path.getsize(pa)} vs {os.path.getsize(pb)} bytes)")
    for root, _, files in os.walk(b):
        rel = os.path.relpath(root, b)
        for name in files:
            if not os.path.exists(os.path.join(a, rel, name)):
                differences.append(f"only in native output: {os.path.join(rel, name)}")
    return differences


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    balsam, asset, options = sys.argv[1], sys.argv[2], sys.argv[3:]

    with tempfile.TemporaryDirectory() as tmp:
        out_assimp = os.path.join(tmp, "assimp")
        out_native = os.path.join(tmp, "native")
        os.makedirs(out_assimp)
        os.makedirs(out_native)
        run_balsam(balsam, asset, out_assimp, options, native=False)
        run_balsam(balsam, asset, out_native, options, native=True)

        differences = compare_trees(out_assimp, out_native)
        if not differences:
            print(f"{asset}: outputs are identical")
            return 0
        print(f"{asset}: {len(differences)} difference(s)")
        for diff in differences:
            print(f"  {diff}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
