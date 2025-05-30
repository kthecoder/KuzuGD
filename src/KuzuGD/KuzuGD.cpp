#include "KuzuGD.h"

using namespace godot;

KuzuGD::KuzuGD() {
	/*
		Assumes Small DB Size by
			max_num_threads, max_db_size
	*/

	config.buffer_pool_size = 1024 * 1024 * 512; // 512MB
	config.max_num_threads = 1; // Defaults 1
	config.enable_compression = true;
	config.read_only = false;
	config.max_db_size = (1ULL << 30); // 1 GB default
	config.auto_checkpoint = true;
	config.checkpoint_threshold = 1024 * 1024 * 100; // 100MB
}

KuzuGD::~KuzuGD() {
	if (myKuzuDB) {
		kuzu_database_destroy(myKuzuDB);
		myKuzuDB = nullptr;
	}

	if (dbConnection) {
		kuzu_connection_destroy(dbConnection);
		dbConnection = nullptr;
	}
}

/******************************************************************


	System Configuration Functions

		Define the Databases Setup Configuration


******************************************************************/

void KuzuGD::set_buffer_pool_size(uint64_t size) {
	if (size > 0) {
		config.buffer_pool_size = size;
	}
}

uint64_t KuzuGD::get_buffer_pool_size() const {
	return config.buffer_pool_size;
}

void KuzuGD::set_max_num_threads(uint64_t threads) {
	if (threads > 0) {
		config.max_num_threads = threads;
	}
}

uint64_t KuzuGD::get_max_num_threads() const {
	return config.max_num_threads;
}

void KuzuGD::set_enable_compression(bool enabled) {
	config.enable_compression = enabled;
}
bool KuzuGD::get_enable_compression() const {
	return config.enable_compression;
}

void KuzuGD::set_read_only(bool readonly) {
	config.read_only = readonly;
}
bool KuzuGD::get_read_only() const {
	return config.read_only;
}

void KuzuGD::set_max_db_size(uint64_t size) {
	if (size != -1u) {
		config.max_db_size = size;
	}
}
uint64_t KuzuGD::get_max_db_size() const {
	return config.max_db_size;
}

void KuzuGD::set_auto_checkpoint(bool enabled) {
	config.auto_checkpoint = enabled;
}
bool KuzuGD::get_auto_checkpoint() const {
	return config.auto_checkpoint;
}

void KuzuGD::set_checkpoint_threshold(uint64_t threshold) {
	if (threshold >= 0) {
		config.checkpoint_threshold = threshold;
	}
}
uint64_t KuzuGD::get_checkpoint_threshold() const {
	return config.checkpoint_threshold;
}

Array KuzuGD::get_config() const {
	Array config_array;

	config_array.append(config.buffer_pool_size);
	config_array.append(config.max_num_threads);
	config_array.append(config.enable_compression);
	config_array.append(config.read_only);
	config_array.append(config.max_db_size);
	config_array.append(config.auto_checkpoint);
	config_array.append(config.checkpoint_threshold);

	return config_array;
}

/******************************************************************


	Initialization

		Init the DB, assume Configuration is Chosen


******************************************************************/

bool KuzuGD::kuzu_init(const String database_path) {
	if (database_path.is_empty()) {
		UtilityFunctions::push_error("KuzuGD Error | Database path is empty");
		return false;
	}

	myKuzuDB = (kuzu_database *)malloc(sizeof(kuzu_database));
	if (!myKuzuDB) {
		UtilityFunctions::push_error("KuzuGD Error | Memory allocation failed for KuzuDB Init");
		return false;
	}
	myKuzuDB->_database = nullptr;
	memset(myKuzuDB, 0, sizeof(kuzu_database));

	kuzu_state state = kuzu_database_init(
			database_path.utf8().get_data(),
			config,
			myKuzuDB);

	return state == KuzuSuccess;
}

bool KuzuGD::kuzu_connect(int num_threads) {
	dbConnection = (kuzu_connection *)malloc(sizeof(kuzu_connection));
	if (!dbConnection) {
		UtilityFunctions::push_error("KuzuGD Error | Memory allocation failed for KuzuDB Connect");
		return false;
	}
	memset(dbConnection, 0, sizeof(kuzu_connection));

	kuzu_state state = kuzu_connection_init(
			myKuzuDB,
			dbConnection);

	if (state == KuzuSuccess) {
		// Set the Thread Count for Queries
		kuzu_connection_set_max_num_thread_for_exec(
				dbConnection,
				num_threads);
	}

	return state == KuzuSuccess;
}

