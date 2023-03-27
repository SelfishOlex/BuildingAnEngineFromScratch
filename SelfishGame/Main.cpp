
#include <GLFW/glfw3.h>
#include <Renderer/Renderer.h>

class Application {
public:
    void Run()
    {
        InitWindow();
        InitVulkan();
        MainLoop();
        Cleanup();
    }

private:
    void InitWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        m_window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    }

    void InitVulkan()
    {
        m_renderer.InitVulkan(m_window);
    }

    void MainLoop()
    {
        while (!glfwWindowShouldClose(m_window)) 
        {
            glfwPollEvents();
            m_renderer.DrawFrame();
        }

        m_renderer.OnExitMainLoop();
    }

    void Cleanup()
    {
        m_renderer.Cleanup();

        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    Renderer m_renderer;

    GLFWwindow* m_window;
};

int main()
{
    Application app;
    app.Run();
    return 0;
}
