/*
 * Copyright (c) 2026 Kirill Sergeev, Nikolay Sugonyako, Andrey Agarkov, Gleb Safyannikov
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of brazier.
 *
 * brazier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * brazier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with brazier; if not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace brazier {

	using json = nlohmann::json;

	template <typename ModelType>
	class Collection : public std::vector<std::shared_ptr<ModelType>>{
	public:
		using std::vector<std::shared_ptr<ModelType>>::vector;

		Collection() = default;

		Collection(const std::vector<std::shared_ptr<ModelType>>& vec)
			: std::vector<std::shared_ptr<ModelType>>(vec) {}

		Collection(std::vector<std::shared_ptr<ModelType>>&& vec)
			: std::vector<std::shared_ptr<ModelType>>(std::move(vec)) {}

		bool save() {
			if (this->empty()) {
				return false;
			}
			return ModelType::saveMany(*this);
		}

		bool delete_() {
			if (this->empty()) {
				return false;
			}

			bool success = true;
			for (auto& item : *this) {
				try {
					item->delete_();
				}
				catch (const std::exception& e) {
					Logger::log("Failed to delete item: " + std::string(e.what()), "ERROR");
					success = false;
				}
			}
			return success;
		}

		json toJson() const {
			json j = json::array();
			for (const auto& item : *this) {
				j.push_back(item->toJson());
			}
			return j;
		}

		std::shared_ptr<ModelType> first() const {
			return this->empty() ? nullptr : this->front();
		}

		std::shared_ptr<ModelType> last() const {
			return this->empty() ? nullptr : this->back();
		}

		/*
			@brief Filters the collection based on a predicate function.
			@param predicate A function that takes a shared pointer to ModelType and returns a boolean.
			@return A new Collection containing only the elements that satisfy the predicate.
			@note This method does not modify the original collection; it returns a new filtered collection.
		*/
		Collection<ModelType> filter(std::function<bool(std::shared_ptr<ModelType>)> predicate) const {
			Collection<ModelType> result;
			std::copy_if(this->begin(), this->end(), std::back_inserter(result), predicate);
			return result;
		}

		bool all(std::function<bool(std::shared_ptr<ModelType>)> predicate) const {
			return std::all_of(this->begin(), this->end(), predicate);
		}

		bool any(std::function<bool(std::shared_ptr<ModelType>)> predicate) const {
			return std::any_of(this->begin(), this->end(), predicate);
		}

		std::shared_ptr<ModelType> find(std::function<bool(std::shared_ptr<ModelType>)> predicate) const {
			auto it = std::find_if(this->begin(), this->end(), predicate);
			return it != this->end() ? *it : nullptr;
		}

		std::vector<std::shared_ptr<ModelType>> toVector() const {
			return *this;
		}

		operator std::vector<std::shared_ptr<ModelType>>() const {
			return *this;
		}
	};
}