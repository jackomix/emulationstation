#include "FileData.h"
#include "ProfileManager.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include "AudioManager.h"
#include "CollectionSystemManager.h"
#include "FileFilterIndex.h"
#include "FileSorts.h"
#include "Log.h"
#include "MameNames.h"
#include "utils/Platform.h"
#include "Scripting.h"
#include "SystemData.h"
#include "VolumeControl.h"
#include "Window.h"
#include "views/UIModeController.h"
#include <assert.h>
#include "SystemConf.h"
#include "InputManager.h"
#include "RetroAchievements.h"
#include "scrapers/ThreadedScraper.h"
#include "Gamelist.h" 
#include "ApiSystem.h"
#include <time.h>
#include <algorithm>
#include "LangParser.h"
#include "resources/ResourceManager.h"
#include "RetroAchievements.h"
#include "SaveStateRepository.h"
#include "Genres.h"
#include "TextToSpeech.h"
#include "LocaleES.h"
#include "guis/GuiMsgBox.h"
#include "Paths.h"
#include "resources/TextureData.h"

using namespace Utils::Platform;

static std::map<std::string, std::function<BindableProperty(FileData*)>> properties =
{
	{ "name",				[](FileData* file) { return file->getName(); } },
	{ "rom",				[](FileData* file) { return BindableProperty(Utils::FileSystem::getFileName(file->getPath()), BindablePropertyType::String); } },
	{ "stem",				[](FileData* file) { return BindableProperty(Utils::FileSystem::getStem(file->getPath()), BindablePropertyType::String); } },
	{ "path",				[](FileData* file) { return BindableProperty(file->getPath(), BindablePropertyType::Path); } },
	{ "image",				[](FileData* file) { return BindableProperty(file->getImagePath(), BindablePropertyType::Path); } },
	{ "thumbnail",			[](FileData* file) { return BindableProperty(file->getThumbnailPath(false), BindablePropertyType::Path); } },
	{ "video",				[](FileData* file) { return BindableProperty(file->getVideoPath(), BindablePropertyType::Path); } },
	{ "marquee",			[](FileData* file) { return BindableProperty(file->getMarqueePath(), BindablePropertyType::Path); } },
	{ "favorite",			[](FileData* file) { return BindableProperty(file->getFavorite()); } },
	{ "hidden",				[](FileData* file) { return BindableProperty(file->getHidden()); } },
	{ "kidGame",			[](FileData* file) { return BindableProperty(file->getKidGame()); } },
	{ "gunGame",			[](FileData* file) { return BindableProperty(file->isLightGunGame()); } },
	{ "wheelGame",			[](FileData* file) { return BindableProperty(file->isWheelGame()); } },
	{ "trackballGame",			[](FileData* file) { return BindableProperty(file->isTrackballGame()); } },
	{ "spinnerGame",			[](FileData* file) { return BindableProperty(file->isSpinnerGame()); } },
	{ "cheevos",			[](FileData* file) { return BindableProperty(file->hasCheevos()); } },
	{ "genre",			    [](FileData* file) { return BindableProperty(file->getGenre(), BindablePropertyType::String); } },
	{ "hasKeyboardMapping", [](FileData* file) { return BindableProperty(file->hasKeyboardMapping()); } },	
	{ "systemName",			[](FileData* file) { return BindableProperty(file->getSourceFileData()->getSystem()->getFullName(), BindablePropertyType::String); } },
};

FileData* FileData::mRunningGame = nullptr;

FileData::FileData(FileType type, const std::string& path, SystemData* system)
	: mPath(path), mType(type), mSystem(system), mParent(nullptr), mDisplayName(nullptr), mSortName(nullptr), mMetadata(type == GAME ? GAME_METADATA : FOLDER_METADATA) 
{
	if (mMetadata.get(MetaDataId::Name).empty() && !mPath.empty())
		mMetadata.set(MetaDataId::Name, getDisplayName());
	
	mMetadata.resetChangedFlag();
}

const std::string FileData::getPath() const
{
	if (mPath.empty())
		return getSystemEnvData()->mStartPath;

	return mPath;
}

