#pragma once

#include "App.g.h"

namespace OnyxOS
{
    public ref class App sealed
    {
    public:
        App();

    protected:
        virtual void OnLaunched(
            Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args) override;
    };
}
