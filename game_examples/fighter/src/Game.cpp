#include "Game.hpp"
#include "characters/Roster.hpp"
#include "core/Assets.hpp"
#include "core/Text.hpp"
#include "scenes/TitleScene.hpp"
#include <utility>

namespace fighter {

Game::Game() = default;

Game::~Game()
{
    Shutdown();
}

void Game::Init()
{
    if (initialized_) return;
    initialized_ = true;

    TextInit();
    Roster::Instance().Load();
    background_.Load();

    ChangeScene(std::unique_ptr<Scene>(new TitleScene()));
    ApplyPendingScene();
}

void Game::ChangeScene(std::unique_ptr<Scene> next)
{
    pending_ = std::move(next);
}

void Game::ApplyPendingScene()
{
    if (!pending_) return;
    if (scene_) scene_->OnExit(*this);
    scene_ = std::move(pending_);
    pending_.reset();
    scene_->OnEnter(*this);
}

void Game::Update(float dt)
{
    if (!initialized_) Init();
    if (scene_) scene_->Update(*this, dt);
    // Đổi scene ở cuối frame: nếu xoá ngay giữa Update thì đang đứng trên
    // chính đối tượng vừa bị huỷ.
    ApplyPendingScene();
}

void Game::Draw()
{
    if (scene_) scene_->Draw(*this);
}

void Game::Shutdown()
{
    if (!initialized_) return;
    initialized_ = false;

    // Thứ tự quan trọng: các Fighter giữ tham chiếu tới CharacterDef trong
    // Roster, còn Roster giữ handle texture của Assets.
    scene_.reset();
    pending_.reset();
    Roster::Instance().Unload();
    Assets::Instance().UnloadAll();
    TextShutdown();
}

} // namespace fighter
