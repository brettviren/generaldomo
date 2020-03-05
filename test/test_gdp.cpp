/*! Test generaldomo
 */

#include "generaldomo/broker.hpp"
#include "generaldomo/client.hpp"
#include "generaldomo/worker.hpp"

#include <zmq_actor.hpp>

using namespace generaldomo;

void countdown_echo(zmq::socket_t& pipe, std::string address, int socktype)
{
    console_log log;

    zmq::context_t ctx;
    zmq::socket_t sock(ctx, socktype);
    Client client(sock, address, log);

    pipe.send(zmq::message_t{}, zmq::send_flags::none);

    int countdown = 4;

    while (countdown) {
        --countdown;
        std::stringstream ss;
        if (countdown) {
            ss << countdown << "...";
        }
        else {
            ss << "blast off!";
        }
        zmq::multipart_t mmsg(ss.str());
        client.send("echo", mmsg);
        mmsg.clear();
        client.recv(mmsg);
        if (mmsg.empty()) {
            log.error("countdown echo timeout");
            break;
        }
        {
            std::stringstream ss;
            ss << "countdown echo [" << mmsg.size() << "]:\n";
            while (mmsg.size()) {
                ss << "\t" << mmsg.popstr() << "\n";
            }
            log.debug(ss.str());
        }
    }
    pipe.send(zmq::message_t{}, zmq::send_flags::none);
    zmq::message_t die;
    auto res = pipe.recv(die);
    log.debug("countdown echo exiting");
}


void doit(int serverish, int clientish, int nworkers, int nclients)
{
    console_log log;
    log.level=console_log::log_level::debug;
    std::stringstream ss;
    ss<<"main doit("<<serverish<<","<<clientish<<","<<nworkers<<","<<nclients<<")";
    log.info(ss.str());

    zmq::context_t ctx;
    std::string broker_address = "tcp://127.0.0.1:5555";

    std::vector<zmq::actor_t*> clients;
    std::vector<std::pair<std::string, zmq::actor_t*> > actors;


    log.info("main make broker actor");
    actors.push_back({"broker",new zmq::actor_t(ctx, broker_actor, broker_address, serverish)});

    while (nworkers--) {
        log.info("main make worker actor");
        actors.push_back({"worker",new zmq::actor_t(ctx,    echo_worker, broker_address, clientish)});
    }
    while (nclients--) {
        log.info("main make client actor");
        auto client = new zmq::actor_t(ctx, countdown_echo, broker_address, clientish);
        actors.push_back({"client",client});
        clients.push_back(client);
    }


    for (auto client : clients) {
        log.debug("main wait for client");
        zmq::message_t done;
        auto res = client->pipe().recv(done);
    }

    // terminate backwards
    for (auto it = actors.rbegin(); it != actors.rend(); ++it) {
        log.info("main terminate actor " + it->first);
        zmq::actor_t* actor = it->second;
        actor->pipe().send(zmq::message_t{}, zmq::send_flags::none);
        delete actor;
    }
    log.info("main doit exiting");
}

int main()
{
    generaldomo::catch_signals();

    doit(ZMQ_SERVER, ZMQ_CLIENT, 1, 1);
    //doit(ZMQ_ROUTER, ZMQ_DEALER, 1, 1);
}
