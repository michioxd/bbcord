import bb.cascades 1.4
import bb.system 1.2
import "components"

Sheet {
    id: settingsSheet

    onOpened: {
        settingsController.refreshCacheUsed()
    }

    Page {
        titleBar: TitleBar {
            title: qsTr("Settings")
            dismissAction: ActionItem {
                imageSource: "asset:///images/icons/accent/caret-left.png"
                onTriggered: settingsSheet.close()
            }
        }

        ScrollView {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            scrollRole: ScrollRole.Main

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                
                Header {
                    title: qsTr("User interface")
                }
                
                SettingItem {
                    iconSource: "asset:///images/icons/settings/ca_sms.png"
                    title: qsTr("Compact message")
                    description: qsTr("Fit more messages on screen at one time. #IRC")

                    ToggleButton {
                        id: uiCompactSwitch
                    }
                }

                Header {
                    title: qsTr("Storage")
                }
                
                SettingItem {
                    iconSource: "asset:///images/icons/settings/ca_storage_access.png"
                    title: qsTr("Cached media location")
                    description: settingsController.cachePath
                }

                SettingItem {
                    iconSource: "asset:///images/icons/settings/ca_data_management.png"
                    title: qsTr("Used")
                    description: settingsController.cacheUsed
                }

                SettingItem {
                    iconSource: "asset:///images/icons/settings/ca_security_wipe.png"
                    title: qsTr("Clear cache")
                    description: qsTr("Clear cached media such as avatars and attachments.")
                    onTriggered: clearCacheDialog.show()
                }

                Header {
                    title: qsTr("Discord API")
                }
                
                SettingItem {
                    iconSource: ""
                    title: qsTr("API URL")
                    description: "https://discord.com/api/v9/"
                }
                
                SettingItem {
                    iconSource: ""
                    title: qsTr("CDN URL")
                    description: "https://cdn.discordapp.com/"
                }
                
                Container {
                    leftPadding: ui.du(2.0)
                    rightPadding: ui.du(2.0)
                    horizontalAlignment: HorizontalAlignment.Left
                    verticalAlignment: VerticalAlignment.Center

                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    
                    ImageView {
                        imageSource: "asset:///images/icons/settings/ca_message_error.png"
                        preferredWidth: ui.du(8.0)
                        scalingMethod: ScalingMethod.AspectFit
                    }

                    Label {
                        text: qsTr("For security reasons, after you update the Discord API URL, your saved token will be cleared.")
                        multiline: true
                        opacity: 0.5
                        textStyle.fontSize: FontSize.XXSmall
                        verticalAlignment: VerticalAlignment.Center
                    }
                }

                Container {
                    leftPadding: ui.du(2.0)
                    rightPadding: ui.du(2.0)
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill

                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }

                    Button {
                        text: "Reset to official Discord backend"
                        horizontalAlignment: HorizontalAlignment.Fill
                        verticalAlignment: VerticalAlignment.Fill
                    }
                }

                Header {
                    title: qsTr("Miscellaneous")
                }

                SettingItem {
                    iconSource: "asset:///images/icons/settings/ca_audio_active.png"
                    title: qsTr("Sound effects")
                    description: qsTr("Play login, connecting, and error sounds. #MSNTV2")

                    ToggleButton {
                        id: sfxSwitch
                        checked: settingsController.sfxEnabled

                        onCheckedChanged: {
                            settingsController.setSfxEnabled(checked)
                        }
                    }
                }
            }
        }

        attachedObjects: [
            SystemDialog {
                id: clearCacheDialog
                title: qsTr("Clear cache")
                body: qsTr("Clear cached media? This cannot be undone.")
                confirmButton.label: qsTr("Clear")
                cancelButton.label: qsTr("Cancel")

                onFinished: {
                    if (result == SystemUiResult.ConfirmButtonSelection) {
                        settingsController.clearCache()
                    }
                }
            }
        ]

        onCreationCompleted: {
            settingsController.refreshCacheUsed()
        }
    }
}
