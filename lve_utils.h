#pragma once

#include <functional>

namespace lve {

	template <typename T, typename... Rest>
	void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
		seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		(hashCombine(seed, rest), ...);
	};

	struct StatusBar {
		bool reloadResources = false;
		std::string command = "";
	};

	using id_t = unsigned int;

}  // namespace lve