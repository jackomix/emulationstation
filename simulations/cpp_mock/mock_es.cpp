#include "Window.h"
#include "Settings.h"
#include "Log.h"
#include "renderers/Renderer.h"
#include "resources/Font.h"
#include "ThemeData.h"

// Stubs for compiling the components. We will add to this iteratively.

const char* ngettext(const char* msgid, const char* msgid_plural, unsigned long int n) { return n == 1 ? msgid : msgid_plural; }
const char* pgettext(const char* context, const char* msgid) { return msgid; }
std::string EsLocale::init(std::string locale, std::string path) { return locale; }
std::string EsLocale::changeLocale(const std::string& locale) { return locale; }
const bool EsLocale::isRTL() { return false; }
std::string EsLocale::default_LANGUAGE = "en";

#include "ApiSystem.h"

// Stubs for ApiSystem
ApiSystem* ApiSystem::instance = nullptr;
ApiSystem* ApiSystem::getInstance() {
    if (!instance) instance = new ApiSystem();
    return instance;
}
ApiSystem::ApiSystem() {}
bool ApiSystem::isScriptingSupported(ScriptId) { return false; }
unsigned long ApiSystem::getFreeSpaceGB(std::string) { return 0; }
std::string ApiSystem::getFreeSpaceUserInfo() { return ""; }
std::string ApiSystem::getFreeSpaceSystemInfo() { return ""; }
std::string ApiSystem::getVersion(bool) { return ""; }
std::string ApiSystem::getApplicationName() { return ""; }
std::pair<std::string, int> ApiSystem::updateSystem(const std::function<void(const std::string)>&) { return {"", 0}; }
bool ApiSystem::ping() { return true; }
bool ApiSystem::canUpdate(std::vector<std::string>&) { return false; }
void ApiSystem::setReadyFlag(bool) {}
bool ApiSystem::isReadyFlagSet() { return true; }
bool ApiSystem::launchKodi(Window*) { return false; }
std::string ApiSystem::getIpAddress() { return ""; }
std::vector<std::string> ApiSystem::getVideoModes(const std::string) { return {}; }
std::vector<std::string> ApiSystem::getAvailableStorageDevices() { return {}; }
std::vector<std::string> ApiSystem::getSystemInformations() { return {}; }
std::vector<BatoceraTheme> ApiSystem::getBatoceraThemesList() { return {}; }
bool ApiSystem::isThemeInstalled(const std::string&, const std::string&) { return false; }
std::pair<std::string,int> ApiSystem::installBatoceraTheme(std::string, const std::function<void(const std::string)>&) { return {"", 0}; }
std::pair<std::string, int> ApiSystem::uninstallBatoceraBezel(std::string, const std::function<void(const std::string)>&) { return {"", 0}; }
std::vector<BatoceraBezel> ApiSystem::getBatoceraBezelsList() { return {}; }
std::pair<std::string,int> ApiSystem::installBatoceraBezel(std::string, const std::function<void(const std::string)>&) { return {"", 0}; }
std::pair<std::string,int> ApiSystem::uninstallBatoceraTheme(std::string, const std::function<void(const std::string)>&) { return {"", 0}; }
std::string ApiSystem::getCRC32(const std::string, bool) { return ""; }
std::string ApiSystem::getMD5(const std::string, bool) { return ""; }
bool ApiSystem::unzipFile(const std::string, const std::string, const std::function<bool(const std::string)>&) { return false; }
int ApiSystem::getPdfPageCount(const std::string&) { return 0; }
std::vector<std::string> ApiSystem::extractPdfImages(const std::string&, int, int, int) { return {}; }
std::string ApiSystem::getRunningArchitecture() { return ""; }
std::string ApiSystem::getRunningBoard() { return ""; }
std::vector<std::string> ApiSystem::getRetroachievementsSoundsList() { return {}; }
std::vector<std::string> ApiSystem::getVideoFilterList(const std::string&, const std::string&, const std::string&) { return {}; }
std::vector<std::string> ApiSystem::getShaderList(const std::string&, const std::string&, const std::string&) { return {}; }
std::vector<std::string> ApiSystem::getTimezones() { return {}; }
std::string ApiSystem::getCurrentTimezone() { return ""; }
bool ApiSystem::setTimezone(std::string) { return false; }
std::vector<PadInfo> ApiSystem::getPadsInfo() { return {}; }
std::string ApiSystem::getHostsName() { return ""; }
bool ApiSystem::emuKill() { return false; }
void ApiSystem::suspend() {}
void ApiSystem::replugControllers_sindenguns() {}
void ApiSystem::replugControllers_wiimotes() {}
void ApiSystem::replugControllers_steamdeckguns() {}
bool ApiSystem::isPlaneMode() { return false; }
bool ApiSystem::setPlaneMode(bool) { return false; }
bool ApiSystem::isReadPlaneModeSupported() { return false; }
std::vector<Service> ApiSystem::getServices() { return {}; }
bool ApiSystem::enableService(std::string, bool) { return false; }
std::vector<std::string> ApiSystem::backglassThemes() { return {}; }
void ApiSystem::restartBackglass() {}
bool ApiSystem::executeScript(const std::string) { return false; }
std::pair<std::string, int> ApiSystem::executeScript(const std::string, const std::function<void(const std::string)>&) { return {"", 0}; }
std::vector<std::string> ApiSystem::executeEnumerationScript(const std::string) { return {}; }
bool ApiSystem::downloadGitRepository(const std::string&, const std::string&, const std::string&, const std::string&, const std::function<void(const std::string)>&, int64_t) { return false; }
std::string ApiSystem::getGitRepositoryDefaultBranch(const std::string&) { return ""; }
std::string ApiSystem::getUpdateUrl() { return ""; }
std::string ApiSystem::getThemesUrl() { return ""; }