const std::string FileData::getBreadCrumbPath()
{
	std::vector<std::string> paths;
	FileData* root = getSystem()->getParentGroupSystem() != nullptr ? getSystem()->getParentGroupSystem()->getRootFolder() : getSystem()->getRootFolder();
	FileData* parent = (getType() == GAME ? getParent() : this);
	while (parent != nullptr)
	{
		if (parent == root->getSystem()->getRootFolder() && !parent->getSystem()->isCollection())
			break;
		if (parent->getSystem()->getName() == CollectionSystemManager::get()->getCustomCollectionsBundle()->getName())
			break;
		if (parent->getSystem()->isGroupChildSystem() && parent->getSystem()->getParentGroupSystem() != nullptr && parent->getParent() == parent->getSystem()->getParentGroupSystem()->getRootFolder() && parent->getSystem()->getName() != "windows_installers")
			break;
		paths.push_back(parent->getName());
		parent = parent->getParent();
	}
	std::reverse(paths.begin(), paths.end());
	return Utils::String::join(paths, " > ");
}

const std::string FileData::getConfigurationName()
{
	std::string gameConf = Utils::FileSystem::getFileName(getPath());
	gameConf = Utils::String::replace(gameConf, "=", "");
	gameConf = Utils::String::replace(gameConf, "#", "");
	gameConf = getSourceFileData()->getSystem()->getName() + std::string("[\"") + gameConf + std::string("\"]");
	return gameConf;
}

inline SystemEnvironmentData* FileData::getSystemEnvData() const
{ 
	return mSystem->getSystemEnvData(); 
}

std::string FileData::getSystemName() const
{
	return mSystem->getName();
}

FileData::~FileData()
{
	if (mDisplayName) delete mDisplayName;
#ifdef _ENABLEAMBERELEC
    if (mSortName) delete mSortName;
#endif
	if (mParent) mParent->removeChild(this);
	if (mType == GAME) mSystem->removeFromIndex(this);
}

std::string& FileData::getDisplayName()
{
	if (mDisplayName == nullptr)
	{
		std::string stem = Utils::FileSystem::getStem(getPath());
		if (mSystem && (mSystem->hasPlatformId(PlatformIds::ARCADE) || mSystem->hasPlatformId(PlatformIds::NEOGEO)))
			stem = MameNames::getInstance()->getRealName(stem);
		mDisplayName = new std::string(stem);
	}
	return *mDisplayName;
}

std::string FileData::getCleanName() { return Utils::String::removeParenthesis(getDisplayName()); }

std::string FileData::findLocalArt(const std::string& type, std::vector<std::string> exts)
{
	if (Settings::getInstance()->getBool("LocalArt"))
	{
		for (auto ext : exts)
		{
			std::string path = getSystemEnvData()->mStartPath + "/images/" + getDisplayName() + (type.empty() ? "" :  "-" + type) + ext;
			if (Utils::FileSystem::exists(path)) return path;
		}
	}
	return "";
}

const std::string FileData::getThumbnailPath(bool fallbackWithImage)
{
	std::string thumbnail = getMetadata(MetaDataId::Thumbnail);
	if (thumbnail.empty()) {
		thumbnail = findLocalArt("thumb");
		if (!thumbnail.empty()) setMetadata(MetaDataId::Thumbnail, thumbnail);
	}
	return thumbnail;
}

const bool FileData::getFavorite() const
{
	return ProfileManager::getInstance()->isFavorite(getPath());
}

std::string FileData::getMetadata(MetaDataId key) const
{
	if (getType() == GAME)
	{
		if (key == MetaDataId::Favorite)
			return ProfileManager::getInstance()->isFavorite(getPath()) ? "true" : "false";
		
		if (key == MetaDataId::PlayCount || key == MetaDataId::GameTime || key == MetaDataId::LastPlayed)
		{
			std::string keyStr = (key == MetaDataId::PlayCount) ? "playcount" : (key == MetaDataId::GameTime ? "playtime" : "last_played");
			return ProfileManager::getInstance()->getStat(keyStr);
		}
	}
	return mMetadata.get(key);
}

void FileData::setMetadata(MetaDataId key, const std::string& value)
{
	if (getType() == GAME)
	{
		if (key == MetaDataId::Favorite)
		{
			ProfileManager::getInstance()->setFavorite(getPath(), value == "true");
			return;
		}
	}
	mMetadata.set(key, value);
}

const bool FileData::getHidden() const { return getMetadata(MetaDataId::Hidden) == "true"; }
const bool FileData::getKidGame() const { auto data = getMetadata(MetaDataId::KidGame); return data != "false" && !data.empty(); }

const bool FileData::hasCheevos()
{
	if (Utils::String::toInteger(getMetadata(MetaDataId::CheevosId)) > 0)
		return getSourceFileData()->getSystem()->isCheevosSupported();
	if (RetroAchievements::isLocalEngineActive()) return true;
	return false;
}

