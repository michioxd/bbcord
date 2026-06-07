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

import bb.cascades 1.4
import bb.system 1.2

NavigationPane {
    id: nav

    property variant currentLoginPage: null
    property variant currentMainPage: null
    property variant currentUserSheet: null

    onCreationCompleted: {
        currentUserSheet = userSheetDefinition.createObject()

        var loginPage = loginPageDefinition.createObject()
        if (loginPage) {
            currentLoginPage = loginPage
            loginPage.loginSucceeded.connect(openMainPage)
            nav.push(loginPage)
        }
        discordClient.loginSucceeded.connect(openMainPage)
        discordClient.autoLogin()
    }

    function openMainPage() {
        if (currentMainPage) {
            return
        }

        var mainPage = mainPageDefinition.createObject()
        if (mainPage) {
            currentMainPage = mainPage
            mainPage.navigationPane = nav
            nav.push(mainPage)
            if (currentLoginPage) {
                nav.remove(currentLoginPage)
                currentLoginPage.destroy()
                currentLoginPage = null
            }
        }
    }

    function showLoginPage() {
        if (currentMainPage) {
            nav.remove(currentMainPage)
            currentMainPage.destroy()
            currentMainPage = null
        }

        if (!currentLoginPage) {
            currentLoginPage = loginPageDefinition.createObject()
            if (currentLoginPage) {
                currentLoginPage.loginSucceeded.connect(openMainPage)
                nav.push(currentLoginPage)
            }
        }
    }

    Menu.definition: MenuDefinition {
        actions: [
            ActionItem {
                title: appStore.currentUserName.length > 0 ? appStore.currentUserName : "User"
                imageSource: appStore.currentUserAvatarSource
                enabled: appStore.loggedIn

                onTriggered: {
                    if (currentUserSheet) {
                        currentUserSheet.open()
                    }
                }
            },

            ActionItem {
                title: "Log out"
                imageSource: "asset:///images/icons/sign-out.png"
                enabled: appStore.loggedIn

                onTriggered: {
                    logoutDialog.show()
                }
            },

            ActionItem {
                title: "Settings"
                enabled: appStore.loggedIn

                onTriggered: {
                    console.log("settings")
                }
            }
        ]
    }

    onPopTransitionEnded: {
        if (page != currentMainPage) {
            page.destroy()
        }
    }

    attachedObjects: [
        ComponentDefinition {
            id: loginPageDefinition
            source: "asset:///LoginPage.qml"
        },
        ComponentDefinition {
            id: mainPageDefinition
            source: "asset:///MainPage.qml"
        },
        ComponentDefinition {
            id: userSheetDefinition
            source: "asset:///UserSheet.qml"
        },

        SystemDialog {
            id: logoutDialog
            title: "Log out"
            body: "Log out and clear saved token?"
            confirmButton.label: "OK"
            cancelButton.label: "Cancel"

            onFinished: {
                if (result == SystemUiResult.ConfirmButtonSelection) {
                    discordClient.logout()
                    nav.showLoginPage()
                }
            }
        }
    ]
}
