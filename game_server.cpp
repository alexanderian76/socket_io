#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>
#include <boost/asio.hpp>
#include <unordered_map>
#include <vector>
#include <memory>
#include <random>
#include <string>
#include <mutex>
#include <iostream>
#include <set>
#include <boost/assert.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace json = boost::json;
using tcp = boost::asio::ip::tcp;

// Game data structures
struct Player
{
    std::string id;
    double x, y;
    std::string color;
    int radius = 5;
    double speed = 10.0;
    bool is_alive = true;
    int direction = 0;
    int score = 0;

    json::value to_json() const
    {
        return {
            {"id", id},
            {"x", x},
            {"y", y},
            {"color", color},
            {"radius", radius},
            {"speed", speed},
            {"isAlive", is_alive},
            {"direction", direction},
            {"score", score}};
    }
};

struct GridCell
{
    int grid_x, grid_y;
    std::string color;

    json::value to_json() const
    {
        return {
            {"gridX", grid_x},
            {"gridY", grid_y},
            {"color", color}};
    }
};

class GameState
{
    std::vector<Player> players_;
    std::vector<GridCell> grid_;
    std::mutex mutex_;
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_int_distribution<> hue_dist_;

public:
    GameState() : gen_(rd_()), hue_dist_(0, 359) {}

    std::string generate_color()
    {
        return "hsl(" + std::to_string(hue_dist_(gen_)) + ",100%,50%)";
    }

    Player add_player(const std::string &id)
    {
        std::uniform_real_distribution<> pos_dist(0.0, 800.0);
        std::lock_guard<std::mutex> lock(mutex_);

        Player p;
        p.id = id;
        p.x = pos_dist(gen_);
        p.y = pos_dist(gen_);
        p.color = generate_color();
        p.direction = hue_dist_(gen_) % 4;

        players_.push_back(p);
        return p;
    }

    void remove_player(const std::string id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            std::vector<Player> newPlayers;
            Player deletedPlayer;
            for(int i = 0; i < players_.size(); i++)
            {
                if(players_[i].id != id)
                    newPlayers.push_back(players_[i]);
                else
                    deletedPlayer = players_[i];
            }
            players_ = newPlayers;

            std::vector<GridCell> newGrid;
            for (int i = 0; i < grid_.size(); i++)
            {
                if (grid_[i].color != deletedPlayer.color)
                    newGrid.push_back(grid_[i]);
            }
            grid_ = newGrid;
            //    if(players_.contains(id))
            //   players_.erase(id);
        }
        catch (std::exception e)
        {
        }
    }

    bool move_player(const std::string id, double x, double y, int direction)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        int index = 0;
        for (int i = 0; i < players_.size(); i++)
        {
            if(players_[i].id == id)
                index = i;
        }
        Player& it = players_[index];
        //auto it = foundPlayer;
        if (!it.is_alive)
        {
            return false;
        }

        Player &player = it;
        player.x = x;
        player.y = y;
        player.direction = direction;

        int grid_x = static_cast<int>(x / 10) * 10;
        int grid_y = static_cast<int>(y / 10) * 10;

        // Check for collisions
        auto collision = std::find_if(grid_.begin(), grid_.end(),
                                      [grid_x, grid_y, &player](const GridCell &cell)
                                      {
                                          return cell.grid_x == grid_x && cell.grid_y == grid_y && cell.color != player.color;
                                      });

        if (collision != grid_.end())
        {
            player.is_alive = false;

            // Find owner of the collided cell and transfer score
            auto owner = std::find_if(players_.begin(), players_.end(),
                                      [&collision](const auto &p)
                                      { return p.color == collision->color; });

            if (owner != players_.end())
            {
                owner->score += player.score;
            }
            std::vector<GridCell> newGrid;
            for (int i = 0; i < grid_.size(); i++)
            {
                if (grid_[i].color != player.color)
                    newGrid.push_back(grid_[i]);
            }
            grid_ = newGrid;
            return true;
        }

        // Add to grid
        grid_.push_back({grid_x, grid_y, player.color});
        player.score++;

        return false;
    }

    json::value get_game_state()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        json::array players_json;
        for (const auto &player : players_)
        {
            players_json.push_back(player.to_json());
        }

        json::array grid_json;
        for (const auto &cell : grid_)
        {
            grid_json.push_back(cell.to_json());
        }

        return {
            {"players", players_json},
            {"grid", grid_json}};
    }

    json::value get_init_state(const std::string player_id)
    {
        auto state = get_game_state();
        state.as_object()["yourId"] = player_id;
        return state;
    }
};

