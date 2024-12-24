#include "postgres.h"
#include "tagged_uuid.h"

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

ConnectionPool::ConnectionWrapper::
ConnectionWrapper(std::shared_ptr<pqxx::connection> &&conn, PoolType &pool) noexcept: conn_{std::move(conn)}
    , pool_{&pool} {
}

pqxx::connection & ConnectionPool::ConnectionWrapper::operator*() const & noexcept {
    return *conn_;
}

pqxx::connection * ConnectionPool::ConnectionWrapper::operator->() const & noexcept {
    return conn_.get();
}

ConnectionPool::ConnectionWrapper::~ConnectionWrapper() {
    if (conn_) {
        pool_->ReturnConnection(std::move(conn_));
    }
}

ConnectionPool::ConnectionWrapper ConnectionPool::GetConnection() {
    std::unique_lock lock{mutex_};
    // Блокируем текущий поток и ждём, пока cond_var_ не получит уведомление и не освободится
    // хотя бы одно соединение
    cond_var_.wait(lock, [this] {
        return used_connections_ < pool_.size();
    });
    // После выхода из цикла ожидания мьютекс остаётся захваченным

    return {std::move(pool_[used_connections_++]), *this};
}

std::optional<ConnectionPool::ConnectionWrapper> ConnectionPool::GetConnection(std::chrono::milliseconds timeout) {
    // Блокируем текущий поток и ждём, пока cond_var_ не получит уведомление и не освободится
    // хотя бы одно соединение
    std::unique_lock lock{mutex_};
    if (!cond_var_.wait_for(lock, timeout, [this] {
        return used_connections_ < pool_.size();
    })) {
        // Если тайм-аут истёк, возвращаем std::nullopt
        return std::nullopt;
    }
    // После выхода из цикла ожидания мьютекс остаётся захваченным
    return ConnectionWrapper{std::move(pool_[used_connections_++]), *this};
}

void ConnectionPool::ReturnConnection(ConnectionPtr &&conn) {
    // Возвращаем соединение обратно в пул
    {
        std::lock_guard lock{mutex_};
        assert(used_connections_ != 0);
        pool_[--used_connections_] = std::move(conn);
    }
    // Уведомляем один из ожидающих потоков об изменении состояния пула
    cond_var_.notify_one();
}

RetiredPlayersRepository::RetiredPlayersRepository(pqxx::work &transaction)
    : transaction_(transaction) {
}

void RetiredPlayersRepository::Save(const std::string &name, int score, int play_time) const {
    const auto id = RetiredPlayerId::New().ToString();
    transaction_.exec_params(
        R"(
                INSERT INTO retired_players (id, name, score, play_time_ms) VALUES ($1, $2, $3, $4);
        )"_zv, id, name, score, play_time
    );
}

std::vector<PlayerRecordResult> RetiredPlayersRepository::Load(int start, int max_items) const {
    std::vector<PlayerRecordResult> result;
    const pqxx::result query_result = transaction_.exec_params(
        R"(
            SELECT name, score, play_time_ms
            FROM retired_players
            ORDER BY score DESC, play_time_ms, name
            LIMIT $1 OFFSET $2;
        )"_zv, max_items, start
    );
    for (const auto& row : query_result) {
        constexpr double MS_IN_S = 1000.0;
        result.emplace_back(row[0].as<std::string>(), row[1].as<int>(), row[2].as<int>()/MS_IN_S);
    }
    return result;
}

Database::Database(std::shared_ptr<ConnectionPool> pool_ptr)
    : pool_ptr_(std::move(pool_ptr)) {
    const auto conn = pool_ptr_->GetConnection();
    pqxx::work transaction{*conn};
    transaction.exec(
        R"(
            CREATE TABLE IF NOT EXISTS retired_players (
                id UUID CONSTRAINT retired_players_id_constraint PRIMARY KEY,
                name varchar(100) NOT NULL,
                score integer NOT NULL,
                play_time_ms integer NOT NULL
            );
        )"_zv
    );

    transaction.exec(R"(
        CREATE INDEX IF NOT EXISTS score_play_time_ms_name_idx ON retired_players (score DESC, play_time_ms, name);
    )"_zv);
    transaction.commit();
}

ConnectionPool::ConnectionWrapper Database::GetTransaction() const {
    return  pool_ptr_->GetConnection();
}
} // namespace postgres

