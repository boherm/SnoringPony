/*
  ==============================================================================

    PonyEngine.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "CueList/CueListManager.h"
#include "MainIncludes.h"
#include "PonyEngine.h"
#include "UserInputManager.h"

ControllableContainer* getAppSettings();
String getAppVersion();

PonyEngine::PonyEngine() :
	Engine("SnoringPony", ".indy")
	//ossiaDevice(nullptr)
{
	//init here
	Engine::mainEngine = this;
	addChildControllableContainer(CueListManager::getInstance());
	
	// Clean
	getAppSettings()->hideInEditor = true;
	GlobalSettings::getInstance()->getControllableContainerByName("editing")->hideInEditor = true;
	GlobalSettings::getInstance()->getControllableContainerByName("oscRemoteControl")
	->getControllableContainerByName("manualOSCSend")->editorIsCollapsed = true;
	GlobalSettings::getInstance()->getControllableContainerByName("launchArguments")->editorIsCollapsed = true;
    GlobalSettings::getInstance()->askBeforeRemovingItems->setValue(true);

	OSCRemoteControl::getInstance()->addRemoteControlListener(UserInputManager::getInstance());
}

PonyEngine::~PonyEngine()
{
	//Application-end cleanup, nothing should be recreated after this
	//delete singletons here

	isClearing = true;

	CueListManager::deleteInstance();
}

void PonyEngine::clearInternal()
{
	//clear
	//StateManager::getInstance()->clear();
	//ChataigneSequenceManager::getInstance()->clear();

	//ModuleRouterManager::getInstance()->clear();
	//ModuleManager::getInstance()->clear();
	//CVGroupManager::getInstance()->clear();
	CueListManager::getInstance()->clear();
}

var PonyEngine::getJSONData(bool includeNonOverriden)
{
	var data = Engine::getJSONData(includeNonOverriden);

	var clData = CueListManager::getInstance()->getJSONData();
	if (!clData.isVoid() && clData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty(CueListManager::getInstance()->shortName, clData);

	/*var mData = ModuleManager::getInstance()->getJSONData();
	if (!mData.isVoid() && mData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty(ModuleManager::getInstance()->shortName, mData);

	var cvData = CVGroupManager::getInstance()->getJSONData();
	if (!cvData.isVoid() && cvData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty(CVGroupManager::getInstance()->shortName, cvData);

	var sData = StateManager::getInstance()->getJSONData();
	if (!sData.isVoid() && sData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty(StateManager::getInstance()->shortName, sData);

	var seqData = ChataigneSequenceManager::getInstance()->getJSONData();
	if (!seqData.isVoid() && seqData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty(ChataigneSequenceManager::getInstance()->shortName, seqData);

	var rData = ModuleRouterManager::getInstance()->getJSONData();
	if (!rData.isVoid() && rData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty(ModuleRouterManager::getInstance()->shortName, rData);
	 */
	return data;
}

void PonyEngine::loadJSONDataInternalEngine(var data, ProgressTask* loadingTask)
{
	ProgressTask* clTask = loadingTask->addTask("CueLists");
	clTask->start();
	var clData = data.getProperty(CueListManager::getInstance()->shortName, var());
	CueListManager::getInstance()->loadJSONData(clData);
	clTask->setProgress(1);
	clTask->end();

	/*
	ProgressTask* moduleTask = loadingTask->addTask("Modules");
	ProgressTask* cvTask = loadingTask->addTask("Custom Variables");
	ProgressTask* stateTask = loadingTask->addTask("States");
	ProgressTask* sequenceTask = loadingTask->addTask("Sequences");
	ProgressTask* routerTask = loadingTask->addTask("Router");

	ModuleManager::getInstance()->factory->updateCustomModules(false);

	moduleTask->start();
	ModuleManager::getInstance()->loadJSONData(data.getProperty(ModuleManager::getInstance()->shortName, var()));
	moduleTask->setProgress(1);
	moduleTask->end();

	cvTask->start();
	CVGroupManager::getInstance()->loadJSONData(data.getProperty(CVGroupManager::getInstance()->shortName, var()));
	cvTask->setProgress(1);
	cvTask->end();

	stateTask->start();
	StateManager::getInstance()->loadJSONData(data.getProperty(StateManager::getInstance()->shortName, var()));
	stateTask->setProgress(1);
	stateTask->end();

	sequenceTask->start();
	ChataigneSequenceManager::getInstance()->loadJSONData(data.getProperty(ChataigneSequenceManager::getInstance()->shortName, var()));
	sequenceTask->setProgress(1);
	sequenceTask->end();

	routerTask->start();
	ModuleRouterManager::getInstance()->loadJSONData(data.getProperty(ModuleRouterManager::getInstance()->shortName, var()));
	routerTask->setProgress(1);
	routerTask->end();
	 */
}

void PonyEngine::childStructureChanged(ControllableContainer* cc)
{
	Engine::childStructureChanged(cc);
}

void PonyEngine::controllableFeedbackUpdate(ControllableContainer* cc, Controllable* c)
{
	if (isClearing || isLoadingFile) return;
}

void PonyEngine::handleAsyncUpdate()
{
	Engine::handleAsyncUpdate();
}

void PonyEngine::importSelection(File f)
{
	if (!f.existsAsFile())
	{
		FileChooser* fc(new FileChooser("Load a LilNut", File::getCurrentWorkingDirectory(), "*.lilnut"));
		fc->launchAsync(FileBrowserComponent::FileChooserFlags::openMode | FileBrowserComponent::FileChooserFlags::canSelectFiles, [this](const FileChooser& fc)
			{
				File f = fc.getResult();
				delete& fc;
				if (f == File()) return;
				importSelection(f);
			}
		);
		return;
	}

	var data = JSON::parse(f);
	if (!data.isObject()) return;

	CueListManager::getInstance()->addItemsFromData(data.getProperty(CueListManager::getInstance()->shortName, var()));
}

void PonyEngine::exportSelection()
{
	var data(new DynamicObject());

	data.getDynamicObject()->setProperty(CueListManager::getInstance()->shortName, CueListManager::getInstance()->getExportSelectionData());
	
	String s = JSON::toString(data);

	FileChooser* fc(new FileChooser("Save a LilNut", File::getCurrentWorkingDirectory(), "*.lilnut"));
	fc->launchAsync(FileBrowserComponent::FileChooserFlags::saveMode | FileBrowserComponent::FileChooserFlags::canSelectFiles, [s](const FileChooser& fc)
		{
			File f = fc.getResult();
			delete& fc;
			if (f == File()) return;
			f.replaceWithText(s);
		}
	);
}

String PonyEngine::getMinimumRequiredFileVersion()
{
	return "1.0.0b1";
}
