# Kuzu Godot

Embedded property graph database built for speed. Vector search and full-text search built in. Graph Query Language of Cypher.

## Usage

Requires a C++ compiler such as MSVC or MSY2

### Building :

1. `scons use_mingw=yes` or just `scons`

## Debugging Godot Console

1. Execute the Project from the Command Line
   1. `C:\<path to Godot>\Godot 4.4.1>Godot_v4.4.1-stable_win64_console.exe --debug --path C:\<path to project>\KuzuGD\demo`

## Debugging Linux (WSL)

1. Build for Linux
   1. `scons target=linux debug_symbols=yes`
1. Run Godot Demo Project with Debugger
   1. `gdb --args ./Godot_v4.4.1-stable_linux.x86_64 --path /<path to >/KuzuGD/demo/`
1. Memory Issues run Valgrind
   1. `valgrind --tool=memcheck --track-origins=yes ./Godot_v4.4.1-stable_linux.x86_64 --path /mnt/<path to>/KuzuGD/demo`
   1. `valgrind --tool=memcheck --track-origins=yes --show-leak-kinds=all ./Godot_v4.4.1-stable_linux.x86_64 --path /mnt/<path to>/KuzuGD/demo | grep libkuzu`

## Debugging Windows

1. https://youtu.be/8WSIMTJWCBk?t=3624
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