bool FileData::hasAnyMedia()
{
	if (Utils::FileSystem::exists(getImagePath()) || Utils::FileSystem::exists(getThumbnailPath(false)) || Utils::FileSystem::exists(getVideoPath()))
		return true;

	for (auto mdd : mMetadata.getMDD())
	{
		if (mdd.type != MetaDataType::MD_PATH)
			continue;

		std::string path = mMetadata.get(mdd.key);
		if (path.empty())
			continue;

		if (mdd.id == MetaDataId::Manual || mdd.id == MetaDataId::Magazine)
		{
			if (Utils::FileSystem::exists(path))
				return true;
		}
		else if (mdd.id != MetaDataId::Image && mdd.id != MetaDataId::Thumbnail)
		{
			if (Utils::FileSystem::isImage(path))
				continue;

			if (Utils::FileSystem::exists(path))
				return true;
		}
	}

	return false;
}

std::vector<std::string> FileData::getFileMedias()
{
	std::vector<std::string> ret;

	for (auto mdd : mMetadata.getMDD())
	{
		if (mdd.type != MetaDataType::MD_PATH)
			continue;

		if (mdd.id == MetaDataId::Video || mdd.id == MetaDataId::Manual || mdd.id == MetaDataId::Magazine)
			continue;

		std::string path = mMetadata.get(mdd.key);
		if (path.empty())
			continue;

		if (!Utils::FileSystem::isImage(path))
			continue;
		
		if (Utils::FileSystem::exists(path))
			ret.push_back(path);
	}

	return ret;
}

void FileData::resetSettings() { }

const std::string& FileData::getName()
{
	if (mSystem != nullptr && mSystem->getShowFilenames()) return getDisplayName();
	return mMetadata.getName();
}

const std::string& FileData::getSortName()
{
    if (mSortName == nullptr) mSortName = new std::string(getMetadata(MetaDataId::SortName));
    return *mSortName;
}

const std::string FileData::getSortOrName()
{
	std::string s(getSortName());
	if (!s.empty()) return s;
	return getName();
}

const std::string FileData::getVideoPath() { return getMetadata(MetaDataId::Video); }
const std::string FileData::getMarqueePath() { return getMetadata(MetaDataId::Marquee); }
const std::string FileData::getImagePath() { return getMetadata(MetaDataId::Image); }

std::string FileData::getKey() { return getFileName(); }

const bool FileData::isArcadeAsset()
{
	if (mSystem && (mSystem->hasPlatformId(PlatformIds::ARCADE) || mSystem->hasPlatformId(PlatformIds::NEOGEO)))
	{	
		const std::string stem = Utils::FileSystem::getStem(getPath());
		return MameNames::getInstance()->isBiosOrDevice(stem);		
	}
	return false;
}

const bool FileData::isVerticalArcadeGame()
{
	if (mSystem && mSystem->hasPlatformId(PlatformIds::ARCADE))
		return MameNames::getInstance()->isVertical(Utils::FileSystem::getStem(getPath()));
	return false;
}

const bool FileData::isLightGunGame()
{
	return MameNames::getInstance()->isLightgun(Utils::FileSystem::getStem(getPath()), mSystem->getName(), mSystem && mSystem->hasPlatformId(PlatformIds::ARCADE));
}

const bool FileData::isWheelGame()
{
	return MameNames::getInstance()->isWheel(Utils::FileSystem::getStem(getPath()), mSystem->getName(), mSystem && mSystem->hasPlatformId(PlatformIds::ARCADE));
}

const bool FileData::isTrackballGame()
{
	return MameNames::getInstance()->isTrackball(Utils::FileSystem::getStem(getPath()), mSystem->getName(), mSystem && mSystem->hasPlatformId(PlatformIds::ARCADE));
}

const bool FileData::isSpinnerGame()
{
	return MameNames::getInstance()->isSpinner(Utils::FileSystem::getStem(getPath()), mSystem->getName(), mSystem && mSystem->hasPlatformId(PlatformIds::ARCADE));
}

FileData* FileData::getSourceFileData() { return this; }

bool FileData::launchGame(Window* window, LaunchGameOptions options)
{
	LOG(LogInfo) << "Attempting to launch game...";
	FileData* gameToUpdate = getSourceFileData();
	SystemData* system = gameToUpdate->getSystem();
	std::string command = getlaunchCommand(options);

	AudioManager::getInstance()->deinit();
	VolumeControl::getInstance()->deinit();

	bool hideWindow = Settings::getInstance()->getBool("HideWindow");
	window->deinit(hideWindow);

	RetroAchievements::updateRetroArchConfig();
	time_t tstart = time(NULL);

	ProcessStartInfo process(command);
	process.window = hideWindow ? NULL : window;
	int exitCode = process.run();

	mRunningGame = nullptr;
	window->init(hideWindow);
	VolumeControl::getInstance()->init();
	AudioManager::getInstance()->init();

	if (exitCode == 0)
	{
		time_t tend = time(NULL);
		long elapsedSeconds = difftime(tend, tstart);
		if (elapsedSeconds >= 10)
		{
			ProfileManager::getInstance()->updateStats(gameToUpdate->getPath(), (int)elapsedSeconds);
		}
		CollectionSystemManager::get()->refreshCollectionSystems(gameToUpdate);
	}
	return exitCode == 0;
}

