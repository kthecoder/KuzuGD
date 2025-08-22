#ifndef KUZUGD_H
#define KUZUGD_H

#include "kuzu.hpp" // SOURCE : https://github.com/kuzudb/kuzu/releases/tag/v0.11.2

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <chrono>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string>

using namespace godot;
using namespace std;
using namespace kuzu::main; // EXAMPLE : https://github.com/kuzudb/kuzu/blob/master/examples/cpp/main.cpp
using kuzu::common::LogicalTypeID;

class KuzuGD : public Node {
	GDCLASS(KuzuGD, Node);

protected:
	static void _bind_methods();

private:
	SystemConfig config;

	std::unique_ptr<kuzu::main::Database> myKuzuDB;
	std::unique_ptr<kuzu::main::Connection> dbConnection;

	/********************************************

		Helper Functions

	********************************************/

	struct ParsedTime {
		tm tm_value{};
		int64_t tz_offset_microseconds = 0;
		int64_t nanoseconds = 0;
	};

	bool is_date(const string &value);
	bool is_timestamp(const string &value);
	bool is_timestamp_tz(const string &value);
	bool is_timestamp_ns(const string &value);
	bool is_interval(const string &value);
	void string_to_tm(const string &time_str, ParsedTime &timeStruct);

public:
	KuzuGD();
	~KuzuGD();

	/********************************************

		System Configuration Functions

			Define the Databases Setup Configuration

	********************************************/

	void set_buffer_pool_size(uint64_t size);
	uint64_t get_buffer_pool_size() const;

	void set_max_num_threads(uint64_t threads);
	uint64_t get_max_num_threads() const;

	void set_enable_compression(bool enabled);
	bool get_enable_compression() const;

	void set_read_only(bool readonly);
	bool get_read_only() const;

	void set_max_db_size(uint64_t size);
	uint64_t get_max_db_size() const;

	void set_auto_checkpoint(bool enabled);
	bool get_auto_checkpoint() const;

	void set_checkpoint_threshold(uint64_t threshold);
	uint64_t get_checkpoint_threshold() const;

	Array get_config() const;

	/********************************************

		Management Functions

	********************************************/

	bool query_timeout(int timeout_millis);

	bool interrupt_connection();

	int storage_version();

	godot::String get_kuzu_version();

	/********************************************

		Initialization

			Init the DB, assume Configuration is Chosen

	********************************************/
	// @return The Success or Failure of the operation
	bool kuzu_init(const String database_path);
	/*
		Kuzu_Connect :
			Set's up a connection for Queries to the DB

		@return The Success or Failure of the operation
	*/
	bool kuzu_connect(int num_threads);

	// @return Success or Failure of operation
	bool connection_set_max_threads(int num_threads);
	int connection_get_max_threads();

	/********************************************

		Query

			Database Query Operations

	********************************************/

	String query(const String &query);

	Array queryAsArray(const std::string &myQuery);

	Array queryWithParams(const String &query, const Dictionary &params);

	Ref<GDPreparedStatement> prepare(const String &query);

	Ref<GDPreparedStatement> prepareWithParams(const String &query, const Dictionary &params);

	Array executeWithParams(const Ref<GDPreparedStatement> &wrapper, const Dictionary &params);

	/********************************************

		Query Summary

			Query execution time, plan, compiling time.

	********************************************/

	Ref<GDQuerySummary> KuzuGD::query_with_summary(const String &query);
};

/********************************************

	Prepared Statement Wrapper

********************************************/

class GDPreparedStatement : public godot::RefCounted {
	GDCLASS(GDPreparedStatement, godot::RefCounted);

private:
	std::unique_ptr<kuzu::main::PreparedStatement> stmt;

protected:
	static void _bind_methods() {}

public:
	void set_statement(std::unique_ptr<kuzu::main::PreparedStatement> s) {
		stmt = std::move(s);
	}

	kuzu::main::PreparedStatement *get_statement() const {
		return stmt.get();
	}
};

/********************************************

	Prepared Summary Wrapper

********************************************/

class GDPreparedSummary : public godot::RefCounted {
	GDCLASS(GDPreparedSummary, godot::RefCounted);

private:
	kuzu::main::PreparedSummary prepared_summary; // holds the actual Kùzu struct

protected:
	static void _bind_methods() {
		godot::ClassDB::bind_method(D_METHOD("get_compiling_time"), &GDPreparedSummary::get_compiling_time);
		godot::ClassDB::bind_method(D_METHOD("get_statement_type"), &GDPreparedSummary::get_statement_type);
	}

public:
	GDPreparedSummary() = default;
	~GDPreparedSummary() = default;

	// Called by GDQuerySummary to populate this wrapper
	void set_summary(const kuzu::main::PreparedSummary &s) {
		prepared_summary = s;
	}

	// Expose compile time in milliseconds
	double get_compiling_time() const {
		return prepared_summary.compilingTime;
	}

	// Expose statement type as int (cast from enum)
	int get_statement_type() const {
		return static_cast<int>(prepared_summary.statementType);
	}
};

/********************************************

	Query Summary Wrapper

********************************************/

class GDQuerySummary : public godot::RefCounted {
	GDCLASS(GDQuerySummary, godot::RefCounted);

private:
	kuzu::main::QuerySummary summary;

protected:
	static void _bind_methods() {
		godot::ClassDB::bind_method(D_METHOD("get_compiling_time"), &GDQuerySummary::get_compiling_time);
		godot::ClassDB::bind_method(D_METHOD("get_execution_time"), &GDQuerySummary::get_execution_time);
		godot::ClassDB::bind_method(D_METHOD("is_explain"), &GDQuerySummary::is_explain);
		godot::ClassDB::bind_method(D_METHOD("get_statement_type"), &GDQuerySummary::get_statement_type);
	}

public:
	double get_compiling_time() const { return summary.getCompilingTime(); }

	double get_execution_time() const { return summary.getExecutionTime(); }

	/**
	 * @return true if the query is executed with EXPLAIN.
	 */
	bool is_explain() const { return summary.isExplain(); }

	int get_statement_type() const { return static_cast<int>(summary.getStatementType()); }

	void set_summary(const kuzu::main::QuerySummary &s) { summary = s; }
};

#endif