#pragma once

#include "pqxx/connection"
#include "pqxx/zview.hxx"
#include "pqxx/pqxx"
#include <condition_variable>
#include <memory>
#include <mutex>

namespace database
{

    using namespace std::literals;
    using pqxx::operator"" _zv;

    const std::string DB_URL = "postgres://postgres:root@localhost:5432/game_db";
    // const std::string DB_URL = "GAME_DB_URL";
    const size_t DEFAULT_OFFSET = 0;
    const size_t DEFAULT_LIMIT = 100;

    struct DatabasebConnectioSettings
    {
        size_t number_of_connection{1};
        std::string db_url{};
    };

    class ConnectionPool
    {
        using PoolType = ConnectionPool;
        using ConnectionPtr = std::shared_ptr<pqxx::connection>;

    public:
        class ConnectionWrapper
        {
        public:
            ConnectionWrapper(std::shared_ptr<pqxx::connection> &&conn,
                              PoolType &pool) noexcept
                : conn_{std::move(conn)}, pool_{&pool} {}

            ConnectionWrapper(const ConnectionWrapper &) = delete;
            ConnectionWrapper &operator=(const ConnectionWrapper &) = delete;

            ConnectionWrapper(ConnectionWrapper &&) = default;
            ConnectionWrapper &operator=(ConnectionWrapper &&) = default;

            pqxx::connection &operator*() const & noexcept { return *conn_; }
            pqxx::connection &operator*() const && = delete;

            pqxx::connection *operator->() const & noexcept { return conn_.get(); }

            ~ConnectionWrapper()
            {
                if (conn_)
                {
                    pool_->ReturnConnection(std::move(conn_));
                }
            }

        private:
            std::shared_ptr<pqxx::connection> conn_;
            PoolType *pool_;
        };

        ConnectionPool() = delete;
        ConnectionPool(const ConnectionPool &) = delete;
        ConnectionPool &operator=(const ConnectionPool &) = delete;

        // ConnectionFactory is a functional object returning
        // std::shared_ptr<pqxx::connection>
        template <typename ConnectionFactory>
        ConnectionPool(size_t capacity, ConnectionFactory &&connection_factory)
        {
            pool_.reserve(capacity);
            for (size_t i = 0; i < capacity; ++i)
            {
                pool_.emplace_back(connection_factory());
            }
        }

        ConnectionWrapper GetConnection()
        {
            std::unique_lock lock{mutex_};
            // Блокируем текущий поток и ждём, пока cond_var_ не получит уведомление и
            // не освободится хотя бы одно соединение
            cond_var_.wait(lock, [this]
                           { return used_connections_ < pool_.size(); });
            // После выхода из цикла ожидания мьютекс остаётся захваченным

            return {std::move(pool_[used_connections_++]), *this};
        }

    private:
        void ReturnConnection(ConnectionPtr &&conn)
        {
            // Возвращаем соединение обратно в пул
            {
                std::lock_guard lock{mutex_};
                assert(used_connections_ != 0);
                pool_[--used_connections_] = std::move(conn);
            }
            // Уведомляем один из ожидающих потоков об изменении состояния пула
            cond_var_.notify_one();
        }

        std::mutex mutex_;
        std::condition_variable cond_var_;
        std::vector<ConnectionPtr> pool_;
        size_t used_connections_ = 0;
    };

    class PlayerRecord
    {
    public:
        PlayerRecord(std::string name, size_t score, int64_t play_time)
            : name_(std::move(name)), score_(score), play_time_(play_time){};

        const std::string &GetName() const noexcept { return name_; }

        size_t GetScore() const noexcept { return score_; }

        int64_t GetPlayTime() const noexcept { return play_time_; }

    private:
        std::string name_{};
        size_t score_{0};
        int64_t play_time_{0};
    };

    class PlayerRecordRepository
    {
    public:
        explicit PlayerRecordRepository(std::shared_ptr<ConnectionPool> &connection_pool)
            : connection_pool_(connection_pool){};

        void SavePlayerRecordsTable(const std::vector<PlayerRecord> &player_records)
        {
            auto conn = connection_pool_->GetConnection();
            pqxx::work work_{*conn};
            for (const auto &player_record : player_records)
            {
                work_.exec_params(R"(
            INSERT INTO hall_of_fame (name, score, play_time) VALUES ($1, $2, $3);
            )"_zv,
                                  player_record.GetName(), player_record.GetScore(),
                                  player_record.GetPlayTime());
            }
            work_.commit();
        }

        void SavePlayerRecord(PlayerRecord &player_record)
        {
            auto conn = connection_pool_->GetConnection();
            pqxx::work work_{*conn};

            work_.exec_params(R"(
            INSERT INTO hall_of_fame (name, score, play_time) VALUES ($1, $2, $3);
            )"_zv,
                              player_record.GetName(), player_record.GetScore(),
                              player_record.GetPlayTime());

            work_.commit();
        }

        std::vector<PlayerRecord> GetRecordsTable(size_t offset, size_t limit)
        {
            auto conn = connection_pool_->GetConnection();
            std::vector<PlayerRecord> records_table;
            pqxx::read_transaction read_transaction_{*conn};
            auto query_text = "SELECT name, score, play_time FROM hall_of_fame ORDER "
                              "BY score DESC, play_time ASC, name ASC LIMIT " +
                              std::to_string(limit) + " OFFSET " +
                              std::to_string(offset) + ";";
            for (auto [name, score, play_time] :
                 read_transaction_.query<std::string, size_t, int64_t>(query_text))
            {
                records_table.emplace_back(name, score, play_time);
            }
            return records_table;
        }

    private:
        std::shared_ptr<ConnectionPool> &connection_pool_;
    };

    class Database
    {
    public:
        Database(const DatabasebConnectioSettings &db_settings) : connection_pool_(std::make_shared<ConnectionPool>(db_settings.number_of_connection,
                                                                                                                    [db_url = std::move(db_settings.db_url)]()
                                                                                                                    {
                                                                                                                        return std::make_shared<pqxx::connection>(db_url);
                                                                                                                    }))
        {

            auto conn = connection_pool_->GetConnection();
            pqxx::work work_{*conn};
            work_.exec(R"(
CREATE TABLE IF NOT EXISTS hall_of_fame (
    id SERIAL PRIMARY KEY,
    name varchar(40) NOT NULL,
    score integer CONSTRAINT score_positive CHECK (score >= 0),
    play_time integer NOT NULL CONSTRAINT play_time_positive CHECK (play_time >= 0)     
);
CREATE INDEX IF NOT EXISTS hall_of_fame_score ON hall_of_fame (score); 
)"_zv);
            // коммитим изменения
            work_.commit();
        }

        PlayerRecordRepository &GetPlayerRecords() & { return player_records_; }

    private:
        std::shared_ptr<ConnectionPool> connection_pool_ = nullptr;
        PlayerRecordRepository player_records_{connection_pool_};
    };

} // namespace database