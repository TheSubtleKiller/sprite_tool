// sprite_tool.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include "sprite_tool.hpp"

#include "utility/file_helper.hpp"
#include "utility/stl_helper.hpp"

#include "spritesheet.hpp"
#include "compound_sprite.hpp"

#include "ui/ui.hpp"

// Imgui
#include "imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui_impl/imgui_impl_glfw.h"
#include "imgui_impl/imgui_impl_opengl3.h"

// gl stuff
#define GLEW_STATIC
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective

#include "gl_render_helper.hpp"

// stl
#include <iostream>
#include <string>


static const char* s_ShaderVert = 
R"(
    #version 330 core

    uniform mat4 MVP;

    attribute vec4 vCol;
    attribute vec3 vPos;
    attribute vec2 uv;

    varying vec4 color;
    varying vec2 uv_out;

    void main()
    {
        gl_Position = MVP * vec4(vPos, 1.0);
        color = vCol;
        uv_out = uv;
    };
)";

static const char* s_ShaderFrag =
R"(
    #version 330 core

    uniform sampler2D image;

    varying vec4 color;
    varying vec2 uv_out;

    void main()
    {
        gl_FragColor = color * texture(image, uv_out);
    };
)";



void error_callback(int error, const char* description)
{
    char buffer[1024] = { 0 };
    printf_s(buffer, "Error: %hs\n", description);
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

int CSpriteTool::Run()
{
    std::map<std::string, CSpriteSheet> mapSpriteSheets;


    //========================================
    //std::string _sJsonName = "monkey_city_icon.json";
    //std::string _sJsonName = "mix_n_match_icon.json";
    //std::string _sJsonName = "LevelDefinitions/castle/castle.props";
    //std::string _sJsonName = "LevelDefinitions/castle/castle.props";
    //std::string _sJsonName = "BloonSprites/blastapopoulos_01.json";
    //std::string _sJsonName = "BloonSprites/bfb_undamaged.json";
    //std::string _sJsonName = "WeaponSprites/Explosion.json";
    std::string _sJsonName = "BloonSprites/boss_death_explosion_01.json"; 
    std::string _sJsonPathToTest = stl_helper::Format("assets_plz_ignore/JSON/%s", _sJsonName.c_str());
    std::string _sJson = FileHelper::GetFileContentsString(_sJsonPathToTest);

    CCompoundSprite _CompoundSprite;
    _CompoundSprite.ParseJSON(_sJson);
    //========================================


    //========================================
    std::vector<std::string> _vectorTexturesToLoad;
    auto _mapTextureSprites = _CompoundSprite.GetTextureSprites();
    for (auto _item : _mapTextureSprites)
    {
        std::string _sTexture = _item.first;

        _vectorTexturesToLoad.push_back(_sTexture);
    }
    //========================================

    _vectorTexturesToLoad.push_back("InGame");

    //========================================
    for (auto& _sTexture : _vectorTexturesToLoad)
    {
        std::string _sXmlPath = stl_helper::Format("assets_plz_ignore/textures/tablet/%s.xml", _sTexture.c_str());
        std::string _sSpriteSheetXml = FileHelper::GetFileContentsString(_sXmlPath);

        assert(_sSpriteSheetXml.empty() == false);

        CSpriteSheet _SpriteSheet;
        _SpriteSheet.ParseXML(_sSpriteSheetXml);
        mapSpriteSheets[_sTexture] = _SpriteSheet;
    }
    //========================================


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

    glfwSetWindowUserPointer(window, this);
    
    //---------- set key press callback func
    auto key_callback = [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    };
    glfwSetKeyCallback(window, key_callback);

    //---------- set mouse scroll press callback func
    auto scroll_callback = [](GLFWwindow* window, double xoffset, double yoffset)
    {
        CSpriteTool* _pSpriteTool = static_cast<CSpriteTool*>(glfwGetWindowUserPointer(window));
        _pSpriteTool->SetMouseScroll(xoffset, yoffset);
    };
    glfwSetScrollCallback(window, scroll_callback);

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
    GLuint vertex_shader, fragment_shader, program;
    GLint mvp_location;

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &s_ShaderVert, nullptr);
    glCompileShader(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &s_ShaderFrag, nullptr);
    glCompileShader(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint _iProgramLinked;
    glGetProgramiv(program, GL_LINK_STATUS, &_iProgramLinked);
    if (_iProgramLinked != GL_TRUE)
    {
        GLsizei _iIgnored;
        size_t const c_uLogSize = 4096;
        char _LogVertex[c_uLogSize];
        char _LogFragment[c_uLogSize];
        char _LogProgram[c_uLogSize];

        glGetShaderInfoLog(vertex_shader, c_uLogSize, &_iIgnored, _LogVertex);
        glGetShaderInfoLog(fragment_shader, c_uLogSize, &_iIgnored, _LogFragment);
        glGetProgramInfoLog(program, c_uLogSize, &_iIgnored, _LogProgram);

        std::string _sMessage = stl_helper::Format("%s\n%s\n%s", _LogVertex, _LogFragment, _LogProgram);

        assert(false && _sMessage.c_str());
    }


    mvp_location = glGetUniformLocation(program, "MVP");
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


    //========================================
    std::map<std::string, uint32_t> mapTextureNameId;

    for (auto &_sTexture : _vectorTexturesToLoad)
    {
        mapTextureNameId[_sTexture] = 0;

        std::string _sTexturePath = stl_helper::Format("assets_plz_ignore/textures/tablet/%s", _sTexture.c_str());

        int width = 0, height = 0;
        auto _ImageData = FileHelper::LoadImage(_sTexturePath.c_str(), width, height);

        if (_ImageData.m_pData != nullptr && _ImageData.m_pData->size() > 0)
        {
            uint32_t _eChannels = (_ImageData.m_uChannels == 4)? GL_RGBA : GL_RGB;

            uint32_t &_uTextureId = mapTextureNameId[_sTexture];
            glGenTextures(1, &_uTextureId);
            glBindTexture(GL_TEXTURE_2D, _uTextureId);
            glTexImage2D(GL_TEXTURE_2D, 0, _eChannels, width, height, 0, _eChannels, GL_UNSIGNED_BYTE, _ImageData.m_pData->data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        else
        {
            // fail
            assert(false);
        }
    }
    //========================================


    // Our state
    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool _bDockSpaceOpen = true;

    double _dPrevTime = 0.0;

    while (!glfwWindowShouldClose(window))
    {
        double _dDeltaTime = std::fmin(0.05, glfwGetTime() - _dPrevTime);
        _dPrevTime = glfwGetTime();

        SetMouseScroll(0.0, 0.0);

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

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);

            float _fRatio = ViewportData.m_uWidth / (float)ViewportData.m_uHeight;

            m_fViewPortScale += m_dMouseScrollY * 0.01f;
            m_fViewPortScale = std::fmaxf(m_fViewPortScale, 0.01f);

            float _fScale = m_fViewPortScale * 0.01f;

            glm::mat4 m, p, mvp;
            m = glm::mat4(1.0f);
            m = glm::scale(m, glm::vec3(_fScale, _fScale, _fScale));
            m = glm::scale(m, glm::vec3(1, -1, 1));
            p = glm::ortho(-_fRatio, _fRatio, -1.f, 1.f, 1.f, -1.f);
            mvp = p * m;

            glUseProgram(program);
            glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*)&(mvp.operator[](0).x));

            static float s_fTime = 0.0f;
            if (m_bAnimate)
            {
                s_fTime = fmodf(s_fTime + float(_dDeltaTime) * m_fAnimationSpeedMult, _CompoundSprite.GetStageLength());
            }

            // Draw the compound sprite
            auto const &_mapActors = _CompoundSprite.GetActors();
            for (auto const & _itActor : _mapActors)
            {
                CCompoundSprite::SActor const& _Actor = _itActor.second;
                switch (static_cast<CCompoundSprite::SActor::Type>(_Actor.m_uType))
                {
                    case CCompoundSprite::SActor::Type::Sprite:
                    {
                        std::string _sTexture =_CompoundSprite.GetTextureForSprite(_Actor.m_sSprite);
                        mapTextureNameId[_sTexture];

                        CSpriteSheet const &_SpriteSheet = mapSpriteSheets[_sTexture];
                        auto const & _mapSprites = _SpriteSheet.GetSpriteData();
                        auto _itSprite = _mapSprites.find(_Actor.m_sSprite);
                        if (_itSprite != _mapSprites.end())
                        {
                            CSpriteSheet::SSpriteCell const& _Cell = _itSprite->second;

                            CCompoundSprite::SActorState _ActorState = _CompoundSprite.GetStateForActorAtTime(_Actor.m_uID, s_fTime);

                            gl_render_helper::DrawSprite(_Cell,
                                                         _ActorState,
                                                         program,
                                                         mapTextureNameId[_sTexture]);
                        }
                        break;
                    }
                    case CCompoundSprite::SActor::Type::Compound:
                    {
                        break;
                    }
                }
            }
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
            ImGuiID dock_id_bottom = 0;
            if (ImGui::DockBuilderGetNode(_RootDockSpaceId) == nullptr)
            {
                ImVec2 dockspace_size(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

                ImGuiID dock_main_id = _RootDockSpaceId; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.
                ImGui::DockBuilderRemoveNode(_RootDockSpaceId); // Clear out existing layout

                ImGui::DockBuilderAddNode(_RootDockSpaceId, ImGuiDockNodeFlags_DockSpace); // Add empty node
                ImGui::DockBuilderSetNodeSize(dock_main_id, dockspace_size);

                dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.15f, NULL, &dock_main_id);

                std::string const& viewport_window_id = "viewport";
                ImGui::DockBuilderDockWindow(viewport_window_id.c_str(), dock_main_id);

                std::string const& timeline_window_id = "timeline";
                ImGui::DockBuilderDockWindow(timeline_window_id.c_str(), dock_id_bottom);

                std::string const& sprites_window_id = "sprites";
                ImGui::DockBuilderDockWindow(sprites_window_id.c_str(), dock_id_bottom);

                for (auto& _SpriteSheetItem : mapSpriteSheets)
                {
                    std::string _window_id = stl_helper::Format("sprites - %s", _SpriteSheetItem.first.c_str());
                    ImGui::DockBuilderDockWindow(_window_id.c_str(), dock_id_bottom);
                }

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
                    ImGui::Checkbox("Animate", &m_bAnimate);
                    ImGui::SameLine();
                    ImGui::SliderFloat("Animation Speed", &m_fAnimationSpeedMult, 0.0f, 10.0f);

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

                // Sprites
                for (auto &_SpriteSheetItem : mapSpriteSheets)
                {
                    auto _itTexNameId = mapTextureNameId.find(_SpriteSheetItem.first);
                    if (_itTexNameId != mapTextureNameId.end())
                    {
                        if (ImGui::Begin(stl_helper::Format("sprites - %s", _SpriteSheetItem.first.c_str()).c_str(), nullptr))
                        {
                            ui::SpriteSheetWindow(_SpriteSheetItem.second, _itTexNameId->second);
                        }
                        ImGui::End();
                    }
                }
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