bool KuzuGD::connection_set_max_threads(int num_threads) {
	kuzu_state state = kuzu_connection_set_max_num_thread_for_exec(
			dbConnection,
			num_threads);

	return state == KuzuSuccess;
}

int KuzuGD::connection_get_max_threads() {
	uint64_t result = 0;
	kuzu_state state = kuzu_connection_get_max_num_thread_for_exec(
			dbConnection,
			&result);

	if (state == KuzuSuccess) {
		return static_cast<int>(result);
	} else {
		return -1;
	}
}

/******************************************************************


	Query

		Database Query Operations


******************************************************************/

Array KuzuGD::execute_query(const String &query) {
	kuzu_query_result result;
	kuzu_state state = kuzu_connection_query(dbConnection, query.utf8().get_data(), &result);

	if (state != KuzuSuccess || !&result) {
		return Array();
	}

	Array result_array;

	while (true) {
		kuzu_flat_tuple row;
		if (kuzu_query_result_get_next(&result, &row) != KuzuSuccess || !&row) {
			break;
		}

		Dictionary row_dict;

		for (uint32_t i = 0; i < kuzu_query_result_get_num_columns(&result); i++) {
			char *column_name;
			if (kuzu_query_result_get_column_name(&result, i, &column_name) == KuzuSuccess) {
				kuzu_value value;
				if (kuzu_flat_tuple_get_value(&row, i, &value) == KuzuSuccess) {
					row_dict[String(column_name)] = kuzu_value_to_string(&value);
				}
			}
		}

		result_array.append(row_dict);
	}

	return result_array;
}

Array KuzuGD::execute_prepared_query(const String &query, const Dictionary &params) {
	kuzu_prepared_statement stmt;
	if (kuzu_connection_prepare(dbConnection, query.utf8().get_data(), &stmt) != KuzuSuccess) {
		return Array(); // Return empty array if preparation fails
	}

	// Bind parameters
	for (int i = 0; i < params.size(); i++) {
		String key = params.keys()[i];
		Variant value = params.values()[i];

		// Godot does not support the same types as Kuzu
		switch (value.get_type()) {
			case Variant::INT: {
				kuzu_prepared_statement_bind_int64(&stmt, key.utf8().get_data(), value.operator int64_t());
				break;
			}

			case Variant::STRING: {
				string string_value = value.operator String().utf8().get_data();

				if (is_date(string_value)) {
					kuzu_date_t *kuzuDate = nullptr;

					kuzu_date_from_string(string_value.c_str(), kuzuDate);
					kuzu_prepared_statement_bind_date(
							&stmt, key.utf8().get_data(),
							*kuzuDate);
				}

				else if (is_timestamp_tz(string_value)) {
					ParsedTime tmp_time;
					string_to_tm(string_value, tmp_time);

					kuzu_timestamp_tz_t kuzu_ts;
					kuzu_state state = kuzu_timestamp_tz_from_tm(tmp_time.tm_value, &kuzu_ts);
					kuzu_ts.value += tmp_time.tz_offset_microseconds;

					if (state == KuzuSuccess) {
						kuzu_prepared_statement_bind_timestamp_tz(
								&stmt, key.utf8().get_data(),
								kuzu_ts);
					} else {
						UtilityFunctions::print("TimeStamp Format is not a Time Zone");
					}
				}

				else if (is_timestamp(string_value)) {
					ParsedTime tmp_time;
					string_to_tm(string_value, tmp_time);

					kuzu_timestamp_t kuzu_ts;
					kuzu_state state = kuzu_timestamp_from_tm(tmp_time.tm_value, &kuzu_ts);

					if (state == KuzuSuccess) {
						kuzu_prepared_statement_bind_timestamp(
								&stmt, key.utf8().get_data(),
								kuzu_ts);
					} else {
						UtilityFunctions::print("TimeStamp Format is not a Time Zone");
					}
				}

				else if (is_interval(string_value)) {
					kuzu_prepared_statement_bind_interval(
							&stmt, key.utf8().get_data(),
							string_to_kuzu_interval(string_value));
				}

				else {
					kuzu_prepared_statement_bind_string(&stmt, key.utf8().get_data(), string_value.c_str());
				}

				break;
			}

			case Variant::BOOL: {
				kuzu_prepared_statement_bind_bool(&stmt, key.utf8().get_data(), value.operator bool());
				break;
			}

			case Variant::FLOAT: {
				kuzu_prepared_statement_bind_float(&stmt, key.utf8().get_data(), value.operator float());
				break;
			}

			default:
				UtilityFunctions::push_error("KuzuGD Error | Prepared Query - Unsupported data type for binding: " + key);
		}
	}

	// Execute the query
	kuzu_query_result result;
	if (kuzu_connection_execute(dbConnection, &stmt, &result) != KuzuSuccess) {
		return Array(); // Return empty array if execution fails
	}

	// Process result
	Array result_array;
	while (true) {
		kuzu_flat_tuple row;
		if (kuzu_query_result_get_next(&result, &row) != KuzuSuccess || &row == nullptr) {
			break;
		}

		Dictionary row_dict;
		for (uint32_t i = 0; i < kuzu_query_result_get_num_columns(&result); i++) {
			char *column_name;
			if (kuzu_query_result_get_column_name(&result, i, &column_name) == KuzuSuccess) {
				kuzu_value value;
				if (kuzu_flat_tuple_get_value(&row, i, &value) == KuzuSuccess) {
					row_dict[String(column_name)] = kuzu_value_to_string(&value);
				}
			}
		}
		result_array.append(row_dict);
	}

	return result_array;
}

