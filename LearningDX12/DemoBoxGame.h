#pragma once
#include "BaseGameInterface.h"

namespace Olex
{
    class DemoBoxGame final
        : public BaseGameInterface
    {
    public:
        DemoBoxGame() = default;

        void LoadResources() override;
        void UnloadResources() override;
        void Update( UpdateEventArgs args ) override;
        void Render( RenderEventArgs args ) override;
    };
}
