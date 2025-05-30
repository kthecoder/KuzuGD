#!/usr/bin/env python
import os
import sys
import platform

from methods import print_error


libname = "KuzuGD"
projectdir = "demo"

localEnv = Environment(tools=["default"], PLATFORM="")

kuzu_paths = {
    "windows": {
        "lib": "bin/windows/kuzu",
        "include": "bin/windows/kuzu/include",
        "libs": ["kuzu_shared"]
    },
    "linux": {
        "lib": "bin/linux/kuzu",
        "include": "bin/linux/kuzu/include",
        "libs": ["libkuzu_x86_64", "libkuzu_aarch64"]
    },
    "macos": {
        "lib": "bin/macos/kuzu",
        "include": "bin/macos/kuzu/include",
        "libs": ["libkuzu"]
    },
    "android": {
        "lib": "bin/android/kuzu",
        "include": "bin/android/kuzu/include",
        "libs": ["libkuzu"]
    }
}



# Build profiles can be used to decrease compile times.
# You can either specify "disabled_classes", OR
# explicitly specify "enabled_classes" which disables all other classes.
# Modify the example file as needed and uncomment the line below or
# manually specify the build_profile parameter when running SCons.

# localEnv["build_profile"] = "build_profile.json"

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Update(localEnv)

Help(opts.GenerateHelpText(localEnv))

env = localEnv.Clone()

if not (os.path.isdir("godot-cpp") and os.listdir("godot-cpp")):
    print_error("""godot-cpp is not available within this folder, as Git submodules haven't been initialized.
Run the following command to download godot-cpp:

    git submodule update --init --recursive""")
    sys.exit(1)

env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": customs})


env.Append(CPPPATH=["src/"])
# TODO Add Nested Folders as a new Glob
sources = [Glob("src/*.cpp"), Glob("src/Kuzu/*.cpp"), Glob("src/KuzuGD/*.cpp")]

if env["target"] in ["editor", "template_debug"]:
    try:
        doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")


kuzu_lib_name = ''
kuzu_generic_name = ''

if(env["platform"] == 'windows'): 
    env.Append(LIBPATH=["bin/windows/kuzu"])
    env.Append(LIBS=["kuzu_shared"])
    kuzu_lib_name = "kuzu_shared.dll"
    kuzu_generic_name = 'kuzu_shared.dll'

elif (env["platform"] == 'linux'):
    env.Append(LIBPATH=["bin/linux/kuzu"])
    arch = platform.machine()

    kuzu_generic_name = 'libkuzu.so'

    if arch == "x86_64":
        env.Append(LIBS=["libkuzu_x86_64"])
        kuzu_lib_name = "libkuzu_x86_64.so"
    elif arch == "aarch64":
        env.Append(LIBS=["libkuzu_aarch64"])
        kuzu_lib_name = "libkuzu_aarch64.so"

elif (env["platform"] == 'macos'):
    env.Append(LIBPATH=["bin/macos/kuzu"])
    env.Append(LIBS=["libkuzu"])
    kuzu_lib_name = "libkuzu"

    kuzu_generic_name = 'libkuzu.dylib'

elif (env["platform"] == 'android'):
    env.Append(LIBPATH=["bin/android/kuzu"])
    env.Append(LIBS=["libkuzu"])
    kuzu_lib_name = "libkuzu"

    kuzu_generic_name = 'libkuzu.so'



# .dev doesn't inhibit compatibility, so we don't need to key it.
# .universal just means "compatible with all relevant arches" so we don't need to key it.
suffix = env['suffix'].replace(".dev", "").replace(".universal", "")

lib_filename = "{}{}{}{}".format(env.subst('$SHLIBPREFIX'), libname, suffix, env.subst('$SHLIBSUFFIX'))

library = env.SharedLibrary(
    "bin/{}/{}".format(env['platform'], lib_filename),
    source=sources,
)

copy = env.Install("{}/bin/{}/".format(projectdir, env["platform"]), library)

kuzu_library_path = "bin/{}/kuzu/{}/".format(env['platform'], kuzu_lib_name)
#kuzu_copy = env.Install("{}/bin/{}/kuzu/".format(projectdir, env["platform"]), kuzu_library_path)

kuzu_copy = env.InstallAs("{}/bin/{}/{}".format(projectdir, env["platform"], kuzu_generic_name), kuzu_library_path)

if(env["platform"] == 'windows'):

    kuzu_lib_copy = env.InstallAs("{}/bin/{}/kuzu_shared.lib".format(projectdir, env["platform"]), "bin/windows/kuzu/kuzu_shared.lib")

    default_args = [library, copy, kuzu_copy, kuzu_lib_copy]
    Default(*default_args)
else :
    default_args = [library, copy, kuzu_copy]
    Default(*default_args)