/******************************************************************


	Helper Functions


******************************************************************/

bool KuzuGD::is_date(const string &value) {
	regex date_regex(R"(^\d{4}-\d{2}-\d{2}$)");
	return regex_match(value, date_regex);
}

bool KuzuGD::is_timestamp(const string &value) {
	regex timestamp_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$)");
	return regex_match(value, timestamp_regex);
}

bool KuzuGD::is_timestamp_tz(const string &value) {
	regex timestamp_tz_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2} [+-]\d{2}:\d{2}$)");
	return regex_match(value, timestamp_tz_regex);
}

bool KuzuGD::is_timestamp_ns(const string &value) {
	regex timestamp_ns_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{9}$)");
	return regex_match(value, timestamp_ns_regex);
}

bool KuzuGD::is_interval(const string &value) {
	regex interval_regex(R"(^\d+ (YEAR|MONTH|DAY|HOUR|MINUTE|SECOND|MICROSECOND)(, .*)?$)");
	return regex_match(value, interval_regex);
}

void KuzuGD::string_to_tm(const string &time_str, ParsedTime &timeStruct) {
	regex date_regex(R"(^\d{4}-\d{2}-\d{2}$)");
	regex timestamp_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$)");
	regex timestamp_ns_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.(\d{1,9})$)");
	regex timestamp_tz_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2} ([+-]\d{2}:\d{2})$)");

	tm out_tm = {};
	int64_t tz_offset_microseconds = 0, nanoseconds = 0;
	istringstream ss(time_str);
	string tz_offset_str, nano_str;
	char dash1, dash2, space;

	if (regex_match(time_str, date_regex)) {
		ss >> out_tm.tm_year >> dash1 >> out_tm.tm_mon >> dash2 >> out_tm.tm_mday;
		out_tm.tm_hour = 0;
		out_tm.tm_min = 0;
		out_tm.tm_sec = 0;
	} else if (regex_match(time_str, timestamp_regex)) {
		ss >> out_tm.tm_year >> dash1 >> out_tm.tm_mon >> dash2 >> out_tm.tm_mday >> space >> out_tm.tm_hour >> dash1 >> out_tm.tm_min >> dash2 >> out_tm.tm_sec;
	} else if (regex_match(time_str, timestamp_ns_regex)) {
		ss >> out_tm.tm_year >> dash1 >> out_tm.tm_mon >> dash2 >> out_tm.tm_mday >> space >> out_tm.tm_hour >> dash1 >> out_tm.tm_min >> dash2 >> out_tm.tm_sec >> space >> nano_str;

		// Convert nanoseconds
		while (nano_str.length() < 9)
			nano_str += "0"; // Normalize to 9 digits
		nanoseconds = stoll(nano_str);
	} else if (regex_match(time_str, timestamp_tz_regex)) {
		ss >> out_tm.tm_year >> dash1 >> out_tm.tm_mon >> dash2 >> out_tm.tm_mday >> space >> out_tm.tm_hour >> dash1 >> out_tm.tm_min >> dash2 >> out_tm.tm_sec >> space >> tz_offset_str;

		char sign = tz_offset_str[0];
		int tz_hour = stoi(tz_offset_str.substr(1, 2));
		int tz_minute = stoi(tz_offset_str.substr(4, 2));

		tz_offset_microseconds = (tz_hour * 3600 + tz_minute * 60) * 1000000;
		if (sign == '-') {
			tz_offset_microseconds = -tz_offset_microseconds;
		}
	} else {
		UtilityFunctions::push_error("string_to_tm -> Invalid Time Format!");
	}

	// Adjust for `tm` structure
	out_tm.tm_year -= 1900;
	out_tm.tm_mon -= 1;

	timeStruct = { out_tm, tz_offset_microseconds, nanoseconds };
}

