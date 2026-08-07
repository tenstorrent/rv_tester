<!--
SPDX-FileCopyrightText: © 2026 Tenstorrent AI ULC
SPDX-License-Identifier: CC-BY-4.0
-->
# Docs Architecture

This page describes the documentation architecture for developers maintaining the `rv_tester`
docs. The docs are built with Sphinx.

## Structure

The documentation lives in `docs/source` in several directories:

### `common`

Templates, images, and fonts shared with the rest of the TT documentation (the `tt_theme.css`
theme, logo, and favicon). The `source/_static` symlink points here so the theme assets are
picked up by Sphinx.

### `tutorials`

Introductory material for first-time users or people interested in the project. These explain
what `rv_tester` is, how the pieces fit together, and how to build and run it. Further reading
points into the [`user_guides`](#user_guides).

### `user_guides`

In-depth guides to the major subsystems (`cosim`, `sysmod`, `pmu`, the SW testbench) with
background info, data flow, and configuration. These largely track the per-subsystem `README`
files under `src/` and `test/`.

### `reference`

Reference material for the source tree, including the repository layout / component map.

## Adding Documentation

If you haven't used Sphinx before, [the official Sphinx user guide](https://www.sphinx-doc.org/en/master/usage/quickstart.html)
has resources for getting started. These docs consist of `.rst` files. To add a new page:

1. Add an `.rst` file to the correct section using the [structure](#structure) guide.
2. Add a reference to the new `.rst` file in the directory's `index.rst`. This links it into the
   Table of Contents Tree (`toctree`) and into the sidebar.

Where a subsystem also has a `README.md` in the source tree, keep the two consistent — the user
guide is the long-form version of the README.
