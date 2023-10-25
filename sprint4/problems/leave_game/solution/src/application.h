#pragma once

#include <iostream>
#include <memory>
#include <filesystem>
#include <fstream>

#include <boost/asio.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include "model.h"
#include "serialization.h"

namespace app
{
    namespace net = boost::asio;

    class Application
    {
        using Strand = net::strand<net::io_context::executor_type>;

    public:
        Application() = default;
        Application(
            model::Game& game,
            Players& players,
            const std::string &state_file,
            const size_t &save_tick_period) : game_{game},
                                              players_{players},
                                              state_file_path(state_file),
                                              period{save_tick_period}, 
                                              reserve_state_file_path{state_file_path + ".reserve"} {};
        ~Application() = default;

        void SaveGameState(const std::chrono::milliseconds &delta_time)
        {
            if (true)
            {
                SaveGame();
            }
        };

        void SaveGame()
        {
            if (state_file_path.empty())
            {
                return;
            }

            serialization::GameSerialization game_ser{std::make_shared<model::Game>(game_), std::make_shared<Players>(players_)};

            std::fstream output_fstream;
            output_fstream.open(reserve_state_file_path, std::ios_base::out | std::fstream::out);
            {
                boost::archive::text_oarchive oarchive{output_fstream};
                oarchive << game_ser;
            }
            output_fstream.close();

            std::filesystem::rename(reserve_state_file_path, state_file_path);
        }

        void RestoreGame()
        {
            if (state_file_path.empty())
            {
                return;
            }
            serialization::GameSerialization game_ser;

            try
            {
                std::fstream input_fstream;
                input_fstream.open(state_file_path, std::ios_base::in);
                if (!input_fstream.is_open())
                {
                    return;
                }

                boost::archive::text_iarchive iarchive{input_fstream};
                iarchive >> game_ser;
                input_fstream.close();

                game_ser.Restore(std::move(game_), std::move(players_));
            }
            catch (const std::exception &ex)
            {
                throw ex;
            }
        }

    private:
        model::Game& game_;
        Players& players_;

        const std::string state_file_path;
        const std::string reserve_state_file_path;
        std::chrono::milliseconds period{0};
    };

}