#include "Sound.h"
#include "Window.h"
#include "ImageIO.h"
#include "utils/ZipFile.h"

// Sound
std::shared_ptr<Sound> Sound::get(const std::string&) { return nullptr; }
void Sound::play() {}

unsigned int Utils::Zip::ZipFile::computeCRC(unsigned int, const void*, size_t) { return 0; }

// Window
void Window::setHelpPrompts(const std::vector<HelpPrompt>&, const HelpStyle&) {}
void Window::renderSplashScreen(float, bool) {}
GuiComponent* Window::peekGui() { return nullptr; }
void Window::pushGui(GuiComponent*) {}
void Window::removeGui(GuiComponent*) {}

// ImageIO
bool ImageIO::loadImageSize(const std::string&, unsigned int*, unsigned int*) { return false; }
void ImageIO::flipPixelsVert(unsigned char*, const size_t&, const size_t&) {}
void ImageIO::removeImageCache(const std::string&) {}
Vector2i ImageIO::adjustPictureSize(Vector2i, Vector2i, bool) { return {0,0}; }
Vector2f ImageIO::adjustPictureSizeF(Vector2f, Vector2f, bool) { return {0,0}; }
unsigned char* ImageIO::loadFromMemoryRGBA32(const unsigned char*, const size_t, size_t&, size_t&, MaxSizeInfo*, Vector2i*, Vector2i*, int) { return nullptr; }
bool ImageIO::getMultiBitmapInformation(const std::string&, int&, int&) { return false; }

// Dummy Components

// RetroAchievements


#include "components/VideoVlcComponent.h"
#include "components/WebImageComponent.h"
#include "components/TextEditComponent.h"
#include "components/AnimatedImageComponent.h"
#include "FileData.h"

// VideoVlcComponent
VideoVlcComponent::VideoVlcComponent(Window* w) : VideoComponent(w) {}

// WebImageComponent
WebImageComponent::WebImageComponent(Window* w, double) : ImageComponent(w) {}

// TextEditComponent
TextEditComponent::TextEditComponent(Window* w) : GuiComponent(w) {}
void TextEditComponent::startEditing() {}
void TextEditComponent::setCursor(size_t) {}

// AnimatedImageComponent
AnimatedImageComponent::AnimatedImageComponent(Window* w) : ImageComponent(w) {}
void AnimatedImageComponent::load(const AnimationDef*) {}

// FolderData
std::vector<FileData*> FolderData::getFilesRecursive(unsigned int, bool, SystemData*, bool) const { return {}; }

// MultiLineMenuEntry (we don't have its header included, so define a stub if we can, but it might be used by value)
// Wait, MultiLineMenuEntry is in es-core/src/components/MenuComponent.h or something. Let me check its constructor.

// Watchers
#include "watchers/WatchersManager.h"
#include "watchers/BatteryLevelWatcher.h"
#include "watchers/NetworkStateWatcher.h"
// If typeinfo is missing, we need the first virtual method of these Watchers
// They inherit from IWatcher? Let's compile WatchersManager.cpp and BatteryLevelWatcher.cpp instead?
