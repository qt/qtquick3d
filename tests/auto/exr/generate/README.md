<!--
Copyright (C) 2026 The Qt Company Ltd.
SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only
-->

# Regenerating the EXR test fixtures

`generate-exr-fixtures.cpp` writes the fixtures in `../data` that
`tst_qssgexrreader` cannot build itself: PIZ, the tiled layouts, the level
modes and the rejected codecs. It is not part of the build and Qt does not
depend on OpenEXR.

Build it against a local OpenEXR 3.x, for example a Homebrew one:

```
clang++ -std=c++17 -O1 generate-exr-fixtures.cpp \
  -I/opt/homebrew/opt/openexr/include/OpenEXR \
  -I/opt/homebrew/opt/imath/include/Imath \
  -L/opt/homebrew/opt/openexr/lib -L/opt/homebrew/opt/imath/lib \
  -lOpenEXR -lIex -lIlmThread -lImath \
  -o generate-exr-fixtures
./generate-exr-fixtures ../data
```

The pixel values are `(x * 7 + y * 13 + c * 29) / 256`, exactly representable
in both half and float, so the test compares without a tolerance.
