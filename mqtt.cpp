
#include "broker1/SocketContextFactory.h" // IWYU pragma: keep
#include "core/SNodeC.h"
#include "log/Logger.h"
#include "net/in/stream/legacy/SocketServer.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    core::SNodeC::init(argc, argv);

    using MQTTLegacyInServer = net::in::stream::legacy::SocketServer<mqtt::broker1::SocketContextFactory>;
    using LegacyInSocketConnection = MQTTLegacyInServer::SocketConnection;

    MQTTLegacyInServer mqttLegacyInServer(
        "legacyin",
        []([[maybe_unused]] LegacyInSocketConnection* socketConnection) -> void { // OnConnect
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        },
        []([[maybe_unused]] LegacyInSocketConnection* socketConnection) -> void { // OnConnected
            VLOG(0) << "OnConnected";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        },
        []([[maybe_unused]] LegacyInSocketConnection* socketConnection) -> void { // OnDisconnected
            VLOG(0) << "OnDisconnected";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        });

    mqttLegacyInServer.listen([mqttLegacyInServer](const MQTTLegacyInServer::SocketAddress& socketAddress, int errnum) mutable -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << mqttLegacyInServer.getConfig().getName() << " listening on " << socketAddress.toString();
        }
    });

    return core::SNodeC::start();
}
