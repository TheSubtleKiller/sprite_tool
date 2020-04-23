// sprite_tool.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define GLEW_STATIC

#include "gl_render_helper.hpp"
#include "file_helper.hpp"
#include "spritesheet.hpp"

#include "imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui_impl/imgui_impl_glfw.h"
#include "imgui_impl/imgui_impl_opengl3.h"

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective

#define cimg_use_png
#include "CImg/CImg.h"

#include "json/json.h"

#include <iostream>
#include <string>



static const struct
{
    float x, y;
    float r, g, b;
} vertices[3] =
{
    { -0.6f, -0.4f, 1.f, 0.f, 0.f },
    {  0.6f, -0.4f, 0.f, 1.f, 0.f },
    {   0.f,  0.6f, 0.f, 0.f, 1.f }
};

static const char* vertex_shader_text =
"#version 110\n"
"uniform mat4 MVP;\n"
"attribute vec3 vCol;\n"
"attribute vec2 vPos;\n"
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
"    color = vCol;\n"
"}\n";

static const char* fragment_shader_text =
"#version 110\n"
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_FragColor = vec4(color, 1.0);\n"
"}\n";



void error_callback(int error, const char* description)
{
    char buffer[1024] = { 0 };
    printf_s(buffer, "Error: %hs\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void SetupViewportFramebuffer(uint32_t& _uFBO, uint32_t & _uTexture, uint32_t & _uRBO, uint32_t const _uWidth, uint32_t const _uHeight)
{
    //---------- generate FBO
    //========================================
    if (_uFBO != 0)
    {
        glDeleteFramebuffers(1, &_uFBO);
    }
    glGenFramebuffers(1, &_uFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, _uFBO);
    //========================================


    //---------- generate texture
    //========================================
    if (_uTexture != 0)
    {
        glDeleteTextures(1, &_uTexture);
    }
    glGenTextures(1, &_uTexture);
    glBindTexture(GL_TEXTURE_2D, _uTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _uWidth, _uHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // attach it to currently bound framebuffer object
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _uTexture, 0);
    //========================================


    //---------- generate RBO
    //========================================
    if (_uRBO != 0)
    {
        glDeleteRenderbuffers(1, &_uRBO);
    }
    glGenRenderbuffers(1, &_uRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, _uRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _uWidth, _uHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _uRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //========================================
}

int main()
{
    {
        using namespace cimg_library;

        CImg <unsigned char> image("assets_plz_ignore/collection_Golden.png");

        /*CImgDisplay main_disp(image, "Click a point");

        while (!main_disp.is_closed()) {
            main_disp.wait();
            if (main_disp.button() && main_disp.mouse_y() >= 0) {
                const int y = main_disp.mouse_y();
            }
        }*/
    }

    std::string _sSpriteSheetXml = FileHelper::GetFileContents("assets_plz_ignore/InGame.xml");

    CSpriteSheet _SpriteSheet;
    _SpriteSheet.ParseXML(_sSpriteSheetXml);


    //---------- Setup GLFW
    //========================================

    //---------- Init GLFW
    if (!glfwInit())
    {
        // Initialization failed
        exit(EXIT_FAILURE);
    }

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(1024, 768, "Sprite Tool", NULL, NULL);
    if (!window)
    {
        // Window or OpenGL context creation failed
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);
    //========================================

    //========================================
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    //========================================


    // NOTE: OpenGL error checks have been omitted for brevity
    //========================================
    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location, vcol_location;

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    mvp_location = glGetUniformLocation(program, "MVP");
    vpos_location = glGetAttribLocation(program, "vPos");
    vcol_location = glGetAttribLocation(program, "vCol");

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
        sizeof(vertices[0]), (void*)0);
    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
        sizeof(vertices[0]), (void*)(sizeof(float) * 2));
    //========================================


    // Setup Dear ImGui context
    //========================================
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    //========================================

    //---------- setup texture and FBO for our scene to render into
    //========================================
    struct SViewportData{
        uint32_t m_uFrameBuffer = 0;
        uint32_t m_uTexture = 0;
        uint32_t m_uRenderBufferObj = 0;

        uint32_t m_uWidth = 0;
        uint32_t m_uHeight = 0;
    };

    SViewportData ViewportData;

    SetupViewportFramebuffer(ViewportData.m_uFrameBuffer, ViewportData.m_uTexture, ViewportData.m_uRenderBufferObj, 800, 600);

    ImVec2 vec2ViewportWindowSize(800, 600);
    //========================================


    // Our state
    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool _bDockSpaceOpen = true;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();


        // Setup main window viewport
        //========================================
        int _iWidth, _iHeight;
        glfwGetFramebufferSize(window, &_iWidth, &_iHeight);

        glViewport(0, 0, _iWidth, _iHeight);
        glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        //========================================


        // Update scene FBO size if required
        //========================================
        if (vec2ViewportWindowSize.x != ViewportData.m_uWidth ||
            vec2ViewportWindowSize.y != ViewportData.m_uHeight)
        {
            ViewportData.m_uWidth = static_cast<uint32_t>(vec2ViewportWindowSize.x);
            ViewportData.m_uHeight = static_cast<uint32_t>(vec2ViewportWindowSize.y);
            SetupViewportFramebuffer(ViewportData.m_uFrameBuffer, ViewportData.m_uTexture, ViewportData.m_uRenderBufferObj, ViewportData.m_uWidth, ViewportData.m_uHeight);
        }
        //========================================


        // Draw our scene to the FBO
        //========================================
        glBindFramebuffer(GL_FRAMEBUFFER, ViewportData.m_uFrameBuffer);
        {
            glViewport(0, 0, ViewportData.m_uWidth, ViewportData.m_uHeight);
            glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            float _fRatio = ViewportData.m_uWidth / (float)ViewportData.m_uHeight;

            glm::mat4 m, p, mvp;
            m = glm::mat4(1.0f);
            m = glm::rotate(m, (float)glfwGetTime(), glm::vec3(0, 0, 1));
            p = glm::ortho(-_fRatio, _fRatio, -1.f, 1.f, 1.f, -1.f);
            mvp = p * m;

            glUseProgram(program);
            glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*)&(mvp.operator[](0).x));
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //========================================


        //---------- Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        //---------- Do the UI
        //========================================
        {
            static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

            // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
            // because it would be confusing to have two docking targets within each others.
            ImGuiWindowFlags window_flags = 
                (ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

            ImGuiViewport* _pMainViewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(_pMainViewport->GetWorkPos());
            ImGui::SetNextWindowSize(_pMainViewport->GetWorkSize());
            ImGui::SetNextWindowViewport(_pMainViewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

            // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
            // This is because we want to keep our DockSpace() active.
            ImGui::Begin("Root DockSpace", &_bDockSpaceOpen, window_flags);

            ImGui::PopStyleVar(3);



            // Pre-defined layout for dockspace
            //========================================
            ImGuiID _RootDockSpaceId = ImGui::GetID("MainDockSpace");
            if (ImGui::DockBuilderGetNode(_RootDockSpaceId) == nullptr)
            {
                ImVec2 dockspace_size(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

                ImGuiID dock_main_id = _RootDockSpaceId; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.
                ImGui::DockBuilderRemoveNode(_RootDockSpaceId); // Clear out existing layout

                ImGui::DockBuilderAddNode(_RootDockSpaceId, ImGuiDockNodeFlags_DockSpace); // Add empty node
                ImGui::DockBuilderSetNodeSize(dock_main_id, dockspace_size);

                ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.15f, NULL, &dock_main_id);

                std::string const& viewport_window_id = "viewport";
                ImGui::DockBuilderDockWindow(viewport_window_id.c_str(), dock_main_id);

                std::string const& timeline_window_id = "timeline";
                ImGui::DockBuilderDockWindow(timeline_window_id.c_str(), dock_id_bottom);

                ImGui::DockBuilderFinish(_RootDockSpaceId);
            }
            //========================================


            // Begin the DockSpace in current window
            //========================================
            ImGuiIO& io = ImGui::GetIO();
            ImGui::DockSpace(_RootDockSpaceId, ImVec2(0.0f, 0.0f), dockspace_flags);
            {
                // DockSpace menu bar
                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu("Docking"))
                    {
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }

                // Main viewport window where we view the scene
                if (ImGui::Begin("viewport", nullptr, 0))
                {
                    ImTextureID id = (ImTextureID)uint64_t(ViewportData.m_uTexture);
                    vec2ViewportWindowSize = ImGui::GetContentRegionAvail();
                    ImGui::Image(id, vec2ViewportWindowSize, ImVec2(0, 1), ImVec2(1, 0));
                }
                ImGui::End();

                // Animation timeline
                if (ImGui::Begin("timeline", nullptr))
                {
                }
                ImGui::End();
            }
            //========================================

            ImGui::End(); // end root window
        }
        //========================================


        // Show the big demo window
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);


        // Build ImGui draw data
        ImGui::Render();

        // Render ImGui
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere).
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
