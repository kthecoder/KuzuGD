#ifndef KUZUGD_H
#define KUZUGD_H

#include "kuzu.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <chrono>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string>

using namespace godot;
using namespace std;

class KuzuGD : public Node {
	GDCLASS(KuzuGD, Node);

protected:
	static void _bind_methods();

private:
	kuzu_system_config config;

	kuzu_database *myKuzuDB;
	kuzu_connection *dbConnection;

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
	kuzu_interval_t string_to_kuzu_interval(const string &interval_str);

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

	void interrupt_connection();

	int storage_version();

	String get_kuzu_version();

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

	Array execute_query(const String &query);

	Array execute_prepared_query(const String &query, const Dictionary &params);

	int query_columns_count(const String &query);

	String query_column_name(const String &query, int colIndex);

	String query_column_data_type(const String &query, int colIndex);

	int query_num_tuples(const String &query);

	float query_summary_compile_time(const String &query);

	float query_summary_execution_time(const String &query);
};

#endif