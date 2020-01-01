#include "gameview.hpp"

#include <fmt/format.h>

#include "dialog.hpp"
#include "file.hpp"
#include "imgui.hpp"
extern "C"
{
#include "style.h"
}

namespace
{
constexpr unsigned GameViewWidth = VITA_WIDTH * 0.8;
constexpr unsigned GameViewHeight = VITA_HEIGHT * 0.8;
}

GameView::GameView(
        const Config* config,
        Downloader* downloader,
        DbItem* item,
        std::optional<CompPackDatabase::Item> base_comppack,
        std::optional<CompPackDatabase::Item> patch_comppack)
    : _config(config)
    , _downloader(downloader)
    , _item(item)
    , _base_comppack(base_comppack)
    , _patch_comppack(patch_comppack)
    , _patch_info_fetcher(item->titleid)
    , _image_fetcher(item)
{
    refresh();
}

void GameView::render()
{
    ImGui::SetNextWindowPos(
            ImVec2((VITA_WIDTH - GameViewWidth) / 2,
                   (VITA_HEIGHT - GameViewHeight) / 2));
    ImGui::SetNextWindowSize(ImVec2(GameViewWidth, GameViewHeight), 0);

    char size_str[64];
    pkgi_friendly_size(size_str, sizeof(size_str), _item->size);
    ImGui::Begin(
            fmt::format("{} ({})###gameview", _item->titleid, size_str)
                    .c_str(),
            nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoInputs);

    ImGui::PushTextWrapPos(_image_fetcher.get_texture() == nullptr ?
        0.f : GameViewWidth - 300.f);
    ImGui::Text(fmt::format("当前系统固件版本: {}", pkgi_get_system_version())
                        .c_str());
    ImGui::Text(
            fmt::format(
                    "运行所需固件版本: {}", get_min_system_version())
                    .c_str());

    ImGui::Text(" ");

    ImGui::Text(fmt::format(
                        "游戏安装及版本更新情况: {}",
                        _game_version.empty() ? "未安装" : _game_version)
                        .c_str());
    if (_comppack_versions.present && _comppack_versions.base.empty() &&
        _comppack_versions.patch.empty())
    {
        ImGui::Text("已安装的游戏兼容包: 未知版本");
    }
    else
    {
        ImGui::Text(fmt::format(
                            "游戏本体兼容包安装情况: {}",
                            _comppack_versions.base.empty() ? "未安装" : "已安装")
                            .c_str());
        ImGui::Text(fmt::format(
                            "游戏更新兼容包及版本情况: {}",
                            _comppack_versions.patch.empty()
                                    ? "未安装"
                                    : _comppack_versions.patch)
                            .c_str());
    }

    ImGui::Text(" ");

    printDiagnostic();

    ImGui::Text(" ");

    ImGui::PopTextWrapPos();

    if (_patch_info_fetcher.get_status() == PatchInfoFetcher::Status::Found)
    {
        if (ImGui::Button("安装游戏本体及更新###installgame"))
            start_download_package();
    }
    else
    {
        if (ImGui::Button("安装游戏###installgame"))
            start_download_package();
    }

    if (_base_comppack)
    {
        if (!_downloader->is_in_queue(CompPackBase, _item->titleid))
        {
            if (ImGui::Button("安装游戏本体兼容"
                              "包###installbasecomppack"))
                start_download_comppack(false);
        }
        else
        {
            if (ImGui::Button("取消安装游戏本体兼容"
                              "包###installbasecomppack"))
                cancel_download_comppacks(false);
        }
    }
    if (_patch_comppack)
    {
        if (!_downloader->is_in_queue(CompPackPatch, _item->titleid))
        {
            if (ImGui::Button(fmt::format(
                                      "安装游戏更新兼容包"
                                      "{}###installpatchcommppack",
                                      _patch_comppack->app_version)
                                      .c_str()))
                start_download_comppack(true);
        }
        else
        {
            if (ImGui::Button("取消安装游戏更新兼容"
                              "包###installpatchcommppack"))
                cancel_download_comppacks(true);
        }
    }

    auto tex = _image_fetcher.get_texture();
    // Display game image
    if (tex != nullptr)
    {
        int tex_w = vita2d_texture_get_width(tex);
        int tex_h = vita2d_texture_get_height(tex);
        float tex_x = ImGui::GetWindowContentRegionMax().x - tex_w;
        float tex_y = ImGui::GetWindowContentRegionMin().y;
        ImGui::SetCursorPos(ImVec2(tex_x, tex_y));
        ImGui::Image(tex, ImVec2(tex_w, tex_h));
    }

    ImGui::End();
}

