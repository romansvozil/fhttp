#pragma once

#include <unordered_map>
#include <optional>
#include <chrono>
#include <thread>

namespace example_states {

struct fake_redis_manager {
    std::unordered_map<std::string, std::string> cache;

    std::optional<std::string> get(const std::string& key) {
        if (cache.find(key) == cache.end()) {
            return std::nullopt;
        }
        return cache[key];
    }

    void set(const std::string& key, const std::string& value) {
        cache[key] = value;
    }
};

struct fake_sql_manager {
    struct profile {
        std::string name;
        std::string email;
    };

    /// @brief Create and get profile by name
    /// @param name 
    /// @return profile
    std::optional<profile> create_profile(const std::string& name) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
     
        return profile { name, name + "@example.com" };
    }
};

using views_shared_state = std::tuple<example_states::fake_redis_manager, example_states::fake_sql_manager>;

} // namespace example_states

/// @brief Example of creating a state for the server
namespace fhttp {
    template <>
    std::optional<example_states::fake_redis_manager> create_state(const server_config& config) {
        FHTTP_LOG(INFO) << "Creating state for fake_redis_manager";
        
        if (config.redis_connection_string.empty()) {
            return std::nullopt;
        }

        return example_states::fake_redis_manager {};
    }
}
