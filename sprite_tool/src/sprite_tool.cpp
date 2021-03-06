// sprite_tool.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include "sprite_tool.hpp"

#include "version.hpp"

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
#include "ui/imgui_style.hpp"

// gl stuff
#define GLEW_STATIC
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp" // glm::translate, glm::rotate, glm::scale, glm::perspective

#include "gl_render_helper.hpp"

// stl
#include <iostream>
#include <string>
#include <functional>

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

void GetTexturesFromCompound(tSharedCompoundSprite &_pCompound, std::vector<std::string> & _vectorTextures)
{
    auto _mapTextureSprites = _pCompound->GetTextureSprites();
    for (auto _item : _mapTextureSprites)
    {
        _vectorTextures.push_back(_item.first);
    }
}

bool CSpriteTool::OpenJSONFile(std::string const& _sPath)
{
    std::string _sAbsPath = FileHelper::GetAbsolutePath(_sPath);

    // Parse the compounds
    CCompoundSprite::ParseJSONFileRecursive(_sAbsPath, m_mapCompounds);

    // Failed to load any compounds?
    if (m_mapCompounds.size() == 0)
    {
        fprintf(stdout, "Failed to load an compounds?.\n");
        return false;
    }

    std::string _sTextureParentFolder = FileHelper::PickFolderDialog(_sPath);
    if (_sTextureParentFolder.empty())
    {
        fprintf(stdout, "No texture folder supplied.\n");
        return false;
    }

    // Get required textures from compounds
    //========================================
    std::vector<std::string> _vectorTexturesToLoad;
    for (auto& _Item : m_mapCompounds)
    {
        GetTexturesFromCompound(_Item.second, _vectorTexturesToLoad);
    }
    //========================================

    // Load required spritesheets
    //========================================
    LoadSpriteSheets(_sTextureParentFolder, _vectorTexturesToLoad, m_mapSpriteSheets);
    //========================================

    // Load textures into opengl
    //========================================
    LoadTextures(_sTextureParentFolder, _vectorTexturesToLoad, m_mapTextureNameId);
    //========================================

    return true;
}

std::vector<SActorInstance> CSpriteTool::BuildActorInstances(std::shared_ptr<CCompoundSprite>& _pCompound)
{
    std::vector<SActorInstance> _vectorInstances;

    auto const& _vectorActors = _pCompound->GetActors();
    for (auto const& _Actor : _vectorActors)
    {
        _vectorInstances.emplace_back();
        SActorInstance &_ActorInstance = _vectorInstances.back();
        _ActorInstance.m_pCompound = _pCompound;
        _ActorInstance.m_uActorId = _Actor.m_uID;

        switch (static_cast<CCompoundSprite::SActor::Type>(_Actor.m_uType))
        {
            // Do nothing, already have what we need above
            case CCompoundSprite::SActor::Type::Sprite:
            {
                break;
            }

            //Recurse!
            case CCompoundSprite::SActor::Type::Compound:
            {
                auto _itSubCompound = m_mapCompounds.find(_Actor.m_sSubCompoundPath);

                if (_itSubCompound != m_mapCompounds.end())
                {
                    _ActorInstance.m_vectorActors = BuildActorInstances(_itSubCompound->second);
                }
                break;
            }
        }
    }

    return _vectorInstances;
}

void CSpriteTool::LoadSpriteSheets(std::string const& _sParentFolder, std::vector<std::string> const& _vectorTextures, std::map<std::string, CSpriteSheet>& _mapSpriteSheets)
{
    for (auto& _sTexture : _vectorTextures)
    {
        std::string _sXmlPath = stl_helper::Format("%s/%s.xml", _sParentFolder.c_str(), _sTexture.c_str());
        std::string _sSpriteSheetXml = FileHelper::GetFileContentsString(_sXmlPath);

        assert(_sSpriteSheetXml.empty() == false);

        CSpriteSheet _SpriteSheet;
        _SpriteSheet.ParseXML(_sSpriteSheetXml);
        _SpriteSheet.SetTextureRes(CSpriteSheet::TextureRes::High);
        _mapSpriteSheets[_sTexture] = _SpriteSheet;
    }
}

