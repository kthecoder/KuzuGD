![Banner](Documentation/Assets/banner.jpg)

# Kuzu Godot

**Godot 4.4 is currently used Version**

Bindings for [Kuzu](https://github.com/kuzudb/kuzu) in [Godot](https://github.com/godotengine/godot)

Embedded property graph database built for speed. Using Graph Query Language of [Cypher](https://opencypher.org/resources/).

# Overview

1. Create an Instance of Kuzu
1. Set the Database Folder Path
1. Initialize the Database
1. Create a Connection to the DB with thread count

```gdscript

var myKuzuDB : KuzuGD = KuzuGD.new(); # Instantiate
var db_path = ProjectSettings.globalize_path("res://data/database/kuzu_db");
var success_db = myKuzuDB.kuzu_init(db_path); # Set Path
var success_connect = myKuzuDB.kuzu_connect(1); # Activate Connection

```

1. Execute Queries
1. Define Tables
1. Write Data
1. Read Data

```gdscript

# Define Tables
myKuzuDB.execute_query("CREATE NODE TABLE IF NOT EXISTS person(name STRING, age INT64, PRIMARY KEY(name));");

# Write Data
myKuzuDB.execute_query("CREATE (:person {name: 'Alice', age: 30});");

# Read Data
var queryResult : Array = myKuzuDB.execute_query("MATCH (p:person) RETURN p.*");
print(queryResult);

```

# Setup

## Building from Source

Requires the Kuzu Binaries in the `bin/<platform>/kuzu` folders, pythons Scons, Godot, g++ compiler.
