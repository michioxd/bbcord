/*
 * Copyright (c) 2011-2015 BlackBerry Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "applicationui.hpp"

#include "core/AppStore.hpp"
#include "core/Client.hpp"
#include "ui/ChatController.hpp"
#include "ui/DmListController.hpp"
#include "ui/MainPageController.hpp"
#include "ui/ServerListController.hpp"
#include "ui/SettingsController.hpp"

#include <bb/cascades/AbstractPane>
#include <bb/cascades/Application>
#include <bb/cascades/LocaleHandler>
#include <bb/cascades/QmlDocument>

using namespace bb::cascades;

ApplicationUI::ApplicationUI()
    : QObject(), m_appStore(new AppStore(this)),
      m_discordClient(new DiscordClient(m_appStore, this)),
      m_chatController(new ChatController(m_discordClient, m_appStore, this)),
      m_dmListController(
          new DmListController(m_discordClient, m_appStore, this)),
      m_mainPageController(
          new MainPageController(m_discordClient, m_appStore, this)),
      m_serverListController(
          new ServerListController(m_discordClient, m_appStore, this)),
      m_settingsController(new SettingsController(this)) {
  // prepare the localization
  m_pTranslator = new QTranslator(this);
  m_pLocaleHandler = new LocaleHandler(this);

  bool res = QObject::connect(m_pLocaleHandler, SIGNAL(systemLanguageChanged()),
                              this, SLOT(onSystemLanguageChanged()));
  // This is only available in Debug builds
  Q_ASSERT(res);
  // Since the variable is not used in the app, this is added to avoid a
  // compiler warning
  Q_UNUSED(res);

  // initial load
  onSystemLanguageChanged();

  // Create scene document from main.qml asset, the parent is set
  // to ensure the document gets destroyed properly at shut down.
  QmlDocument *qml = QmlDocument::create("asset:///main.qml").parent(this);
  qml->setContextProperty("appStore", m_appStore);
  qml->setContextProperty("discordClient", m_discordClient);
  qml->setContextProperty("chatController", m_chatController);
  qml->setContextProperty("dmListController", m_dmListController);
  qml->setContextProperty("mainPageController", m_mainPageController);
  qml->setContextProperty("serverListController", m_serverListController);
  qml->setContextProperty("settingsController", m_settingsController);

  // Create root object for the UI
  AbstractPane *root = qml->createRootObject<AbstractPane>();

  // Set created root object as the application scene
  Application::instance()->setScene(root);
}

void ApplicationUI::onSystemLanguageChanged() {
  QCoreApplication::instance()->removeTranslator(m_pTranslator);
  // Initiate, load and install the application translation files.
  QString locale_string = QLocale().name();
  QString file_name = QString("BBCord_%1").arg(locale_string);
  if (m_pTranslator->load(file_name, "app/native/qm")) {
    QCoreApplication::instance()->installTranslator(m_pTranslator);
  }
}
