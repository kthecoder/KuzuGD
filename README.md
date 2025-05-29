# Kuzu Godot

**Godot 4.4 is currently used Version**

Bindings for Kuzu in Godot

Embedded property graph database built for speed. Vector search and full-text search built in. Using Graph Query Language of Cypher.

# Overview

# Setup

## Binaries

Requires the Kuzu Binaries in the `bin/<platform>/kuzu` folders.

## Windows

1. Setup a C++ Environment
   1. Possible Setup for VS Code : https://code.visualstudio.com/docs/cpp/config-mingw

Ensure that C++ can run :

```bash
gcc --version
g++ --version
gdb --version
```

# Godot

## Godot GDExtension

[This repository uses the Godot quickstart template for GDExtension development with Godot 4.0+.](https://github.com/godotengine/godot-cpp-template)

### Contents

- godot-cpp as a submodule (`godot-cpp/`)
- (`demo/`) Godot 4.4 Project that tests the Extension
- preconfigured source files for C++ development of the GDExtension (`src/`)
- setup to automatically generate `.xml` files in a `doc_classes/` directory to be parsed by Godot as [GDExtension built-in documentation](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/gdextension_docs_system.html)

## Github

_Currently Commented Out for Base Development_

- GitHub Issues template (`.github/ISSUE_TEMPLATE.yml`)
- GitHub CI/CD workflows to publish your library packages when creating a release (`.github/workflows/builds.yml`)

This repository comes with a GitHub action that builds the GDExtension for cross-platform use. It triggers automatically for each pushed change. You can find and edit it in [builds.yml](.github/workflows/builds.yml).
After a workflow run is complete, you can find the file `godot-cpp-template.zip` on the `Actions` tab on GitHub.