kuzu_interval_t KuzuGD::string_to_kuzu_interval(const string &interval_str) {
	int32_t months = 0, days = 0;
	int64_t micros = 0;
	istringstream ss(interval_str);
	string token;

	while (getline(ss, token, ',')) {
		istringstream part(token);
		int value;
		string unit;

		part >> value >> unit;

		if (unit.find("year") != string::npos) {
			months += value * 12; // Convert years to months
		} else if (unit.find("month") != string::npos) {
			months += value;
		} else if (unit.find("day") != string::npos) {
			days += value;
		} else if (unit.find("microsecond") != string::npos) {
			micros += value;
		} else if (unit.find("millisecond") != string::npos) {
			micros += value * 1000;
		} else if (unit.find("second") != string::npos) {
			micros += value * 1000000;
		}
	}

	return kuzu_interval_t{ months, days, micros };
}

/******************************************************************


	GDExtension Bind Methods

		Bind the Middleware Wrapper Functions for GDSCript usage


******************************************************************/

void KuzuGD::_bind_methods() {
	/********************************************

		System Configuration Functions

			Define the Databases Setup Configuration

	********************************************/
	ClassDB::bind_method(D_METHOD("set_buffer_pool_size", "size"), &KuzuGD::set_buffer_pool_size);
	ClassDB::bind_method(D_METHOD("get_buffer_pool_size"), &KuzuGD::get_buffer_pool_size);
	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "buffer_pool_size"), "set_buffer_pool_size", "get_buffer_pool_size");

	ClassDB::bind_method(D_METHOD("set_max_num_threads", "threads"), &KuzuGD::set_max_num_threads);
	ClassDB::bind_method(D_METHOD("get_max_num_threads"), &KuzuGD::get_max_num_threads);
	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "max_num_threads"), "set_max_num_threads", "get_max_num_threads");

	ClassDB::bind_method(D_METHOD("set_enable_compression", "enabled"), &KuzuGD::set_enable_compression);
	ClassDB::bind_method(D_METHOD("get_enable_compression"), &KuzuGD::get_enable_compression);
	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL, "enable_compression"), "set_enable_compression", "get_enable_compression");

	ClassDB::bind_method(D_METHOD("set_read_only", "readonly"), &KuzuGD::set_read_only);
	ClassDB::bind_method(D_METHOD("get_read_only"), &KuzuGD::get_read_only);
	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL, "read_only"), "set_read_only", "get_read_only");

	ClassDB::bind_method(D_METHOD("set_max_db_size", "size"), &KuzuGD::set_max_db_size);
	ClassDB::bind_method(D_METHOD("get_max_db_size"), &KuzuGD::get_max_db_size);
	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "max_db_size"), "set_max_db_size", "get_max_db_size");

	ClassDB::bind_method(D_METHOD("set_auto_checkpoint", "enabled"), &KuzuGD::set_auto_checkpoint);
	ClassDB::bind_method(D_METHOD("get_auto_checkpoint"), &KuzuGD::get_auto_checkpoint);
	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL, "auto_checkpoint"), "set_auto_checkpoint", "get_auto_checkpoint");

	ClassDB::bind_method(D_METHOD("set_checkpoint_threshold", "threshold"), &KuzuGD::set_checkpoint_threshold);
	ClassDB::bind_method(D_METHOD("get_checkpoint_threshold"), &KuzuGD::get_checkpoint_threshold);
	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "checkpoint_threshold"), "set_checkpoint_threshold", "get_checkpoint_threshold");

	ClassDB::bind_method(D_METHOD("get_config"), &KuzuGD::get_config);

	/********************************************

		Initialization

	********************************************/

	ClassDB::bind_method(D_METHOD("kuzu_init", "database_path"), &KuzuGD::kuzu_init);

	ClassDB::bind_method(D_METHOD("kuzu_connect", "num_threads"), &KuzuGD::kuzu_connect);

	ClassDB::bind_method(D_METHOD("connection_set_max_threads", "num_threads"), &KuzuGD::connection_set_max_threads);

	ClassDB::bind_method(D_METHOD("connection_get_max_threads"), &KuzuGD::connection_get_max_threads);

	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "num_threads"), "connection_set_max_threads", "connection_get_max_threads");

	/********************************************

		Query

	********************************************/

	ClassDB::bind_method(D_METHOD("execute_query", "query"), &KuzuGD::execute_query);
	ClassDB::bind_method(D_METHOD("execute_prepared_query", "query", "params"), &KuzuGD::execute_prepared_query);
}