void CSpriteTool::LoadTextures(std::string const& _sParentFolder, std::vector<std::string> const& _vectorTextures, std::map<std::string, uint32_t>& _mapTextureNameId)
{
    for (auto& _sTexture : _vectorTextures)
    {
        uint32_t& _uTextureId = _mapTextureNameId[_sTexture];

        // Already loaded, skip
        if (_uTextureId != 0)
        {
            continue;
        }

        fprintf(stdout, "Attempting to load texture '%s\\%s'.\n", _sParentFolder.c_str(), _sTexture.c_str());

        std::string _sTexturePath = stl_helper::Format("%s/%s", _sParentFolder.c_str(), _sTexture.c_str());

        int width = 0, height = 0;
        auto _ImageData = FileHelper::LoadImageFromFile(_sTexturePath.c_str(), width, height);

        if (_ImageData.m_pData != nullptr && _ImageData.m_pData->size() > 0)
        {
            uint32_t _eChannels = (_ImageData.m_uChannels == 4) ? GL_RGBA : GL_RGB;

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
}

int CSpriteTool::Run()
{
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

    std::string _sWindowTitle = stl_helper::Format("Sprite Tool %s", GetVersionString().c_str());
    glfwSetWindowTitle(window, _sWindowTitle.c_str());

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

    // style

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

    {
        //========================================
        {
            //std::string _sJsonName = "monkey_city_icon.json";
            //std::string _sJsonName = "mix_n_match_icon.json";
            //std::string _sJsonName = "LevelDefinitions/castle/castle.props";
            //std::string _sJsonName = "LevelDefinitions/castle/castle.props";
            std::string _sJsonName = "BloonSprites/blastapopoulos_01.json";
            //std::string _sJsonName = "BloonSprites/blastapopoulos_prop.json";
            //std::string _sJsonName = "BloonSprites/bfb_undamaged.json";
            //std::string _sJsonName = "WeaponSprites/Explosion.json";
            //std::string _sJsonName = "BloonSprites/boss_death_explosion_01.json";

            //m_sOpenFile = stl_helper::Format("assets_plz_ignore/JSON/%s", _sJsonName.c_str());
        }
        //========================================
    }


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


        // Handle selecting new file to open
        //========================================
        if (m_sOpenFile.empty() == false)
        {
            // Delete everything so we have a clean slate for next compound
            {
                m_vectorActorInstances.clear();
                m_mapCompounds.clear();
                m_mapSpriteSheets.clear();
                for (auto& item : m_mapTextureNameId)
                {
                    glDeleteTextures(1, &item.second);
                }
                m_mapTextureNameId.clear();
            }

            // Load new compound
            bool _bRetVal = OpenJSONFile(m_sOpenFile);

            // If success, build actors for rendering
            if (_bRetVal == true)
            {
                auto _itCompound = m_mapCompounds.find(FileHelper::GetAbsolutePath(m_sOpenFile));
                if (_itCompound == m_mapCompounds.end())
                {
                    fprintf(stdout, "%s", "Couldn't find compound to build actor instances.");
                }
                else
                {
                    auto _pRootCompound = _itCompound->second;
                    m_vectorActorInstances = BuildActorInstances(_pRootCompound);
                }
            }

            m_sOpenFile = "";
        }
        //========================================


        std::string _sIndent;
        std::string _sTempHierarchy;

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

            m_fViewPortScale += static_cast<float>(m_dMouseScrollY) * 0.01f;
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


            if (m_vectorActorInstances.size() > 0)
            {
                if (m_bAnimate)
                {
                    m_fTime += float(_dDeltaTime) * m_fAnimationSpeedMult;
                }

                // create matrix stack
                std::vector<glm::mat4> _vectorMatrixStack;
                // push initial identity matrix
                _vectorMatrixStack.push_back(glm::mat4(1.0f));
                //_vectorMatrixStack.back() = glm::scale(_vectorMatrixStack.back(), glm::vec3(2, 2, 2));

                //========================================
                std::function<void(std::vector<SActorInstance> const &)> DrawActors;
                DrawActors = [&](std::vector<SActorInstance> const & _vectorActorInstances)->void
                {
                    // Draw the actors
                    for (auto const& _ActorInstance : _vectorActorInstances)
                    {
                        if (_ActorInstance.m_bShow == false)
                        {
                            continue;
                        }

                        auto _pCompound = _ActorInstance.m_pCompound;
                        auto _pActor = _pCompound->GetActorById(_ActorInstance.m_uActorId);

                        if (_pActor == nullptr)
                        {
                            assert(false);
                            continue;
                        }

                        float _fTime = fmodf(m_fTime, _pCompound->GetStageLength());
                        CCompoundSprite::SActorState _ActorState = _pCompound->GetStateForActorAtTime(_pActor->m_uID, _fTime);

                        _sTempHierarchy += _sIndent + _pActor->m_sSprite + "\n";

                        if (_ActorInstance.m_vectorActors.size() == 0)
                        {
                            std::string _sTexture = _pCompound->GetTextureForSprite(_pActor->m_sSprite);
                            m_mapTextureNameId[_sTexture];

                            CSpriteSheet const& _SpriteSheet = m_mapSpriteSheets[_sTexture];
                            auto const& _mapSprites = _SpriteSheet.GetSpriteData();
                            auto _itSprite = _mapSprites.find(_pActor->m_sSprite);
                            if (_itSprite != _mapSprites.end())
                            {
                                CSpriteSheet::SSpriteCell const& _Cell = _itSprite->second;

                                gl_render_helper::DrawSprite(_vectorMatrixStack.back(),
                                                             _Cell,
                                                             _ActorState,
                                                             program,
                                                             m_mapTextureNameId[_sTexture]);
                            }
                        }
                        else
                        {
                            // copy current matrix, modify for this actor and push onto our stack
                            glm::mat4 _matSub = _vectorMatrixStack.back();
                            _matSub = glm::translate(_matSub, glm::vec3(_ActorState.m_fPosX, _ActorState.m_fPosY, 0.0f));
                            _matSub = glm::scale(_matSub, glm::vec3(_ActorState.m_fScaleX, _ActorState.m_fScaleY, 0.0f));
                            _vectorMatrixStack.push_back(_matSub);

                            _sIndent += "\t";

                            DrawActors(_ActorInstance.m_vectorActors);

                            _sIndent = _sIndent.substr(0, _sIndent.size() - 1);

                            _vectorMatrixStack.pop_back();
                        }
                    }
                };
                //========================================

                DrawActors(m_vectorActorInstances);
                
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

                std::string const& sprites_window_id = "Sprite Sheets";
                ImGui::DockBuilderDockWindow(sprites_window_id.c_str(), dock_id_bottom);

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
                    if (ImGui::BeginMenu("File"))
                    {
                        if (ImGui::MenuItem("Open Compound Sprite..."))
                        {
                            m_sOpenFile = FileHelper::OpenFileDialog("json");
                        }
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Styles"))
                    {
                        for (auto _sItem : s_vectorStyles)
                        {
                            if (ImGui::MenuItem(_sItem.c_str()))
                            {
                                ChooseStyle(_sItem);
                            }
                        }
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("About"))
                    {
                        if (ImGui::MenuItem("Made By Argh#2682"))
                        {

                        }
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
                    //========================================
                    std::function<void(std::vector<SActorInstance> &)> DrawActorTimelines;
                    DrawActorTimelines = [&](std::vector<SActorInstance> & _vectorActorInstances)->void
                    {
                        // Draw the actors
                        for (auto & _ActorInstance : _vectorActorInstances)
                        {
                            ImGui::PushID(_ActorInstance.m_uActorId);

                            auto _pCompound = _ActorInstance.m_pCompound;
                            auto _pActor = _pCompound->GetActorById(_ActorInstance.m_uActorId);

                            if (_pActor != nullptr)
                            {

                                float _fTime = fmodf(m_fTime, _pCompound->GetStageLength());
                                CCompoundSprite::SActorState _ActorState = _pCompound->GetStateForActorAtTime(_pActor->m_uID, _fTime);

                                if (_ActorInstance.m_vectorActors.size() == 0)
                                {
                                    //std::string _sTexture = _pCompound->GetTextureForSprite(_pActor->m_sSprite);
                                    //m_mapTextureNameId[_sTexture];

                                    //CSpriteSheet const& _SpriteSheet = mapSpriteSheets[_sTexture];
                                    //auto const& _mapSprites = _SpriteSheet.GetSpriteData();
                                    //auto _itSprite = _mapSprites.find(_pActor->m_sSprite);
                                    //if (_itSprite != _mapSprites.end())
                                    //{
                                    //    CSpriteSheet::SSpriteCell const& _Cell = _itSprite->second;


                                    //    gl_render_helper::DrawSprite(_vectorMatrixStack.back(),
                                    //                                 _Cell,
                                    //                                 _ActorState,
                                    //                                 program,
                                    //                                 m_mapTextureNameId[_sTexture]);

                                    //}

                                    ImGui::Checkbox(_pActor->m_sSprite.c_str(), &_ActorInstance.m_bShow);
                                }
                                else
                                {
                                    ImGui::Checkbox(_pActor->m_sSprite.c_str(), &_ActorInstance.m_bShow);
                                    ImGui::Indent();
                                    DrawActorTimelines(_ActorInstance.m_vectorActors);
                                    ImGui::Unindent();
                                }
                            }

                            ImGui::PopID();
                        }
                    };
                    //========================================

                    ImGui::Text("ROOT");
                    DrawActorTimelines(m_vectorActorInstances);
                }
                ImGui::End();

                // Sprite sheet sprites
                if (ImGui::Begin("Sprite Sheets", nullptr))
                {
                    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
                    if (ImGui::BeginTabBar("sprite_sheets_tab_bar", tab_bar_flags))
                    {
                        for (auto& _SpriteSheetItem : m_mapSpriteSheets)
                        {
                            auto _itTexNameId = m_mapTextureNameId.find(_SpriteSheetItem.first);
                            if (_itTexNameId != m_mapTextureNameId.end())
                            {
                                if (ImGui::BeginTabItem(_SpriteSheetItem.first.c_str()))
                                {
                                    ui::SpriteSheetWindow(_SpriteSheetItem.second, _itTexNameId->second);
                                    ImGui::EndTabItem();
                                }
                            }
                        }
                        ImGui::EndTabBar();
                    }
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
