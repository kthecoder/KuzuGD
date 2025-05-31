# Kuzu Godot

Embedded property graph database built for speed. Vector search and full-text search built in. Graph Query Language of Cypher.

# Usage

## Setup

Requires a C++ compiler such as MSVC or MSYS2

### Windows

1. Setup a C++ Environment
   1. Possible Setup for VS Code : https://code.visualstudio.com/docs/cpp/config-mingw

Ensure that C++ can run :

```bash
gcc --version
g++ --version
gdb --version
```

### Building :

1. `scons use_mingw=yes` or just `scons`

## Debugging Godot Console

1. Execute the Project from the Command Line
   1. `C:\<path to Godot>\Godot 4.4.1>Godot_v4.4.1-stable_win64_console.exe --debug --path C:\<path to project>\KuzuGD\demo`

## Debugging Linux (WSL)

Install Gdb && Valgrind

1. Build for Linux
   1. `scons target=linux debug_symbols=yes`
1. Run Godot Demo Project with Debugger
   1. `gdb --args ./Godot_v4.4.1-stable_linux.x86_64 --path /<path to >/KuzuGD/demo/`
1. Memory Issues run Valgrind
   1. `valgrind --tool=memcheck --track-origins=yes ./Godot_v4.4.1-stable_linux.x86_64 --path /mnt/<path to>/KuzuGD/demo`
   1. `valgrind --tool=memcheck --track-origins=yes --show-leak-kinds=all ./Godot_v4.4.1-stable_linux.x86_64 --path /mnt/<path to>/KuzuGD/demo | grep libkuzu`

## Debugging Windows

1. `scons target=template_debug debug_symbols=yes`

Launch.json

```json
{
	"version": "0.2.0",
	"configurations": [
		{
			"type": "lldb",
			"request": "launch",
			"preLaunchTask": "build",
			"name": "Debug",
			"program": "<path to godot>/Godot 4.4.1.exe",
			"args": ["--path", "<path to demo project>/KuzuGD/demo"],
			"cwd": "${workspaceFolder}"
		}
	]
}
```

Tasks.json:

```json
{
	"version": "2.0.0",
	"tasks": [
		{
			"label": "build",
			"type": "shell",
			"command": "scons -j12 target=template_debug debug_symbols=yes"
		}
	]
}
```

# Godot GDExtension

[This repository uses the Godot quickstart template for GDExtension development with Godot 4.0+.](https://github.com/godotengine/godot-cpp-template)

## Contents

- godot-cpp as a submodule (`godot-cpp/`)
- (`demo/`) Godot 4.4 Project that tests the Extension
- preconfigured source files for C++ development of the GDExtension (`src/`)
- setup to automatically generate `.xml` files in a `doc_classes/` directory to be parsed by Godot as [GDExtension built-in documentation](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/gdextension_docs_system.html)

# Github

_Currently Commented Out for Base Development_

- GitHub Issues template (`.github/ISSUE_TEMPLATE.yml`)
- GitHub CI/CD workflows to publish your library packages when creating a release (`.github/workflows/builds.yml`)

This repository comes with a GitHub action that builds the GDExtension for cross-platform use. It triggers automatically for each pushed change. You can find and edit it in [builds.yml](.github/workflows/builds.yml).
After a workflow run is complete, you can find the file `godot-cpp-template.zip` on the `Actions` tab on GitHub.
