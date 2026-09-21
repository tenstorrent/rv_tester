<!--
SPDX-FileCopyrightText: © 2026 Tenstorrent AI ULC
SPDX-License-Identifier: CC-BY-4.0
-->
# Docs

This is where the documentation for `rv_tester` lives. Information about generating and viewing
documentation can be found below.

## Structure

Information about the structure of the documentation can be found in the
[Docs Architecture](ARCHITECTURE.md) guide in this directory.

## Sphinx documentation

This repo uses [Sphinx](https://www.sphinx-doc.org/) to generate HTML docs.

### Dependencies

The build needs Sphinx plus the theme and mermaid extension:

```sh
pip install sphinx sphinx-rtd-theme sphinxcontrib-mermaid
```

### Themes

To stay consistent with other TT documentation, we reuse the `tt_theme.css` and shared assets
(logo, favicon, fonts) under `docs/source/common/`.

### Build flow

The build flow can be run using `./docs/build.py`. It defaults to placing the HTML in
`docs/_build`. To dump in a different directory, for example in CI, run:

```sh
./docs/build.py --build_dir public
```

Use `--check` to treat Sphinx warnings as errors.

### Testing locally

Using the `--local_host` option launches a simple `http.server` locally so the HTML can be viewed
in a web browser. Note this cleans the build directory if it exists to avoid stale files sticking
around. It prints the link to the locally hosted docs:

```sh
./docs/build.py --local_host
```

```
The HTML pages are in docs/_build.
Starting local host server on:
        http://localhost:8888
CTRL+C to stop
```

Alternatively, manually cd into the build directory and run:

```sh
python3 -m http.server 8888 --bind 0.0.0.0
```

Then open `http://localhost:8888` to view the generated documentation.