CollectionFileData::CollectionFileData(FileData* file, SystemData* system)
	: FileData(file->getSourceFileData()->getType(), "", system)
{
	mSourceFileData = file->getSourceFileData();
	mParent = NULL;	
}

SystemEnvironmentData* CollectionFileData::getSystemEnvData() const { return mSourceFileData->getSystemEnvData(); }
const std::string CollectionFileData::getPath() const { return mSourceFileData->getPath(); }
std::string CollectionFileData::getSystemName() const { return mSourceFileData->getSystem()->getName(); }

CollectionFileData::~CollectionFileData()
{
	if(mParent) mParent->removeChild(this);
	mParent = NULL;
}

std::string CollectionFileData::getMetadata(MetaDataId key) const { return mSourceFileData->getMetadata(key); }
void CollectionFileData::setMetadata(MetaDataId key, const std::string& value) { mSourceFileData->setMetadata(key, value); }
std::string CollectionFileData::getKey() { return getFullPath(); }
FileData* CollectionFileData::getSourceFileData() { return mSourceFileData; }
const std::string& CollectionFileData::getName() { return mSourceFileData->getName(); }

FolderData::FolderData(const std::string& startpath, SystemData* system, bool ownsChildrens) : FileData(FOLDER, startpath, system)
{
	mIsDisplayableAsVirtualFolder = false;
	mOwnsChildrens = ownsChildrens;
}

FolderData::~FolderData() { clear(); }

void FolderData::clear() {
	if (mOwnsChildrens)
		for (auto* child : mChildren)
		{
			child->setParent(nullptr);
			delete child;
		}
	mChildren.clear();
}

void FolderData::bulkRemoveChildren(std::vector<FileData*>& mChildren, const std::unordered_set<FileData*>& filesToRemove)
{
	mChildren.erase(std::remove_if(mChildren.begin(), mChildren.end(), [&](FileData* file) { return filesToRemove.find(file) != filesToRemove.end(); }), mChildren.end());
}

void FolderData::createChildrenByFilenameMap(std::unordered_map<std::string, FileData*>& map)
{
	for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
		map[(*it)->getFileName()] = *it;
}

FileData* FolderData::findUniqueGameForFolder()
{
	FileData* unique = nullptr;

	for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
	{
		if ((*it)->getType() == GAME)
		{
			if (unique != nullptr) return nullptr;
			unique = *it;
		}
		else if ((*it)->getType() == FOLDER)
		{
			FileData* folderUnique = ((FolderData*)(*it))->findUniqueGameForFolder();
			if (folderUnique == nullptr) return nullptr;
			if (unique != nullptr) return nullptr;
			unique = folderUnique;
		}
	}

	return unique;
}

std::vector<FileData*> FolderData::getFlatGameList(bool displayedOnly, SystemData* system) const
{
	std::vector<FileData*> ret;
	for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
	{
		if ((*it)->getType() == GAME)
		{
			if (!displayedOnly || !(*it)->getHidden()) ret.push_back(*it);
		}
		else if ((*it)->getType() == FOLDER)
		{
			auto folderList = ((FolderData*)(*it))->getFlatGameList(displayedOnly, system);
			ret.insert(ret.end(), folderList.begin(), folderList.end());
		}
	}
	return ret;
}

void FolderData::removeVirtualFolders() {
	if (!mOwnsChildrens) return;
	std::unordered_set<FileData*> filesToRemove;
	for (auto file : mChildren)
		if (file->getType() == FOLDER && !((FolderData*)file)->mOwnsChildrens)
			filesToRemove.insert(file);
	bulkRemoveChildren(mChildren, filesToRemove);
	for (auto file : filesToRemove) delete file;
}

void FolderData::removeFromVirtualFolders(FileData* game)
{
	for (auto it = mChildren.begin(); it != mChildren.end(); ++it) 
	{		
		if ((*it)->getType() == FOLDER) {
			((FolderData*)(*it))->removeFromVirtualFolders(game);
			continue;
		}
		if ((*it) == game) {
			mChildren.erase(it);
			return;
		}
	}
}
