#include "database.h"

namespace database
{

    using namespace std::literals;
    using pqxx::operator"" _zv;

    ConnectionPool::ConnectionWrapper ConnectionPool::GetConnection()
    {
        std::unique_lock lock{mutex_};
        // Блокируем текущий поток и ждём, пока cond_var_ не получит уведомление и
        // не освободится хотя бы одно соединение
        cond_var_.wait(lock, [this]
                       { return used_connections_ < pool_.size(); });
        // После выхода из цикла ожидания мьютекс остаётся захваченным

        return {std::move(pool_[used_connections_++]), *this};
    }

    void ConnectionPool::ReturnConnection(ConnectionPtr &&conn)
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

    void PlayerRecordRepository::SavePlayerRecordsTable(const std::vector<PlayerRecord> &player_records)
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

    void PlayerRecordRepository::SavePlayerRecord(PlayerRecord &player_record)
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

    std::vector<PlayerRecord> PlayerRecordRepository::GetRecordsTable(size_t offset, size_t limit)
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

} // namespace database