/**
 * EQEmulator: Everquest Server Emulator
 * Copyright (C) 2001-2020 EQEmulator Development Team (https://github.com/EQEmu/Server)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY except by those people which sell it, which
 * are required to give you total support for your newly bought product;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef EQEMU_TRIBUTE_LEVELS_REPOSITORY_H
#define EQEMU_TRIBUTE_LEVELS_REPOSITORY_H

#include "../database.h"
#include "../string_util.h"

class TributeLevelsRepository {
public:
	struct TributeLevels {
		int tribute_id;
		int level;
		int cost;
		int item_id;
	};

	static std::string PrimaryKey()
	{
		return std::string("level");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"tribute_id",
			"level",
			"cost",
			"item_id",
		};
	}

	static std::string ColumnsRaw()
	{
		return std::string(implode(", ", Columns()));
	}

	static std::string InsertColumnsRaw()
	{
		std::vector<std::string> insert_columns;

		for (auto &column : Columns()) {
			if (column == PrimaryKey()) {
				continue;
			}

			insert_columns.push_back(column);
		}

		return std::string(implode(", ", insert_columns));
	}

	static std::string TableName()
	{
		return std::string("tribute_levels");
	}

	static std::string BaseSelect()
	{
		return fmt::format(
			"SELECT {} FROM {}",
			ColumnsRaw(),
			TableName()
		);
	}

	static std::string BaseInsert()
	{
		return fmt::format(
			"INSERT INTO {} ({}) ",
			TableName(),
			InsertColumnsRaw()
		);
	}

	static TributeLevels NewEntity()
	{
		TributeLevels entry{};

		entry.tribute_id = 0;
		entry.level      = 0;
		entry.cost       = 0;
		entry.item_id    = 0;

		return entry;
	}

	static TributeLevels GetTributeLevelsEntry(
		const std::vector<TributeLevels> &tribute_levelss,
		int tribute_levels_id
	)
	{
		for (auto &tribute_levels : tribute_levelss) {
			if (tribute_levels.level == tribute_levels_id) {
				return tribute_levels;
			}
		}

		return NewEntity();
	}

	static TributeLevels FindOne(
		int tribute_levels_id
	)
	{
		auto results = content_db.QueryDatabase(
			fmt::format(
				"{} WHERE id = {} LIMIT 1",
				BaseSelect(),
				tribute_levels_id
			)
		);

		auto row = results.begin();
		if (results.RowCount() == 1) {
			TributeLevels entry{};

			entry.tribute_id = atoi(row[0]);
			entry.level      = atoi(row[1]);
			entry.cost       = atoi(row[2]);
			entry.item_id    = atoi(row[3]);

			return entry;
		}

		return NewEntity();
	}

	static int DeleteOne(
		int tribute_levels_id
	)
	{
		auto results = content_db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {} = {}",
				TableName(),
				PrimaryKey(),
				tribute_levels_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int UpdateOne(
		TributeLevels tribute_levels_entry
	)
	{
		std::vector<std::string> update_values;

		auto columns = Columns();

		update_values.push_back(columns[2] + " = " + std::to_string(tribute_levels_entry.cost));
		update_values.push_back(columns[3] + " = " + std::to_string(tribute_levels_entry.item_id));

		auto results = content_db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET {} WHERE {} = {}",
				TableName(),
				implode(", ", update_values),
				PrimaryKey(),
				tribute_levels_entry.level
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static TributeLevels InsertOne(
		TributeLevels tribute_levels_entry
	)
	{
		std::vector<std::string> insert_values;

		insert_values.push_back(std::to_string(tribute_levels_entry.cost));
		insert_values.push_back(std::to_string(tribute_levels_entry.item_id));

		auto results = content_db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseInsert(),
				implode(",", insert_values)
			)
		);

		if (results.Success()) {
			tribute_levels_entry.id = results.LastInsertedID();
			return tribute_levels_entry;
		}

		tribute_levels_entry = TributeLevelsRepository::NewEntity();

		return tribute_levels_entry;
	}

	static int InsertMany(
		std::vector<TributeLevels> tribute_levels_entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &tribute_levels_entry: tribute_levels_entries) {
			std::vector<std::string> insert_values;

			insert_values.push_back(std::to_string(tribute_levels_entry.cost));
			insert_values.push_back(std::to_string(tribute_levels_entry.item_id));

			insert_chunks.push_back("(" + implode(",", insert_values) + ")");
		}

		std::vector<std::string> insert_values;

		auto results = content_db.QueryDatabase(
			fmt::format(
				"{} VALUES {}",
				BaseInsert(),
				implode(",", insert_chunks)
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static std::vector<TributeLevels> All()
	{
		std::vector<TributeLevels> all_entries;

		auto results = content_db.QueryDatabase(
			fmt::format(
				"{}",
				BaseSelect()
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			TributeLevels entry{};

			entry.tribute_id = atoi(row[0]);
			entry.level      = atoi(row[1]);
			entry.cost       = atoi(row[2]);
			entry.item_id    = atoi(row[3]);

			all_entries.push_back(entry);
		}

		return all_entries;
	}

	static std::vector<TributeLevels> GetWhere(std::string where_filter)
	{
		std::vector<TributeLevels> all_entries;

		auto results = content_db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			TributeLevels entry{};

			entry.tribute_id = atoi(row[0]);
			entry.level      = atoi(row[1]);
			entry.cost       = atoi(row[2]);
			entry.item_id    = atoi(row[3]);

			all_entries.push_back(entry);
		}

		return all_entries;
	}

	static int DeleteWhere(std::string where_filter)
	{
		auto results = content_db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {}",
				TableName(),
				PrimaryKey(),
				where_filter
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

};

#endif //EQEMU_TRIBUTE_LEVELS_REPOSITORY_H