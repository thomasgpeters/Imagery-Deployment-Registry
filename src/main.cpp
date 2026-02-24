#include "app/RegistryApplication.h"

#include <Wt/WServer.h>
#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[])
{
    try {
        Wt::WServer server(argc, argv);

        // Configure Wt logger based on LOG_LEVEL env var
        {
            std::string level = "info";
            if (const char* ll = std::getenv("LOG_LEVEL"))
                level = ll;

            if (level == "error")
                server.logger().configure("* -info -debug -warning -accesslog");
            else if (level == "warn" || level == "warning")
                server.logger().configure("* -info -debug -accesslog");
            else if (level == "info")
                server.logger().configure("* -debug -accesslog");
        }

        server.addEntryPoint(
            Wt::EntryPointType::Application,
            [](const Wt::WEnvironment& env) {
                return std::make_unique<dr::RegistryApplication>(env);
            }
        );

        std::cout << "Deployment Registry v0.1.0" << std::endl;
        std::cout << "Starting server..." << std::endl;

        server.run();
    } catch (const Wt::WServer::Exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