static const auto Red = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
static const auto Yellow = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
static const auto Green = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

void GameView::printDiagnostic()
{
    bool ok = true;
    auto const printError = [&](auto const& str) {
        ok = false;
        ImGui::TextColored(Red, str);
    };

    auto const systemVersion = pkgi_get_system_version();
    auto const minSystemVersion = get_min_system_version();

    ImGui::Text("运行诊断:");

    if (systemVersion < minSystemVersion)
    {
        if (!_comppack_versions.present)
        {
            if (_refood_present)
                ImGui::Text(
                        "- 游戏将通过 reF00D 插件引导运行, "
                        "安装兼容包可有效缩短游戏启动所需时间");
            else if (_0syscall6_present)
                ImGui::Text(
                        "- 游戏将通过 0syscall6 插件引导运行");
            else
                printError(
                        "- 当前系统固件版本低于游戏运行所需固件版本,"
                        "安装兼容包或安装 reF00D 插件或 0syscall6 插件");
        }
    }
    else
    {
        ImGui::Text(
                "- 当前系统固件版本已高于游戏运行所需固件版本,"
                "无需安装兼容包");
    }

    if (_comppack_versions.present && _comppack_versions.base.empty() &&
        _comppack_versions.patch.empty())
    {
        ImGui::TextColored(
                Yellow,
                "- 游戏兼容包已安装, 但并非通过PKGj进行安装, 请"
                "确保该兼容包与游戏版本相匹配，如出现运行异常, 请通过PKGj"
                "重新安装");
        ok = false;
    }

    if (_comppack_versions.base.empty() && !_comppack_versions.patch.empty())
        printError(
                "- 已安装游戏更新兼容包, 但未安装"
                "游戏本体兼容包, 请先安装游戏本体兼容包, "
                "再安装游戏更新兼容包");

    std::string comppack_version;
    if (!_comppack_versions.patch.empty())
        comppack_version = _comppack_versions.patch;
    else if (!_comppack_versions.base.empty())
        comppack_version = _comppack_versions.base;

    if (_item->presence == PresenceInstalled && !comppack_version.empty() &&
        comppack_version < _game_version)
        printError(
                "- 游戏版本与已安装的兼容包版本"
                "不匹配, 如游戏版本已更新, 请"
                "安装游戏更新兼容包");

    if (_item->presence == PresenceInstalled &&
        comppack_version > _game_version)
        printError(
                "- 游戏版本与已安装的兼容包版本"
                "不匹配, 请删除游戏更新文件恢复至初始版本, "
                "或通过LiveArea更新游戏版本");

    if (_item->presence != PresenceInstalled)
    {
        ImGui::Text("- 未安装游戏");
        ok = false;
    }

    if (ok)
        ImGui::TextColored(Green, "已满足运行条件");
}

std::string GameView::get_min_system_version()
{
    auto const patchInfo = _patch_info_fetcher.get_patch_info();
    if (patchInfo)
        return patchInfo->fw_version;
    else
        return _item->fw_version;
}

void GameView::refresh()
{
    LOGF("refreshing gameview");
    _refood_present = pkgi_is_module_present("ref00d");
    _0syscall6_present = pkgi_is_module_present("0syscall6");
    _game_version = pkgi_get_game_version(_item->partition, _item->titleid);
    _comppack_versions = pkgi_get_comppack_versions(_item->partition, _item->titleid);
}

void GameView::start_download_package()
{
    if (_item->presence == PresenceInstalled)
    {
        LOGF("[{}] {} - already installed", _item->titleid, _item->name);
        pkgi_dialog_error("已安装");
        return;
    }

    pkgi_start_download(*_downloader, *_item);

    _item->presence = PresenceUnknown;
}

void GameView::cancel_download_package()
{
    _downloader->remove_from_queue(Game, _item->content);
    _item->presence = PresenceUnknown;
}

void GameView::start_download_comppack(bool patch)
{
    const auto& entry = patch ? _patch_comppack : _base_comppack;

    _downloader->add(DownloadItem{patch ? CompPackPatch : CompPackBase,
                                  _item->name,
                                  _item->titleid,
                                  _config->comppack_url + entry->path,
                                  std::vector<uint8_t>{},
                                  std::vector<uint8_t>{},
                                  false,
                                  _config->install_psv_location,
                                  _config->install_psp_game_path,
                                  _config->install_psp_iso_path,
                                  _config->install_psp_psx_path,
                                  entry->app_version});
}

void GameView::cancel_download_comppacks(bool patch)
{
    _downloader->remove_from_queue(
            patch ? CompPackPatch : CompPackBase, _item->titleid);
}
