#pragma once

#include <Wt/WApplication.h>
#include <string>

namespace dr {

class MainLayout;

/// Root Wt application for the Deployment Registry.
/// One instance per browser session.
class RegistryApplication : public Wt::WApplication {
public:
    explicit RegistryApplication(const Wt::WEnvironment& env);

    static RegistryApplication* instance();

private:
    void setupTheme();
    void setupRouting();

    MainLayout* mainLayout_ = nullptr;
};

} // namespace dr
