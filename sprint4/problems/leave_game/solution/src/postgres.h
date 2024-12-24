#pragma once

#include <condition_variable>
#include <pqxx/pqxx>

#include "tagged_uuid.h"

namespace postgres {

struct PlayerRecordResult {
    std::string name;
    int score{0};
    double play_time{0.0};
};

class ConnectionPool {
    using PoolType = ConnectionPool;
    using ConnectionPtr = std::shared_ptr<pqxx::connection>;

public:
    class ConnectionWrapper {
    public:
        ConnectionWrapper(std::shared_ptr<pqxx::connection>&& conn, PoolType& pool) noexcept;

        ConnectionWrapper(const ConnectionWrapper&) = delete;
        ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

        ConnectionWrapper(ConnectionWrapper&&) = default;
        ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

        pqxx::connection& operator*() const& noexcept;

        pqxx::connection& operator*() const&& = delete;

        pqxx::connection* operator->() const& noexcept;

        ~ConnectionWrapper();

    private:
        std::shared_ptr<pqxx::connection> conn_;
        PoolType* pool_;
    };

    // ConnectionFactory is a functional object returning std::shared_ptr<pqxx::connection>
    template <typename ConnectionFactory>
    ConnectionPool(size_t capacity, ConnectionFactory&& connection_factory);

    ConnectionWrapper GetConnection();

    std::optional<ConnectionWrapper> GetConnection(std::chrono::milliseconds timeout);

private:
    void ReturnConnection(ConnectionPtr&& conn);

    std::mutex mutex_;
    std::condition_variable cond_var_;
    std::vector<ConnectionPtr> pool_;
    size_t used_connections_ = 0;
};

template<typename ConnectionFactory>
ConnectionPool::ConnectionPool(size_t capacity, ConnectionFactory &&connection_factory) {
    pool_.reserve(capacity);
    for (size_t i = 0; i < capacity; ++i) {
        pool_.emplace_back(connection_factory());
    }
}

namespace detail {
struct RetiredPlayerId {};
}  // namespace detail

using RetiredPlayerId = util::TaggedUUID<detail::RetiredPlayerId>;

class RetiredPlayersRepository {
public:
    explicit RetiredPlayersRepository(pqxx::work& transaction);

    void Save(const std::string &name, int score, int play_time) const;

    [[nodiscard]] std::vector<PlayerRecordResult> Load(int start, int max_items) const;

private:
    pqxx::work& transaction_;
};

class Database {
public:
    explicit Database(std::shared_ptr<ConnectionPool> pool_ptr);

    [[nodiscard]] ConnectionPool::ConnectionWrapper GetTransaction() const;

private:
    std::shared_ptr<ConnectionPool> pool_ptr_;
};

} // namespace postgres