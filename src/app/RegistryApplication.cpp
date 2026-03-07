#include "app/RegistryApplication.h"
#include "ui/MainLayout.h"

#include <Wt/WBootstrap5Theme.h>
#include <Wt/WContainerWidget.h>

namespace dr {

RegistryApplication::RegistryApplication(const Wt::WEnvironment& env)
    : Wt::WApplication(env)
{
    setTitle("Deployment Registry");

    enableUpdates(true);

    setupTheme();

    // Build the main layout directly (no auth for v1)
    auto* wrapper = root()->addNew<Wt::WContainerWidget>();
    wrapper->setStyleClass("d-flex flex-column vh-100");

    mainLayout_ = wrapper->addNew<MainLayout>();

    setInternalPath("/deployments", false);
    setupRouting();
}

RegistryApplication* RegistryApplication::instance()
{
    return dynamic_cast<RegistryApplication*>(Wt::WApplication::instance());
}

void RegistryApplication::setupTheme()
{
    auto theme = std::make_shared<Wt::WBootstrap5Theme>();
    setTheme(theme);

    useStyleSheet("https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.3/font/bootstrap-icons.min.css");
    useStyleSheet("resources/css/registry.css");
}

void RegistryApplication::setupRouting()
{
    internalPathChanged().connect([this]() {
        auto path = internalPath();
        // Strip trailing slash so WMenu can match path components
        if (path.size() > 1 && path.back() == '/') {
            path.pop_back();
            setInternalPath(path, false);
        }
        if (mainLayout_)
            mainLayout_->handleInternalPath(path);
    });

    if (mainLayout_)
        mainLayout_->handleInternalPath(internalPath());
}

} // namespace dr
