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

    std::vector<profile> get_all_profiles() {
        // Simulate reading from a database
        return {
            { "User1", "user1@gmail.com" },
            { "User2", "user2@gmail.com" },
            { "User3", "user3@gmail.com" },
            { "User4", "user4@gmail.com" },
            { "User5", "user5@gmail.com" },
            { "User6", "user6@gmail.com" },
            { "User7", "user7@gmail.com" },
            { "User8", "user8@gmail.com" },
            { "User9", "user9@gmail.com" },
            { "User10", "user10@gmail.com" }
        };
    }
};

struct fake_prometheus_manager {
    void increment_request_count(
        const std::string& path,
        const std::string& method,
        int status_code
    ) {
        FHTTP_UNUSED(path);
        FHTTP_UNUSED(method);
        FHTTP_UNUSED(status_code);
    }
};

using views_shared_state = std::tuple<example_states::fake_redis_manager, example_states::fake_sql_manager, example_states::fake_prometheus_manager>;

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
