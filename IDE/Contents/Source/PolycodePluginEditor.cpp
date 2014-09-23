/*
Copyright (C) 2014 by Joachim Meyer

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "PolycodePluginEditor.h"
#include "PolyPluginManager.h"

using namespace Polycode;

PluginBrowser::PluginBrowser() : UIElement() {
	treeContainer = new UITreeContainer("boxIcon.png", L"Polycode Plugins", 200, 555);
	treeContainer->getRootNode()->toggleCollapsed();
	treeContainer->getRootNode()->addEventListener(this, UITreeEvent::SELECTED_EVENT);
	treeContainer->addEventListener(this, InputEvent::EVENT_MOUSEDOWN);

	pluginNode = treeContainer->getRootNode()->addTreeChild("pluginEditor/plugin_icon.png", "Plugins", NULL);

	addChild(treeContainer);
	selectedData = NULL;

	headerBg = new UIRect(10, 10);
	addChild(headerBg);
	headerBg->setAnchorPoint(-1.0, -1.0, 0.0);
	headerBg->color.setColorHexFromString(CoreServices::getInstance()->getConfig()->getStringValue("Polycode", "uiHeaderBgColor"));

	newPluginButton = new UIImageButton("pluginEditor/new_plugin.png", 1.0, 22, 22);
	addChild(newPluginButton);
	newPluginButton->setPosition(5, 4);

	removeButton = new UIImageButton("main/remove_icon.png", 1.0, 12, 12);
	addChild(removeButton);
	removeButton->setPosition(0, 4);

	selectedNode = NULL;
}

PluginBrowser::~PluginBrowser() {}

void PluginBrowser::removeSelected() {
	selectedNode->getParent()->removeTreeChild(selectedNode);
}

void PluginBrowser::handleEvent(Event *event) {

	if (event->getDispatcher() == treeContainer->getRootNode()) {
		if (event->getEventCode() == UITreeEvent::SELECTED_EVENT) {
			selectedNode = treeContainer->getRootNode()->getSelectedNode();
			PluginBrowserData *data = (PluginBrowserData *)selectedNode->getUserData();
			selectedData = data;
			dispatchEvent(new Event(), Event::CHANGE_EVENT);
		}
	}
	Entity::handleEvent(event);
}


UITree *PluginBrowser::addPlugin(Plugin *plugin) {
	PluginBrowserData *data = new PluginBrowserData();
	data->plugin = plugin;
	return pluginNode->addTreeChild("pluginEditor/plugin_icon.png", plugin->getResourceName(), (void*)data);
}

void PluginBrowser::Resize(Number width, Number height) {
	treeContainer->Resize(width, height - 30);
	treeContainer->setPosition(0, 30);
	headerBg->Resize(width, 30);
	removeButton->setPosition(width - 24, 8);
}


PluginEditorPane::PluginEditorPane(ResourcePool *resourcePool) : UIElement() {

	this->resourcePool = resourcePool;
	changingPlugin = false;
	currentPlugin = NULL;

	headerBg = new UIRect(10, 10);
	addChild(headerBg);
	headerBg->setAnchorPoint(-1.0, -1.0, 0.0);
	headerBg->color.setColorHexFromString(CoreServices::getInstance()->getConfig()->getStringValue("Polycode", "uiHeaderBgColor"));

	propList = new PropList("PLUGIN EDITOR");
	addChild(propList);
	propList->setPosition(0, 0);

	PropSheet *baseProps = new PropSheet("PLUGIN SETTINGS", "");
	propList->addPropSheet(baseProps);

	nameProp = new StringProp("Name:");
	baseProps->addProp(nameProp);
	nameProp->addEventListener(this, Event::CHANGE_EVENT);

	author = new StringProp("Author:");
	baseProps->addProp(author);
	author->addEventListener(this, Event::CHANGE_EVENT);

	addPropButton = new ButtonProp("New Prop");
	baseProps->addProp(addPropButton);
	addPropButton->getButton()->addEventListener(this, UIEvent::CLICK_EVENT);

	baseProps->propHeight = 220;

	propsSheet = new PropSheet("PROPS", "");
	propList->addPropSheet(propsSheet);

	propList->updateProps();

	enabled = false;
}

PluginEditorPane::~PluginEditorPane() {}

void PluginEditorPane::handleEvent(Event *event) {
	if (!changingPlugin) {

		if (event->getDispatcher() == nameProp) {
			currentPlugin->setResourceName(nameProp->get());
			dispatchEvent(new Event(), Event::CHANGE_EVENT);
		}

		if (event->getDispatcher() == author) {
			currentPlugin->author = author->get();
			dispatchEvent(new Event(), Event::CHANGE_EVENT);
		}

		int maxPadding = 0;
		for (int p = 0; p < propsSheet->props.size(); p++){
			if (event->getEventCode() == Event::CHANGE_EVENT && event->getDispatcher() == propsSheet->props[p]) {
				currentPlugin->getProps()[p]->name = propsSheet->props[p]->getPropName();
				currentPlugin->getProps()[p]->type = ((PropEditProp*)propsSheet->props[p])->getPropType();
				if (currentPlugin->getProps()[p]->type == Prop::PROP_COMBO){
					currentPlugin->getProps()[p]->children.clear();
					for (int c = 0; c < ((PropEditProp*)propsSheet->props[p])->comboEditProp->currentCombo->comboEntry->getNumItems(); c++){
						Prop* newProp = new Prop(((PropEditProp*)propsSheet->props[p])->comboEditProp->currentCombo->comboEntry->getItemAtIndex(c)->label, Prop::PROP_COMBO);
						newProp->value = (int)((PropEditProp*)propsSheet->props[p])->comboEditProp->currentCombo->comboEntry->getItemAtIndex(c)->data;
						currentPlugin->getProps()[p]->children.push_back(newProp);
					}
				} else if (currentPlugin->getProps()[p]->type == Prop::PROP_SLIDER){
					currentPlugin->getProps()[p]->children.clear();
					Prop* newProp = new Prop("min", Prop::PROP_SLIDER);
					newProp->value = ((PropEditProp*)propsSheet->props[p])->sliderEditProp->currentSlider->slider->getStart();
					currentPlugin->getProps()[p]->children.push_back(newProp);
					newProp = new Prop("max", Prop::PROP_SLIDER);
					newProp->value = ((PropEditProp*)propsSheet->props[p])->sliderEditProp->currentSlider->slider->getEnd();
					currentPlugin->getProps()[p]->children.push_back(newProp);
				}
				dispatchEvent(new Event(), Event::CHANGE_EVENT);
			}
			
			if (((PropEditProp*)propsSheet->props[p])->updatePadding() > maxPadding){
				maxPadding = ((PropEditProp*)propsSheet->props[p])->updatePadding();
			}
			
			if (event->getEventCode() == Event::REMOVE_EVENT && event->getDispatcher() == propsSheet->props[p]){
				currentPlugin->removeProp(propsSheet->props[p]->getPropName());
				propsSheet->removeProp(propsSheet->props[p]);
				dispatchEvent(new Event(), Event::CHANGE_EVENT);
			}
		}

		padding = maxPadding + 20;

		if (padding < (propList->getWidth() - 370)){
			for (int p = 0; p < propsSheet->props.size(); p++){
				propsSheet->props[p]->propContents->setPositionX(padding);
			}
		}

		if (event->getDispatcher() == addPropButton->getButton()) {
			PropEditProp *newProp = new PropEditProp(new PropProp("", 0));
			propsSheet->addProp(newProp);
			currentPlugin->setProp(new Prop("", 0));
			newProp->addEventListener(this, Event::CHANGE_EVENT);
			newProp->addEventListener(this, Event::REMOVE_EVENT);
			newProp->propContents->setPositionX(padding);
			dispatchEvent(new Event(), Event::CHANGE_EVENT);
		}

		if (event->getEventCode() == Event::CHANGE_EVENT){
			propList->Resize(propList->getWidth(), propList->getHeight());
		}
	}
}

void PluginEditorPane::setPlugin(Plugin *plugin) {
for (int p = propsSheet->props.size(); p > 0; p--){
		propsSheet->removeProp(propsSheet->props[p - 1]);
	}
	changingPlugin = true;

	currentPlugin = plugin;

	nameProp->set(plugin->getResourceName());
	author->set(plugin->author);

	for (int p = 0; p < plugin->getProps().size(); p++) {
		Prop* propEntry = plugin->getProps()[p];
		if (propEntry->type == PropProp::PROP_COMBO){
			ComboProp *newComboProp = new ComboProp(propEntry->name);
			for (int c = 0; c < propEntry->children.size(); c++){
				Prop *comboEntry = propEntry->children[c];
				newComboProp->comboEntry->addComboItem(comboEntry->name, (void*)comboEntry->value);
			}
			setProp(newComboProp->getPropName(), newComboProp);
		} else if (propEntry->type == PropProp::PROP_SLIDER) {
			SliderProp *newSliderProp = new SliderProp(propEntry->name, 0, 0);
			Number min, max;
			for (int s = 0; s < propEntry->children.size(); s++){
				Prop *sliderEntry = propEntry->children[s];
				if (sliderEntry->name == "min"){
					min = sliderEntry->value;
				} else if (sliderEntry->name == "max"){
					max = sliderEntry->value;
				}
			}
			newSliderProp->slider->setStartEnd(min, max);
			setProp(newSliderProp->getPropName(), newSliderProp);
		} else {
			setProp(propEntry->name, new PropProp(propEntry->name, propEntry->type));
		}
	}
	
	
	for (int p = 0; p < propsSheet->props.size(); p++){
		propsSheet->props[p]->propContents->setPositionX(padding);
	}

	enabled = true;

	changingPlugin = false;
}

void PluginEditorPane::Resize(Number width, Number height) {
	headerBg->Resize(width, 30);
	propList->Resize(width, height);
	propList->updateProps();
	
}

std::vector<PropProp*> PluginEditorPane::getProps() {
	return propsSheet->props;
}

void PluginEditorPane::setProp(const String& name, PropProp* prop){
	for (int i = 0; i < propsSheet->props.size(); i++) {
		if (propsSheet->props[i]->getPropName() == name) {
			PropEditProp *newProp = new PropEditProp(prop);
			propsSheet->props[i] = newProp;
			if (((PropEditProp*)newProp)->updatePadding() > (padding - 20))
				padding = ((PropEditProp*)newProp)->updatePadding() + 20;
			newProp->addEventListener(this, Event::CHANGE_EVENT);
			newProp->addEventListener(this, Event::REMOVE_EVENT);
			currentPlugin->getProps()[i]->name = name;
			currentPlugin->getProps()[i]->type = prop->propType;
			if (currentPlugin->getProps()[i]->type == Prop::PROP_COMBO){
				currentPlugin->getProps()[i]->children.clear();
				for (int c = 0; c < ((ComboProp*)prop)->comboEntry->getNumItems(); c++){
					Prop* newProp = new Prop(((ComboProp*)prop)->comboEntry->getItemAtIndex(c)->label, Prop::PROP_COMBO);
					newProp->value = (int)((ComboProp*)prop)->comboEntry->getItemAtIndex(c)->data;
					currentPlugin->getProps()[i]->children.push_back(newProp);
				}
			} else if (currentPlugin->getProps()[i]->type == Prop::PROP_SLIDER){
				currentPlugin->getProps()[i]->children.clear();
				Prop* newProp = new Prop("min", Prop::PROP_SLIDER);
				newProp->value = ((SliderProp*)prop)->slider->getStart();
				currentPlugin->getProps()[i]->children.push_back(newProp);
				newProp = new Prop("max", Prop::PROP_SLIDER);
				newProp->value = ((SliderProp*)prop)->slider->getEnd();
				currentPlugin->getProps()[i]->children.push_back(newProp);
			}
			return;
		}
	}

	PropEditProp *newProp = new PropEditProp(prop);
	if (((PropEditProp*)newProp)->updatePadding() > (padding - 20))
		padding = ((PropEditProp*)newProp)->updatePadding() + 20;
	propsSheet->addProp(newProp);
	newProp->addEventListener(this, Event::CHANGE_EVENT);
	newProp->addEventListener(this, Event::REMOVE_EVENT);
	Prop* pProp = new Prop(name, prop->propType);
	if (prop->propType == Prop::PROP_COMBO){
		for (int c = 0; c < ((ComboProp*)prop)->comboEntry->getNumItems(); c++){
			Prop* comboProp = new Prop(((ComboProp*)prop)->comboEntry->getItemAtIndex(c)->label, Prop::PROP_COMBO);
			comboProp->value = (int)((ComboProp*)prop)->comboEntry->getItemAtIndex(c)->data;
			pProp->children.push_back(comboProp);
		}
	} else if (prop->propType == Prop::PROP_SLIDER){
		Prop* sliderProp = new Prop("min", Prop::PROP_SLIDER);
		sliderProp->value = ((SliderProp*)prop)->slider->getStart();
		pProp->children.push_back(sliderProp);
		sliderProp = new Prop("max", Prop::PROP_SLIDER);
		sliderProp->value = ((SliderProp*)prop)->slider->getEnd();
		pProp->children.push_back(sliderProp);
	}
	currentPlugin->setProp(pProp);
}

PluginMainWindow::PluginMainWindow(ResourcePool *resourcePool) : UIElement() {

	pluginPane = new PluginEditorPane(resourcePool);

	ownsChildren = true;

	addChild(pluginPane);
}

PluginMainWindow::~PluginMainWindow() {}

void PluginMainWindow::Resize(Number width, Number height) {
	Vector2 pos = getScreenPositionForMainCamera();
	scissorBox.setRect(pos.x, pos.y, width, height);
	pluginPane->Resize(width, height);
}

PolycodePluginEditor::PolycodePluginEditor() : PolycodeEditor(true) {
	selectedPluginNode = NULL;
}

PolycodePluginEditor::~PolycodePluginEditor() {
	delete resourcePool;

	mainWindow->setOwnsChildrenRecursive(true);
	delete mainWindow;
	delete mainSizer;
	delete pluginBrowser;
}

bool PolycodePluginEditor::openFile(OSFileEntry filePath) {
	this->filePath = filePath;

	mainSizer = new UIHSizer(100, 100, 200, false);
	addChild(mainSizer);

	pluginBrowser = new PluginBrowser();
	mainSizer->addRightChild(pluginBrowser);

	pluginBrowser->addEventListener(this, Event::CHANGE_EVENT);
	
	String resourceName = filePath.fullPath.replace(parentProject->getRootFolder() + "/", "");

	resourcePool = CoreServices::getInstance()->getResourceManager()->getResourcePoolByName(resourceName);

	if (!resourcePool) {
		resourcePool = new ResourcePool(resourceName, CoreServices::getInstance()->getResourceManager()->getGlobalPool());
		resourcePool->reloadResourcesOnModify = true;
		resourcePool->deleteOnUnsubscribe = true;
		CoreServices::getInstance()->getPluginManager()->loadPluginLibraryIntoPool(resourcePool, filePath.fullPath);
		CoreServices::getInstance()->getResourceManager()->addResourcePool(resourcePool);
	}

	CoreServices::getInstance()->getResourceManager()->subscribeToResourcePool(resourcePool);

	mainWindow = new PluginMainWindow(resourcePool);
	mainSizer->addLeftChild(mainWindow);

	for (int i = 0; i < resourcePool->getResources(Resource::RESOURCE_PLUGIN).size(); i++) {
		Plugin *plugin = (Plugin*)resourcePool->getResources(Resource::RESOURCE_PLUGIN)[i];
		pluginBrowser->addPlugin(plugin);
	}

	mainWindow->pluginPane->addEventListener(this, Event::CHANGE_EVENT);

	pluginBrowser->newPluginButton->addEventListener(this, UIEvent::CLICK_EVENT);
	pluginBrowser->removeButton->addEventListener(this, UIEvent::CLICK_EVENT);

	mainWindow->pluginPane->addEventListener(this, Event::CHANGE_EVENT);

	PolycodeEditor::openFile(filePath);
	return true;
}

void PolycodePluginEditor::saveFile() {
	Object saveFile;
	saveFile.root.name = "plugins";
	saveFile.root.addChild("version", 1);
	for (int p = 0; p < resourcePool->getResources(Resource::RESOURCE_PLUGIN).size(); p++) {
		ObjectEntry* pluginEntry = saveFile.root.addChild("plugin");
		savePluginToObjectEntry((Plugin*)resourcePool->getResources(Resource::RESOURCE_PLUGIN)[p], pluginEntry);
	}
	saveFile.saveToXML(filePath.fullPath);
	setHasChanges(false);
}

void PolycodePluginEditor::savePluginToObjectEntry(Plugin* plugin, ObjectEntry* entry) {
	entry->addChild("name", plugin->getResourceName());
	entry->addChild("author", plugin->author);
	entry->addChild("fileext", plugin->ext);
	entry->addChild("type", (int)plugin->pluginType);
	
	ObjectEntry* sheetEntry = entry->addChild("sheet");
	ObjectEntry* propsEntry = sheetEntry->addChild("props");
	std::vector<Prop*> props = plugin->getProps();
	for (int i = 0; i < props.size(); i++) {
		ObjectEntry* propEntry = propsEntry->addChild("prop");
		propEntry->addChild("type", props[i]->type);
		propEntry->addChild("name", props[i]->name);
		if (props[i]->type == PropProp::PROP_COMBO){
			for (int c = 0; c < props[i]->children.size(); c++){
				ObjectEntry *comboEntry = propEntry->addChild("prop");
				comboEntry->addChild("name", props[i]->children[c]->name);
				comboEntry->addChild("value", props[i]->children[c]->value);
			}
		} else if (props[i]->type == PropProp::PROP_SLIDER){
			for (int s = 0; s < props[i]->children.size(); s++){
				if (props[i]->children[s]->name == "min"){
					ObjectEntry* min = propEntry->addChild("prop");
					min->addChild("name", props[i]->children[s]->name);
					min->addChild("value", props[i]->children[s]->value);
				} else if (props[i]->children[s]->name == "max"){
					ObjectEntry* max = propEntry->addChild("prop");
					max->addChild("name", props[i]->children[s]->name);
					max->addChild("value", props[i]->children[s]->value);
				}
			}
		}
	}
}

void PolycodePluginEditor::handleEvent(Event *event) {

	if (event->getDispatcher() == mainWindow->pluginPane && event->getEventType() == "" && event->getEventCode() == Event::CHANGE_EVENT) {
		if (selectedPluginNode && mainWindow->pluginPane->currentPlugin) {
			selectedPluginNode->setLabelText(mainWindow->pluginPane->currentPlugin->getResourceName());
		}
		setHasChanges(true);
	}

	if (event->getDispatcher() == pluginBrowser->newPluginButton && event->getEventType() == "UIEvent" && event->getEventCode() == UIEvent::CLICK_EVENT) {
		Plugin *newPlugin = new Plugin("New Plugin");
		pluginBrowser->addPlugin(newPlugin)->setSelected();
		resourcePool->addResource(newPlugin);
		setHasChanges(true);
	}

	if (event->getDispatcher() == pluginBrowser->removeButton && event->getEventType() == "UIEvent" && event->getEventCode() == UIEvent::CLICK_EVENT) {

		mainWindow->pluginPane->enabled = false;

		if (pluginBrowser->selectedData->plugin) {
			for (int i = 0; i < resourcePool->getResources(Resource::RESOURCE_PLUGIN).size(); i++) {
				if ((Plugin*)resourcePool->getResources(Resource::RESOURCE_PLUGIN)[i] == pluginBrowser->selectedData->plugin) {
					resourcePool->removeResource(resourcePool->getResource(Resource::RESOURCE_PLUGIN, pluginBrowser->selectedData->plugin->getResourceName()));
					pluginBrowser->removeSelected();
					break;
				}
			}
		}
		setHasChanges(true);
	}

	if (event->getDispatcher() == pluginBrowser) {
		if (event->getEventType() == "" && event->getEventCode() == Event::CHANGE_EVENT) {
			if (pluginBrowser->selectedData) {
				mainWindow->pluginPane->enabled = false;

				if (pluginBrowser->selectedData->plugin) {
					selectedPluginNode = pluginBrowser->selectedNode;
					mainWindow->pluginPane->setPlugin(pluginBrowser->selectedData->plugin);
				}

			}
		}
	}
}

void PolycodePluginEditor::Resize(int x, int y) {
	mainSizer->Resize(((Number)x), ((Number)y));
	PolycodeEditor::Resize(((Number)x), ((Number)y));
}