// WebSocket session class
class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>
{
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    GameState &game_state_;
    std::string id_;
    static std::set<std::shared_ptr<WebSocketSession>> sessions_;
    static std::mutex sessions_mutex_;

public:
    WebSocketSession(tcp::socket &&socket, GameState &game_state)
        : ws_(std::move(socket)), game_state_(game_state)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 1000000);
        id_ = std::to_string(dis(gen));
    }

    ~WebSocketSession()
    {
     //   remove_session(shared_from_this());
    }

    void run()
    {
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            sessions_.insert(shared_from_this());
        }

        ws_.set_option(websocket::stream_base::timeout::suggested(
            beast::role_type::server));

        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::response_type &res)
            {
                //  res.set(http::field::server, "Game Server");
                res.set(http::field::access_control_allow_origin, "*");
            }));

        ws_.async_accept(
            beast::bind_front_handler(
                &WebSocketSession::on_accept,
                shared_from_this()));
    }

    void on_accept(beast::error_code ec)
    {
        if (ec)
        {
            std::cerr << "Accept error: " << ec.message() << std::endl;
            return;
        }

        std::cout << "New player connected: " << id_ << std::endl;

        // Add player to game state
        Player player = game_state_.add_player(id_);

        // Send init message
        send(json::serialize(json::value{
            {"type", "init"},
            {"players", game_state_.get_game_state().at("players")},
            {"grid", game_state_.get_game_state().at("grid")},
            {"yourId", id_}}));

        // Notify other players
        broadcast(json::serialize(json::value{
                      {"type", "newPlayer"},
                      {"player", player.to_json()}}),
                  this->id_);

        do_read();
    }

    void do_read()
    {
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &WebSocketSession::on_read,
                shared_from_this()));
    }

    void on_read(beast::error_code ec, size_t bytes_transferred)
    {
        if (ec == websocket::error::closed)
        {
            return handle_disconnect();
        }

        if (ec)
        {
            std::cerr << "Read error: " << ec.message() << std::endl;
            return handle_disconnect();
        }

        try
        {
            auto msg = beast::buffers_to_string(buffer_.data());
            buffer_.consume(buffer_.size());

            json::value json_msg = json::parse(msg);
            std::string type = json_msg.at("type").as_string().c_str();

            if (type == "move")
            {
                double x = json_msg.at("x").as_double();
                double y = json_msg.at("y").as_double();
                int direction = json_msg.at("direction").as_int64();

                bool dead = game_state_.move_player(id_, x, y, direction);

                if (dead)
                {
                    send(json::serialize(json::value{
                        {"type", "dead"}}));

                    broadcast(json::serialize(json::value{
                                  {"type", "playerDead"},
                                  {"id", id_}}),
                              this->id_);
                }

                // Broadcast game state update
                broadcast(json::serialize(json::value{
                              {"type", "update"},
                              {"players", game_state_.get_game_state().at("players")},
                              {"grid", game_state_.get_game_state().at("grid")}}),
                          this->id_);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error processing message: " << e.what() << std::endl;
        }

        do_read();
    }

    void send(const std::string &message)
    {
        ws_.write(
            net::buffer(message));
    }

    void on_write(beast::error_code ec, std::size_t)
    {
        if (ec)
        {
            std::cerr << "Write error: " << ec.message() << std::endl;
            return;
        }
    }

    static void broadcast(const std::string &message, std::string id)
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (auto &session : sessions_)
        {
            if(session.get()->ws_.is_open())
            // if(session.get()->id_ != id)
            session->send(message);
        }
    }

    static void remove_session(std::shared_ptr<WebSocketSession> session)
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_.erase(session);
    }

    void handle_disconnect()
    {
        std::cout << "Player disconnected: " << id_ << std::endl;
        game_state_.remove_player(id_);

        // Notify other players
        broadcast(json::serialize(json::value{
                      {"type", "playerDisconnected"},
                      {"id", id_}}),
                  this->id_);

        // Close the WebSocket connection
        //if(ws_.is_open())
        // ws_.close(websocket::close_code::normal);
    }
};

// Static members initialization
std::set<std::shared_ptr<WebSocketSession>> WebSocketSession::sessions_;
std::mutex WebSocketSession::sessions_mutex_;

// Server class
class Server
{
    net::io_context ioc_;
    tcp::acceptor acceptor_;
    GameState game_state_;

public:
    Server(short port)
        : ioc_(1),
          acceptor_(ioc_, {tcp::v4(), port})
    {
        do_accept();
    }

    void run()
    {
        ioc_.run();
    }

private:
    void do_accept()
    {
        acceptor_.async_accept(
            [this](beast::error_code ec, tcp::socket socket)
            {
                if (ec)
                {
                    std::cerr << "Accept error: " << ec.message() << std::endl;
                }
                else
                {
                    std::make_shared<WebSocketSession>(
                        std::move(socket), game_state_)
                        ->run();
                }
                do_accept();
            });
    }
};

int main()
{
    try
    {
        short port = 3000;
        Server server(port);
        std::cout << "Server running on port " << port << std::endl;
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}