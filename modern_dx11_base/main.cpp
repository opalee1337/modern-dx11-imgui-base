#include "overlay/overlay.h"
#include "common/common.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance,
                   _In_opt_ HINSTANCE /*hPrevInstance*/,
                   _In_ LPSTR /*lpCmdLine*/, _In_ int /*nCmdShow*/) {

    pane::timer::Init(); // initialize hpt (os overhead shit)

    if (!gOverlay.Initialize(hInstance))
      return -1;

    // render logic
    gOverlay.OnRender = []() {
      if (gOverlay.IsOpen) {
        ImGui::ShowDemoWindow();
      }
    };

    gOverlay.Run();
    gOverlay.Shutdown();
    return 0;
}