# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html
# SPDX-FileCopyrightText: © 2026 Tenstorrent AI ULC
# SPDX-License-Identifier: Apache-2.0

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import re
from pathlib import Path

project = "RV_TESTER"
copyright = "© 2026 Tenstorrent AI ULC"
author = "Tenstorrent AI ULC"

# rv_tester is a Bazel (bzlmod) module; pull the version out of MODULE.bazel
# so the docs stay in sync with the project version.
module_bazel_path = Path(__file__).parents[2] / "MODULE.bazel"
version = None
if module_bazel_path.exists():
    text = module_bazel_path.read_text(encoding="utf-8")
    match = re.search(r"""version\s*=\s*["']([^"']+)["']""", text)
    if match:
        version = match.group(1)
if version is None:
    version = "0.0.0"
release = version

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    "sphinxcontrib.mermaid",
]

exclude_patterns = ["public", "_build", "**/_templates"]
templates_path = ["_templates", "../common/_templates"]

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_rtd_theme"
# Canonical URL of the published site. Each Tenstorrent repo publishes its own
# GitHub Pages site routed under docs.tenstorrent.com/<repository-name>/.
html_baseurl = "https://docs.tenstorrent.com/rv_tester/"
# Sphinx would otherwise default to "<project> v<release> documentation", which
# renders as "RV_TESTER 0.0.0 documentation" in the browser tab. Pin the title so
# it matches the index heading and drops the placeholder version.
html_title = f"{project} Documentation"
html_logo = "common/images/tt_logo.svg"
html_favicon = "common/images/favicon.png"
html_context = {"logo_link_url": "https://docs.tenstorrent.com/"}

html_static_path = ["_static"]
# Configure the theme to keep global TOC
html_theme_options = {
    "prev_next_buttons_location": "bottom",
    # TOC options
    "collapse_navigation": False,
    "sticky_navigation": True,
    "navigation_depth": 4,
    "includehidden": True,
    "titles_only": False,
}


def setup(app):
    app.add_css_file("tt_theme.css")
